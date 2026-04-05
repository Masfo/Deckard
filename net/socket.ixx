module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:socket;

import :ip;

import std;
import deckard.scope_exit;
import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.platform;

namespace deckard::net
{

	enum class socket_protocol
	{
		tcp,
		udp,
	};

	template<socket_protocol Proto>
	class basic_socket
	{
		static constexpr int socket_type = (Proto == socket_protocol::tcp) ? SOCK_STREAM : SOCK_DGRAM;
		static constexpr int protocol    = (Proto == socket_protocol::tcp) ? IPPROTO_TCP : IPPROTO_UDP;
		SOCKET               m_socket{INVALID_SOCKET};

	public:
		bool open(const net::endpoint& ep)
		{
			//
			auto [storage, addrlen] = ep.address.to_sockaddr();
			m_socket                = ::socket(storage.ss_family, socket_type, protocol);
			if (m_socket == INVALID_SOCKET)
				return false;


			return connect(ep);
		}

		void close()
		{
			if (m_socket != INVALID_SOCKET)
			{
				::closesocket(m_socket);
				m_socket = INVALID_SOCKET;
			}
		}

		bool connect(const net::endpoint& ep)
		{
			auto [storage, addrlen] = ep.address.to_sockaddr();
			return ::connect(m_socket, reinterpret_cast<sockaddr*>(&storage), addrlen) != SOCKET_ERROR;
		}
	};

	std::pair<sockaddr_storage, socklen_t> endpoint_to_sockaddr(const endpoint& ep)
	{
		auto [storage, addrlen] = ep.address.to_sockaddr();
		if (ep.address.is_ipv6())
			reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(ep.port);
		else
			reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(ep.port);
		return {storage, addrlen};
	}

	endpoint sockaddr_to_endpoint(const sockaddr_storage& storage)
	{
		endpoint                           ret_ep;
		std::array<char, INET6_ADDRSTRLEN> buf{};

		if (storage.ss_family == AF_INET)
		{
			const auto& addr4 = reinterpret_cast<const sockaddr_in&>(storage);
			::inet_ntop(AF_INET, &addr4.sin_addr, buf.data(), buf.size());
			ret_ep.port = ::ntohs(addr4.sin_port);
		}
		else if (storage.ss_family == AF_INET6)
		{
			const auto& addr6 = reinterpret_cast<const sockaddr_in6&>(storage);
			::inet_ntop(AF_INET6, &addr6.sin6_addr, buf.data(), buf.size());
			ret_ep.port = ::ntohs(addr6.sin6_port);
		}

		ret_ep.address  = ip(buf.data());
		ret_ep.hostname = ret_ep.address.to_string();
		return ret_ep;
	}

	export using udpsocket = basic_socket<socket_protocol::udp>;
	export using tcpsocket = basic_socket<socket_protocol::tcp>;

} // namespace deckard::net
