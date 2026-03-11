// clang-format off
#include <Windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
// clang-format on

import std;
import deckard;
import deckard.net;
using namespace deckard;
using namespace std::string_view_literals;

template<typename T>
concept TransportConcept = requires(T t, std::string_view data, u64 n) {
	{ t.open(u16{}) } -> std::same_as<bool>;
	{ t.close() } -> std::same_as<void>;
	{ t.send(data) } -> std::same_as<u64>;
	{ t.receive(n) } -> std::convertible_to<std::string>;
	//{ t.is_connected() } -> std::same_as<bool>;
	{ t.protocol() } -> std::convertible_to<std::string_view>;
};

enum class Address : u32
{
	V4 = AF_INET,
	V6 = AF_INET6,
};

struct TCP
{
	bool open(u16 port)
	{
		info("TCP open port {}", port);
		return true;
	}

	void close() { info("TCP close"); }

	u64 send(std::string_view input)
	{
		info("TCP::send = '{}'", input);
		return input.size();
	}

	std::string receive(u64 n)
	{
		info("TCP::receive {} bytes", n);
		return "TCP receive";
	}

	std::string_view protocol() const { return "TCP"; }
};

struct UDP
{
	bool open(u16 port, Address inet = Address::V6)
	{
		u32 type = (inet == Address::V6) ? AF_INET6 : AF_INET;

		socket = ::socket(type, SOCK_DGRAM, IPPROTO_UDP);
		if (socket == INVALID_SOCKET)
		{
			dbg::println("Failed to create UDP socket: {}", WSAGetLastError());
			return false;
		}
		return true;
	}

	void close() { ::closesocket(socket); }

	u64 send(std::string_view input)
	{
		info("UDP::send = '{}'", input);
		return input.size();
	}

	std::string receive(u64 n)
	{
		info("UDP::receive {} bytes", n);
		return "UDP receive";
	}

	std::string_view protocol() const { return "UDP"; }

	SOCKET socket{INVALID_SOCKET};
};

template<TransportConcept TransportPolicy>
struct Socket : TransportPolicy
{

	bool open(u16 port) { return TransportPolicy::open(port); }

	void close() { TransportPolicy::close(); }

	u64 send(std::string_view input) { return TransportPolicy::send(input); }

	std::string receive(u64 n)
	{
		dbg::println("Policy: {}", TransportPolicy::receive(n));

		return "TCP recv";
	}

	std::string_view protocol() const { return TransportPolicy::protocol(); }

	Socket()  = default;
	~Socket() = default;

	Socket(const Socket&)            = delete;
	Socket& operator=(const Socket&) = delete;
};

using UDPSocket = Socket<UDP>;
using TCPSocket = Socket<TCP>;

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

i32 deckard_main([[maybe_unused]] utf8::view commandline)
{
	WSADATA wsa{};
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	//
	ULONG                 outBufLen  = 0;
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;


	DWORD dw = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &outBufLen);

	if (dw == ERROR_BUFFER_OVERFLOW)
	{
		dbg::println("Buffer overflow, allocating {} bytes", outBufLen);

		pAddresses = (IP_ADAPTER_ADDRESSES*)MALLOC(outBufLen);

		dw = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);
		if (dw != ERROR_SUCCESS)
		{
			dbg::println("GetAdaptersAddresses failed with error: {}", dw);
			return 0;
		}
	}


	// Capture the local IP of the first active tunnel/VPN adapter found
	std::optional<sockaddr_in> tunnel_local_addr;
	std::optional<sockaddr_in> non_tunnel_local_addr;

	while (pAddresses->Next != nullptr)
	{
		auto friendly_name = platform::string_from_wide(pAddresses->FriendlyName);
		dbg::println("Adapter: {}", friendly_name);

		if (pAddresses->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
		{
			dbg::println("Software loopback, skipping...");
			pAddresses = pAddresses->Next;
			continue;
		}


		const bool mtu_unlimited = (pAddresses->Mtu == std::numeric_limits<ULONG>::max());
		if (mtu_unlimited)
			dbg::println("Mtu: unlimited");
		else
			dbg::println("Mtu: {}", pAddresses->Mtu);

		dbg::println("Adapter: {}", pAddresses->AdapterName);
		dbg::println("Description: {}", platform::string_from_wide(pAddresses->Description));


		auto address_to_string = [](auto address) -> std::string
		{
			if (address == nullptr)
				return "<none>";

			char unicast_str[INET_ADDRSTRLEN]{};
			inet_ntop(AF_INET, address, unicast_str, sizeof(unicast_str));
			return std::string{unicast_str};
		};

		//
		while (pAddresses->FirstUnicastAddress and pAddresses->FirstUnicastAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("Unicast address: {}", address_to_string(&pAddresses->FirstUnicastAddress->Address.lpSockaddr));
			pAddresses->FirstUnicastAddress = pAddresses->FirstUnicastAddress->Next;
		}

		//
		while (pAddresses->FirstAnycastAddress and pAddresses->FirstAnycastAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("Anycast address: {}", address_to_string(&pAddresses->FirstAnycastAddress->Address.lpSockaddr));
			pAddresses->FirstAnycastAddress = pAddresses->FirstAnycastAddress->Next;
		}


		//
		while (pAddresses->FirstMulticastAddress and pAddresses->FirstMulticastAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("FirstMulticastAddress address: {}", address_to_string(&pAddresses->FirstMulticastAddress->Address.lpSockaddr));
			pAddresses->FirstMulticastAddress = pAddresses->FirstMulticastAddress->Next;
		}

		//
		while (pAddresses->FirstDnsServerAddress and pAddresses->FirstDnsServerAddress->Address.lpSockaddr != nullptr)
		{
			dbg::println("FirstDnsServerAddress address: {}", address_to_string(&pAddresses->FirstDnsServerAddress->Address.lpSockaddr));
			pAddresses->FirstDnsServerAddress = pAddresses->FirstDnsServerAddress->Next;
		}


		bool has_mac = pAddresses->PhysicalAddressLength > 0;
		if (has_mac)
		{
			dbg::println(
			  "Physical Address: {:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}",
			  pAddresses->PhysicalAddress[0],
			  pAddresses->PhysicalAddress[1],
			  pAddresses->PhysicalAddress[2],
			  pAddresses->PhysicalAddress[3],
			  pAddresses->PhysicalAddress[4],
			  pAddresses->PhysicalAddress[5]);
		}
		else
			dbg::println("No mac");


		dbg::println("Flags: {}", pAddresses->Flags);

		dbg::println(" - DHCP enabled: {}", (pAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0);
		dbg::println(" - Registered: {}", (pAddresses->Flags & IP_ADAPTER_REGISTER_ADAPTER_SUFFIX) != 0);
		dbg::println(" - Dhcpv4 enabled: {}", (pAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0);
		dbg::println(" - Receive-only: {}", (pAddresses->Flags & IP_ADAPTER_RECEIVE_ONLY) != 0);
		dbg::println(" - NoMulticast: {}", (pAddresses->Flags & IP_ADAPTER_NO_MULTICAST) != 0);
		dbg::println(" - IPv6OthersStatefulConfig: {}", (pAddresses->Flags & IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG) != 0);
		dbg::println(" - Netbios over Tcpip enabled: {}", (pAddresses->Flags & IP_ADAPTER_NETBIOS_OVER_TCPIP_ENABLED) != 0);
		dbg::println(" - Ipv4 enabled: {}", (pAddresses->Flags & IP_ADAPTER_IPV4_ENABLED) != 0);
		dbg::println(" - Ipv6 enabled: {}", (pAddresses->Flags & IP_ADAPTER_IPV6_ENABLED) != 0);
		dbg::println(" - Ipv6 Managed Address Config: {}", (pAddresses->Flags & IP_ADAPTER_IPV6_MANAGE_ADDRESS_CONFIG) != 0);

		bool has_ipv4 = (pAddresses->Flags & IP_ADAPTER_IPV4_ENABLED) != 0;
		bool has_ipv6 = (pAddresses->Flags & IP_ADAPTER_IPV6_ENABLED) != 0;


		dbg::println("IfType: {}", pAddresses->IfType);
		dbg::println("OperStatus: {}", as<u32>(pAddresses->OperStatus));
		dbg::println("Ipv6IfIndex: {}", pAddresses->Ipv6IfIndex);

		bool ipv4_enabled = (pAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0;
		bool ipv6_enabled = (pAddresses->Flags & IP_ADAPTER_IPV6_ENABLED) != 0;

		dbg::println("v4 enabled: {}", ipv4_enabled);
		dbg::println("v6 enabled: {}", ipv6_enabled);

		dbg::println("Connection type: {}", as<u32>(pAddresses->ConnectionType));
		dbg::println("Tunnel type: {}", as<u32>(pAddresses->TunnelType));

		bool is_active = (pAddresses->OperStatus == IfOperStatusUp) and (pAddresses->FirstUnicastAddress != nullptr);
		dbg::println("Active: {}", is_active);

		bool is_tunnel =
		  (pAddresses->IfType == IF_TYPE_TUNNEL)                             // generic encapsulation (6to4, Teredo, ISATAP)
		  or (pAddresses->IfType == IF_TYPE_PPP)                             // PPTP / dial-up VPNs
		  or (pAddresses->IfType == IF_TYPE_PROP_VIRTUAL)                    // TAP-driver VPNs (OpenVPN, WireGuard, ...)
		  or (pAddresses->IfType == IF_TYPE_MPLS_TUNNEL)                     // MPLS tunnels
		  or (pAddresses->IfType == IF_TYPE_PPPMULTILINKBUNDLE)              // PPP multilink
		  or (pAddresses->TunnelType != TUNNEL_TYPE_NONE)                    // explicit tunnel type from OS
		  or (pAddresses->PhysicalAddressLength == 0 and not mtu_unlimited); // no MAC = virtual/tunnel (excludes loopback)
		dbg::println("Tunnel: {}", is_tunnel);

		//
		if (is_tunnel == false and is_active == true and has_mac and not non_tunnel_local_addr.has_value())
		{
			for (auto* ua = pAddresses->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				if (ua->Address.lpSockaddr->sa_family == AF_INET)
				{
					non_tunnel_local_addr = *reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
					char ip_str[INET_ADDRSTRLEN]{};
					inet_ntop(AF_INET, &non_tunnel_local_addr->sin_addr, ip_str, sizeof(ip_str));
					dbg::println("Non-tunnel local IP captured: {}", ip_str);
					// break;
				}
			}
		}

		// Capture the first active tunnel's IPv4 unicast address for later bind()
		if (is_active and is_tunnel and not tunnel_local_addr.has_value())
		{
			for (auto* ua = pAddresses->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				if (ua->Address.lpSockaddr->sa_family == AF_INET)
				{
					tunnel_local_addr           = *reinterpret_cast<sockaddr_in*>(ua->Address.lpSockaddr);
					tunnel_local_addr->sin_port = 0; // port 0: OS picks the ephemeral source port
					char ip_str[INET_ADDRSTRLEN]{};
					inet_ntop(AF_INET, &tunnel_local_addr->sin_addr, ip_str, sizeof(ip_str));
					dbg::println("Tunnel local IP captured: {}", ip_str);
					break;
				}
			}
		}

		dbg::println();


		pAddresses = pAddresses->Next;
	}

	info("GAA: {}", dw);
	SOCKET tunnel_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#if 1
	if (non_tunnel_local_addr.has_value())
	{
		char ip_str[INET_ADDRSTRLEN]{};
		inet_ntop(AF_INET, &non_tunnel_local_addr->sin_addr, ip_str, sizeof(ip_str));
		dbg::println("Non-tunnel local IP: {}", ip_str);
		if (::bind(tunnel_socket, reinterpret_cast<sockaddr*>(&non_tunnel_local_addr.value()), sizeof(sockaddr_in)) != SOCKET_ERROR)
		{
			dbg::println("Socket bound to non-tunnel interface");
		}
		else
		{
			dbg::println("bind to non-tunnel failed: {}", WSAGetLastError());
			closesocket(tunnel_socket);
			WSACleanup();
			return 4;
		}
	}
#else
	// Send through the tunnel interface: bind the socket to its local IP before connecting
	if (tunnel_local_addr.has_value())
	{
		// socket_old sock(transport::tcp, &ok);
		//  bind() pins this socket to the tunnel interface — all traffic leaves via it
		if (::bind(tunnel_socket, reinterpret_cast<sockaddr*>(&tunnel_local_addr.value()), sizeof(sockaddr_in)) != SOCKET_ERROR)
		{
			dbg::println("Socket bound to tunnel interface");

			// connect() and send() now go through the tunnel
		}
		else
		{
			dbg::println("bind to tunnel failed: {}", WSAGetLastError());
			closesocket(tunnel_socket);
			WSACleanup();
			return 5;
		}
	}

#endif

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return 4;


	// if (bind(s, reinterpret_cast<sockaddr*>(&tunnel_local_addr.value()), sizeof(*tunnel_local_addr)) == SOCKET_ERROR)
	//{
	//	closesocket(s);
	//	WSACleanup();
	//	return 5;
	// }
	//
	addrinfo hints{};
	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;


	std::string host = "api.ipify.org";

	addrinfo* res = nullptr;
	if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0 or res == nullptr)
	{
		closesocket(s);
		WSACleanup();
		return 6;
	}

	int rc = connect(s, res->ai_addr, static_cast<int>(res->ai_addrlen));
	freeaddrinfo(res);
	if (rc == SOCKET_ERROR)
	{
		closesocket(s);
		WSACleanup();
		return 7;
	}

	std::string req = std::format("GET / HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n", host);
	if (send(s, req.c_str(), static_cast<int>(req.size()), 0) == SOCKET_ERROR)
	{
		closesocket(s);
		WSACleanup();
		return 8;
	}

	std::array<char, 4096> rx{};
	for (;;)
	{
		int n = recv(s, rx.data(), static_cast<int>(rx.size()), 0);
		if (n <= 0)
			break;
		std::string ret{rx.data(), static_cast<size_t>(n)};
		dbg::println("{}", ret);
	}

	closesocket(s);
	WSACleanup();


	UDPSocket udps;
	TCPSocket tdps;

	udps.send("hello");

	tdps.send("hello");

	info("{}", udps.receive(10));
	info("{}", tdps.receive(20));


	return 0;
}
