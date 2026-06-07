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
import deckard.helpers;

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

	export std::pair<sockaddr_storage, socklen_t> endpoint_to_sockaddr(const endpoint& ep)
	{
		auto [storage, addrlen] = ep.address.to_sockaddr();
		if (ep.address.is_ipv6())
			reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(ep.port);
		else
			reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(ep.port);
		return {storage, addrlen};
	}

	export endpoint sockaddr_to_endpoint(const sockaddr_storage& storage)
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

	// Buffered raw TCP socket. peer is either an IP endpoint or a domain endpoint.
	// Data is buffered via write() and flushed to the peer on send().
	export class socket
	{
		SOCKET                                m_socket{INVALID_SOCKET};
		endpoint                              m_peer;
		std::vector<u8>                       m_buffer;
		std::chrono::steady_clock::time_point m_send_time{};
		std::chrono::milliseconds             m_rtt{};

	public:
		socket() = default;

		// peer: IP-based  -> endpoint(ip("1.2.3.4"), 80)
		// peer: domain    -> endpoint("example.com", 80)
		explicit socket(endpoint peer)
			: m_peer(std::move(peer))
		{
		}

		private:
		explicit socket(SOCKET s, endpoint peer)
			: m_socket(s)
			, m_peer(std::move(peer))
		{
		}

		public:
		~socket() { close(); }

		socket(const socket&)            = delete;
		socket& operator=(const socket&) = delete;
		socket(socket&&)                 = default;
		socket& operator=(socket&&)      = default;

		bool bind(const endpoint& ep)
		{
			if (m_socket == INVALID_SOCKET)
			{
				auto [storage, addrlen] = ep.address.to_sockaddr();
				m_socket                = ::socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
				if (m_socket == INVALID_SOCKET)
					return false;
			}

			auto [storage, addrlen] = endpoint_to_sockaddr(ep);
			return ::bind(m_socket, reinterpret_cast<sockaddr*>(&storage), addrlen) != SOCKET_ERROR;
		}

		bool listen(int backlog = SOMAXCONN)
		{
			if (m_socket == INVALID_SOCKET)
				return false;

			return ::listen(m_socket, backlog) != SOCKET_ERROR;
		}

		std::optional<socket> accept()
		{
			if (m_socket == INVALID_SOCKET)
				return std::nullopt;

			sockaddr_storage storage{};
			socklen_t        len = sizeof(storage);

			SOCKET client = ::accept(m_socket, reinterpret_cast<sockaddr*>(&storage), &len);
			if (client == INVALID_SOCKET)
				return std::nullopt;

			return socket(client, sockaddr_to_endpoint(storage));
		}

		bool set_blocking(bool block)
		{
			if (m_socket == INVALID_SOCKET)
				return false;

			u_long mode = block ? 0 : 1;
			return ::ioctlsocket(m_socket, FIONBIO, &mode) != SOCKET_ERROR;
		}

		bool set_timeout(std::chrono::milliseconds timeout)
		{
			if (m_socket == INVALID_SOCKET)
				return false;

			DWORD timeout_ms = static_cast<DWORD>(timeout.count());
			if (::setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) ==
				SOCKET_ERROR)
				return false;

			return ::setsockopt(
					 m_socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout_ms), sizeof(timeout_ms)) !=
				   SOCKET_ERROR;
		}

		bool shutdown(int how = SD_BOTH)
		{
			if (m_socket == INVALID_SOCKET)
				return false;

			return ::shutdown(m_socket, how) != SOCKET_ERROR;
		}

		// Buffer a string
		void write(std::string_view str) { m_buffer.insert(m_buffer.end(), str.begin(), str.end()); }

		// Buffer raw bytes
		void write(std::span<const u8> data) { m_buffer.insert(m_buffer.end(), data.begin(), data.end()); }

		// Buffer any arithmetic type as big-endian bytes
		template<typename T>
		requires(std::is_arithmetic_v<T> and not std::same_as<T, bool>)
		void write(T value)
		{
			const auto bytes = to_byte_array_be(value);
			m_buffer.insert(m_buffer.end(),
							reinterpret_cast<const u8*>(bytes.data()),
							reinterpret_cast<const u8*>(bytes.data()) + bytes.size());
		}

		// Connect to peer (if not already) and flush the buffer
		bool send()
		{
			if (m_buffer.empty())
				return true;

			if (not ensure_connected())
				return false;

			m_rtt       = {};
			m_send_time = std::chrono::steady_clock::now();

			const int sent =
			  ::send(m_socket, reinterpret_cast<const char*>(m_buffer.data()), static_cast<int>(m_buffer.size()), 0);

			m_buffer.clear();
			return sent != SOCKET_ERROR;
		}

		void close()
		{
			if (m_socket != INVALID_SOCKET)
			{
				::closesocket(m_socket);
				m_socket = INVALID_SOCKET;
			}
		}

		// Receive up to n bytes; returns however many arrived
		std::vector<u8> recv(size_t n)
		{
			if (not connected())
				return {};

			std::vector<u8> buf(n);
			const int       got = ::recv(m_socket, reinterpret_cast<char*>(buf.data()), static_cast<int>(n), 0);
			if (got <= 0)
				return {};
			buf.resize(got);
			return buf;
		}

		// Receive all data until the connection is closed; returns raw bytes
		auto recv() -> std::pair<std::vector<u8>, std::chrono::milliseconds>
		{
			if (not connected())
				return {};

			std::vector<u8>      result;
			std::array<u8, 4096> buf{};
			while (true)
			{
				const int n = ::recv(m_socket, reinterpret_cast<char*>(buf.data()), static_cast<int>(buf.size()), 0);
				if (n <= 0)
					break;
				result.insert(result.end(), buf.begin(), buf.begin() + n);
			}
			if (m_send_time != std::chrono::steady_clock::time_point{})
				m_rtt =
				  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_send_time);
			return {result, m_rtt};
		}

		// Receive all data as a string
		auto recv_string() -> std::pair<std::string, std::chrono::milliseconds>
		{
			const auto [bytes, rtt] = recv();
			return {{reinterpret_cast<const char*>(bytes.data()), bytes.size()}, rtt};
		}

		bool connected() const { return m_socket != INVALID_SOCKET; }

		const endpoint& peer() const { return m_peer; }

		endpoint local_endpoint() const
		{
			if (m_socket == INVALID_SOCKET)
				return {};

			sockaddr_storage storage{};
			socklen_t        len = sizeof(storage);
			if (::getsockname(m_socket, reinterpret_cast<sockaddr*>(&storage), &len) == SOCKET_ERROR)
				return {};

			return sockaddr_to_endpoint(storage);
		}

		endpoint remote_endpoint() const
		{
			if (m_socket == INVALID_SOCKET)
				return m_peer;

			sockaddr_storage storage{};
			socklen_t        len = sizeof(storage);
			if (::getpeername(m_socket, reinterpret_cast<sockaddr*>(&storage), &len) == SOCKET_ERROR)
				return m_peer;

			return sockaddr_to_endpoint(storage);
		}


	private:
		bool ensure_connected()
		{
			if (m_socket != INVALID_SOCKET)
				return true;

			if (not m_peer.valid())
				return false;

			auto [storage, addrlen] = m_peer.address.to_sockaddr();
			m_socket                = ::socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
			if (m_socket == INVALID_SOCKET)
				return false;

			auto [s, len] = endpoint_to_sockaddr(m_peer);
			if (::connect(m_socket, reinterpret_cast<sockaddr*>(&s), len) == SOCKET_ERROR)
			{
				close();
				return false;
			}
			return true;
		}
	};

} // namespace deckard::net
