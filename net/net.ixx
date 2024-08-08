module;
// #include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net;

import std;
import deckard.scope_exit;
import deckard.types;
import deckard.debug;
import deckard.win32;

namespace deckard::net
{

	class NetInitializer
	{
	public:
		NetInitializer()
		{
			WSADATA wsadata{};
			WSAStartup(0x0202, &wsadata);
		}

		~NetInitializer() { WSACleanup(); }
	} dummy; // Just to automatically init winsock2

	// ######################
	using namespace deckard::system;


	export enum class protocol {
		v4,
		v6,
	};
	consteval void enable_bitmask_operations(protocol);

	enum class transport
	{
		NUL,
		TCP,
		UDP,
	};

	export struct address
	{
		std::string host;
		u16         port{0};
	};

	//


	sockaddr_in address_to_sockaddr(address addr)
	{
		sockaddr_in ret{0};
		// TODO: ipv6
		ret.sin_family = AF_INET;
		inet_pton(AF_INET, addr.host.c_str(), &ret.sin_addr.S_un.S_addr);
		ret.sin_port = htons(addr.port);
		return ret;
	}

	export std::optional<std::string> hostname_to_ip(std::string_view hostname, protocol version)
	{

		addrinfo hints{}, *res{};

		scope_exit _([&] { freeaddrinfo(res); });

		if (version == protocol::v4)
			hints.ai_family = AF_INET;
		else
			hints.ai_family = AF_INET6;

		hints.ai_socktype = SOCK_STREAM; // TCP
		hints.ai_flags    = AI_PASSIVE;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(hostname.data(), nullptr, &hints, &res) != 0)
		{
			dbg::println("Failed to resolve hostname: {}", hostname);
			return {};
		}


		for (addrinfo* ptr = res; ptr != nullptr; ptr = ptr->ai_next)
		{
			char       buffer[46];
			DWORD      buffer_len  = 46;
			LPSOCKADDR sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
			auto       ret         = WSAAddressToStringA(sockaddr_ip, (DWORD)ptr->ai_addrlen, 0, buffer, &buffer_len);

			if (ret == 0 and ptr->ai_family == AF_INET and version == protocol::v4)
				return buffer;

			if (ret == 0 and ptr->ai_family == AF_INET6 and version == protocol::v6)
				return buffer;
		}

		return {};
	}

	//
	class socket
	{
	public:
		socket() = default;

		socket(transport proto, bool* ok)
		{
			if (proto == transport::NUL)
			{
				dbg::trace("Transport protocol cannot be nul");
				if (ok != nullptr)
					*ok = false;
				return;
			}
			else if (proto == transport::TCP)
			{
				// TODO: ipv6
				m_socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (m_socket == INVALID_SOCKET)
				{
					dbg::println("Failed to create socket: {}", WSAGetLastError());
					if (ok != nullptr)
						*ok = false;
					return;
				}
			}
			else if (proto == transport::UDP)
			{
				// TODO: ipv6
				m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
				if (m_socket == INVALID_SOCKET)
				{
					dbg::println("Failed to create socket: {}", WSAGetLastError());
					if (ok != nullptr)
						*ok = false;
					return;
				}
			}
			//
			i32 opt = 1;
			if (::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR)
			{
				dbg::trace("Socket creation incomplete. Failed to set SO_REUSEADDR");
			}

			m_open = true;
			if (ok != nullptr)
				*ok = true;
		}

		bool bind(address addr)
		{
			m_address = addr;

			SOCKADDR_IN ladder = address_to_sockaddr(addr);
			m_sockaddr         = ladder;
			m_addr_size        = sizeof(ladder);

			if (::bind(m_socket, (SOCKADDR*)&ladder, sizeof(ladder)) == SOCKET_ERROR)
			{
				dbg::println("Socket binding failed: {}", WSAGetLastError());
				return false;
			}

			return true;
		}

		bool listen(int backlog) const
		{

			if (::listen(m_socket, backlog) == SOCKET_ERROR)
			{
				dbg::println("Socket listening failed: {}", WSAGetLastError());
				return false;
			}

			return true;
		}

		std::optional<socket> accept()
		{
			socket ret;
			SOCKET acceptsocket = INVALID_SOCKET;

			acceptsocket = ::accept(m_socket, (SOCKADDR*)&ret.m_sockaddr, &ret.m_addr_size);
			if (acceptsocket == INVALID_SOCKET)
			{
				dbg::println("Socket accept failed: {}", WSAGetLastError());
				return {};
			}

			return ret;
		}

		bool connect(address addr) const
		{
			SOCKADDR_IN sockaddr{0};
			addrinfo    hints{}, *res{};

			// TODO: ipv6
			hints.ai_family   = AF_INET;
			hints.ai_socktype = SOCK_STREAM; // TCP

			if (::getaddrinfo(addr.host.c_str(), std::to_string(addr.port).c_str(), &hints, &res) != 0)
			{
				dbg::println("Socket connect failed, failed to resolve host: {}", WSAGetLastError());
				return false;
			}

			sockaddr = *(SOCKADDR_IN*)res->ai_addr;
			freeaddrinfo(res);

			if (::connect(m_socket, (SOCKADDR*)&sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
			{
				dbg::println("Socket connect failed: {}", WSAGetLastError());
				return false;
			}

			return true;
		}

		i32 send(const std::span<char> buffer) const
		{
			i32 size = ::send(m_socket, buffer.data(), buffer.size_bytes(), 0);

			if (size != SOCKET_ERROR)
				return 0;

			return size;
		}

		i32 receive(std::span<char> buffer) const
		{
			i32 size = ::recv(m_socket, buffer.data(), buffer.size_bytes(), 0);

			if (size == SOCKET_ERROR)
				return SOCKET_ERROR;

			return size;
		}

		//

		//
		//
		void close()
		{
			closesocket(m_socket);
			m_open = false;
		}

	private:
		address   m_address{.host = "", .port = 0};
		transport m_transport{transport::NUL};


		SOCKET      m_socket{INVALID_SOCKET};
		SOCKADDR_IN m_sockaddr{.sin_family = AF_INET};
		i32         m_addr_size{sizeof(m_sockaddr)};
		bool        m_open{false};
	};


} // namespace deckard::net
