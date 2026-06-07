module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

export import :protocol;
export import :auth;
export import :socket;
export import :server;
export import :client;
export import :ip;

import deckard.types;

import std;

// Retransmission timer: https://www.rfc-editor.org/rfc/rfc2988
// For example, assuming an RTO of 500 ms, requests would be sent
// at times 0 ms, 500 ms, 1500 ms, 3500 ms, 7500 ms, 15500 ms,
// and 31500 ms.  If the client has not received a response after
// 39500 ms, the client will consider the transaction to have timed
// out.
//
// STUN: https://www.rfc-editor.org/rfc/rfc5389
//       https://www.rfc-editor.org/rfc/rfc5780.txt


namespace deckard::net
{

	// Clumsy, lag simulator


	export void initialize()
	{
		WSADATA wsadata{};
		WSAStartup(0x0202, &wsadata);
	}

	export void deinitialize() { WSACleanup(); }

	export std::string wsa_error_string(int error = WSAGetLastError())
	{
		std::string ret(256, '\0');
		const DWORD written = FormatMessageA(
		  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		  nullptr,
		  static_cast<DWORD>(error),
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  ret.data(),
		  static_cast<DWORD>(ret.size()),
		  nullptr);
		if (written == 0)
			return std::format("WSA error {}", error);
		ret.resize(written);
		while (not ret.empty() and (ret.back() == '\n' or ret.back() == '\r' or ret.back() == ' '))
			ret.pop_back();
		return ret;
	}

	export std::string hostname()
	{
		std::array<char, 256> hostname{0};
		if (gethostname(hostname.data(), as<i32>(hostname.size())) == SOCKET_ERROR)
			return "<unknown>";
		return std::string{hostname.data()};
	}

	export u32
	measure_mtu_for_target(const net::endpoint& target, std::chrono::milliseconds timeout = std::chrono::milliseconds{500})
	{
		constexpr u32 IP_HEADER  = 20;
		constexpr u32 UDP_HEADER = 8;
		constexpr u32 MIN_MTU    = 576;
		constexpr u32 MAX_MTU    = 1500;

		auto [storage, addrlen] = target.address.to_sockaddr();
		if (target.address.is_ipv6())
			reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(target.port);
		else
			reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(target.port);

		SOCKET s = ::socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET)
			return 0;

		DWORD df = 1;
		if (storage.ss_family == AF_INET6)
			::setsockopt(s, IPPROTO_IPV6, IPV6_DONTFRAG, reinterpret_cast<const char*>(&df), sizeof(df));
		else
			::setsockopt(s, IPPROTO_IP, IP_DONTFRAGMENT, reinterpret_cast<const char*>(&df), sizeof(df));

		DWORD timeout_ms = as<DWORD>(timeout.count());
		::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms));

		u32 lo     = MIN_MTU - IP_HEADER - UDP_HEADER;
		u32 hi     = MAX_MTU - IP_HEADER - UDP_HEADER;
		u32 result = lo;

		std::vector<char> probe;
		while (lo <= hi)
		{
			u32 mid = (lo + hi) / 2;
			probe.assign(mid, 0x00);

			int sent = ::sendto(
			  s,
			  probe.data(),
			  static_cast<int>(probe.size()),
			  0,
			  reinterpret_cast<sockaddr*>(&storage),
			  static_cast<int>(addrlen));

			if (sent != SOCKET_ERROR)
			{
				result = mid;
				lo     = mid + 1;
			}
			else if (WSAGetLastError() == WSAEMSGSIZE)
			{
				// Fragmented
				hi = mid - 1;
			}
			else
			{
				break;
			}
		}

		::closesocket(s);
		return result + IP_HEADER + UDP_HEADER;
	}

	export std::optional<u32> ping(const net::endpoint& target, std::chrono::milliseconds timeout = std::chrono::milliseconds{1000})
	{
		auto [storage, addrlen] = target.address.to_sockaddr();
		if (addrlen == 0)
			return std::nullopt;

		if (target.address.is_ipv6())
			reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(target.port);
		else
			reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(target.port);

		SOCKET s = ::socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET)
			return std::nullopt;

		// Set non-blocking so connect() returns immediately
		u_long nonblocking = 1;
		::ioctlsocket(s, FIONBIO, &nonblocking);

		auto start = std::chrono::steady_clock::now();
		::connect(s, reinterpret_cast<sockaddr*>(&storage), static_cast<int>(addrlen));

		// Wait for socket to become writable (= connected) within timeout
		timeval tv{};
		tv.tv_sec  = as<long>(timeout.count() / 1000);
		tv.tv_usec = as<long>((timeout.count() % 1000) * 1000);

		fd_set wset{};
		FD_ZERO(&wset);
		FD_SET(s, &wset);

		fd_set eset{};
		FD_ZERO(&eset);
		FD_SET(s, &eset);

		const int ready = ::select(0, nullptr, &wset, &eset, &tv);
		auto      end   = std::chrono::steady_clock::now();
		::closesocket(s);

		if (ready <= 0 or FD_ISSET(s, &eset))
			return std::nullopt;

		return static_cast<u32>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	}



} // namespace deckard::net
