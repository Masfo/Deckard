﻿module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:socket;

import :address;

import std;
import deckard.scope_exit;
import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.win32;

namespace deckard::net
{
	// TODO: TLSE
	using namespace deckard::system;

	//


	sockaddr_in address_to_sockaddr(address addr)
	{
		sockaddr_in ret{0};

		ret.sin_family = (addr.proto == protocol::v4) ? AF_INET : AF_INET6;

		inet_pton(ret.sin_family, addr.hostname.c_str(), &ret.sin_addr.S_un.S_addr);
		ret.sin_port = htons(addr.port);
		return ret;
	}

	address sockaddr_to_address(sockaddr_in addr)
	{
		address ret_addr;

		std::array<char, INET6_ADDRSTRLEN> buf{};

		// TODO: ipv6
		::inet_ntop(AF_INET, &addr.sin_addr.S_un.S_addr, buf.data(), buf.size());
		ret_addr.hostname = std::string(buf.data());
		ret_addr.port     = ::ntohs(addr.sin_port);

		return ret_addr;
	}

	export std::optional<std::string> hostname_to_ip(std::string_view hostname, protocol version)
	{

		addrinfo hints{}, *res{};

		scope_exit _([&] { freeaddrinfo(res); });

		if (version == protocol::v4)
			hints.ai_family = AF_INET;
		else
			hints.ai_family = AF_INET6;


		if (getaddrinfo(hostname.data(), nullptr, &hints, &res) != 0)
		{
			dbg::println("Failed to resolve hostname: {}", hostname);
			return {};
		}


		for (addrinfo* ptr = res; ptr != nullptr; ptr = ptr->ai_next)
		{
			wchar_t buffer[INET6_ADDRSTRLEN]{};
			DWORD   buffer_len = INET6_ADDRSTRLEN;

			auto ret = WSAAddressToStringW(ptr->ai_addr, (DWORD)ptr->ai_addrlen, 0, buffer, &buffer_len);

			if (ret == 0 and ptr->ai_family == AF_INET and version == protocol::v4)
			{
				auto ip = as<sockaddr_in*>(ptr->ai_addr);

				auto& i = ip->sin_addr.S_un.S_un_b;
				dbg::println("raw: {:d}.{:d}.{:d}.{:d}", i.s_b1, i.s_b2, i.s_b3, i.s_b4);


				return system::from_wide(buffer);
			}

			if (ret == 0 and ptr->ai_family == AF_INET6 and version == protocol::v6)
			{
				auto  ip    = as<sockaddr_in6*>(ptr->ai_addr);
				auto& ipraw = ip->sin6_addr.u.Byte;
				dbg::print("raw: ");
				for (int i = 0; i < 15; i += 2)
					dbg::print("{:02x}{:02x}{}", ipraw[i], ipraw[i + 1], i < 14 ? ":" : "");
				dbg::println();


				return system::from_wide(buffer);
			}
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
			if (proto == transport::nul)
			{
				dbg::trace("Transport protocol cannot be nul");
				if (ok != nullptr)
					*ok = false;
				return;
			}
			else if (proto == transport::tcp)
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
			else if (proto == transport::udp)
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

			ret.m_socket    = acceptsocket;
			ret.m_address   = sockaddr_to_address(ret.m_sockaddr);
			ret.m_transport = m_transport;

			return ret;
		}

		bool connect(address addr) const
		{
			SOCKADDR_IN sockaddr{0};
			addrinfo    hints{}, *res{};

			// TODO: ipv6
			hints.ai_family   = AF_INET;
			hints.ai_socktype = SOCK_STREAM; // TCP

			if (::getaddrinfo(addr.hostname.c_str(), std::to_string(addr.port).c_str(), &hints, &res) != 0)
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
			i32 size = ::send(m_socket, buffer.data(), as<i32>(buffer.size_bytes()), 0);

			if (size != SOCKET_ERROR)
				return 0;

			return size;
		}

		i32 receive(std::span<char> buffer) const
		{
			i32 size = ::recv(m_socket, buffer.data(), as<i32>(buffer.size_bytes()), 0);

			if (size == SOCKET_ERROR)
				return SOCKET_ERROR;

			return size;
		}

		i32 send_to(std::span<char> data, address to)
		{
			SOCKADDR_IN s_to{};
			addrinfo    hints{}, *res{};
			// TODO: ipv6
			hints.ai_family   = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;

			if (getaddrinfo(to.hostname.c_str(), std::to_string(to.port).c_str(), &hints, &res) != 0)
			{
				dbg::println("sendto failed, failed to resolve host: {}", WSAGetLastError());
				return SOCKET_ERROR;
			}

			s_to = *(SOCKADDR_IN*)res->ai_addr;
			freeaddrinfo(res);

			i32 bytes = ::sendto(m_socket, data.data(), as<i32>(data.size()), 0, (SOCKADDR*)&s_to, sizeof(s_to));

			if (bytes == SOCKET_ERROR)
				return SOCKET_ERROR;

			return bytes;
		}

		i32 receive_from(std::span<char> buffer, address* from)
		{
			SOCKADDR_IN s_from{};
			i32         fromsize = sizeof(s_from);
			i32         bytes    = ::recvfrom(m_socket, buffer.data(), as<i32>(buffer.size()), 0, (SOCKADDR*)&s_from, &fromsize);

			if (from != nullptr and bytes != SOCKET_ERROR)
				*from = sockaddr_to_address(s_from);

			return bytes;
		}

		void close()
		{
			closesocket(m_socket);
			m_open = false;
		}

		const address& getaddress() const { return m_address; }

		const transport& get_transport() const { return m_transport; }

		bool is_open() const { return m_open; }

	private:
		address     m_address{.hostname = "", .port = 0};
		SOCKADDR_IN m_sockaddr{.sin_family = AF_INET};
		SOCKET      m_socket{INVALID_SOCKET};
		transport   m_transport{transport::nul};


		i32  m_addr_size{sizeof(m_sockaddr)};
		bool m_open{false};
	};

} // namespace deckard::net
