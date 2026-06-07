module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:server;

import :protocol;
import :auth;
import :socket;
import :ip;

import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.uuid;
import deckard.scope_exit;

import std;

namespace deckard::net
{
	// ── Internal per-client state ─────────────────────────────────────────────

	struct client_context
	{
		SOCKET             tcp_socket{INVALID_SOCKET};
		endpoint           remote{};
		uuid::v4::uuid     session_id{};
		std::vector<u8>    recv_buf{};
		u32                next_sequence{};
		pow_challenge      challenge{};
		bool               authenticated{false};
	};

	// ── server ────────────────────────────────────────────────────────────────

	export class server
	{
	public:
		using connect_fn    = std::function<void(const uuid::v4::uuid&, const endpoint&)>;
		using disconnect_fn = std::function<void(const uuid::v4::uuid&)>;
		using message_fn    = std::function<void(const uuid::v4::uuid&, const message&)>;

		server()  = default;
		~server() { stop(); }

		// Non-copyable, non-movable (owns OS sockets)
		server(const server&)            = delete;
		server& operator=(const server&) = delete;

		// ── Configuration ─────────────────────────────────────────────────────

		void set_pow_difficulty(u8 bits) { m_pow_difficulty = bits; }

		void on_connect(connect_fn fn)       { m_on_connect    = std::move(fn); }
		void on_disconnect(disconnect_fn fn) { m_on_disconnect = std::move(fn); }
		void on_message(message_fn fn)       { m_on_message    = std::move(fn); }

		// ── Lifecycle ─────────────────────────────────────────────────────────

		bool bind(const endpoint& ep)
		{
			// TCP listening socket
			auto [storage, addrlen] = ep.address.to_sockaddr();
			if (ep.address.is_ipv6())
				reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(ep.port);
			else
				reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(ep.port);

			m_tcp = ::socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
			if (m_tcp == INVALID_SOCKET)
			{
				dbg::eprintln("server::bind TCP socket failed");
				return false;
			}

			BOOL reuse = TRUE;
			::setsockopt(m_tcp, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

			if (::bind(m_tcp, reinterpret_cast<sockaddr*>(&storage), as<int>(addrlen)) == SOCKET_ERROR)
			{
				dbg::eprintln("server::bind TCP bind failed");
				::closesocket(m_tcp);
				m_tcp = INVALID_SOCKET;
				return false;
			}

			if (::listen(m_tcp, SOMAXCONN) == SOCKET_ERROR)
			{
				dbg::eprintln("server::bind TCP listen failed");
				::closesocket(m_tcp);
				m_tcp = INVALID_SOCKET;
				return false;
			}

			// UDP socket on the same address/port
			m_udp = ::socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
			if (m_udp == INVALID_SOCKET)
			{
				dbg::eprintln("server::bind UDP socket failed");
				::closesocket(m_tcp);
				m_tcp = INVALID_SOCKET;
				return false;
			}

			::setsockopt(m_udp, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

			if (::bind(m_udp, reinterpret_cast<sockaddr*>(&storage), as<int>(addrlen)) == SOCKET_ERROR)
			{
				dbg::eprintln("server::bind UDP bind failed");
				::closesocket(m_tcp);
				::closesocket(m_udp);
				m_tcp = m_udp = INVALID_SOCKET;
				return false;
			}

			// Non-blocking
			u_long nonblock = 1;
			::ioctlsocket(m_tcp, FIONBIO, &nonblock);
			::ioctlsocket(m_udp, FIONBIO, &nonblock);

			m_bound_ep = ep;
			return true;
		}

		void stop()
		{
			for (auto& [sock, ctx] : m_clients)
				::closesocket(sock);
			m_clients.clear();

			if (m_tcp != INVALID_SOCKET) { ::closesocket(m_tcp); m_tcp = INVALID_SOCKET; }
			if (m_udp != INVALID_SOCKET) { ::closesocket(m_udp); m_udp = INVALID_SOCKET; }
		}

		// ── tick() ────────────────────────────────────────────────────────────
		// Call from your application loop. Drives all I/O without blocking.

		void tick()
		{
			if (m_tcp == INVALID_SOCKET)
				return;

			fd_set read_set;
			FD_ZERO(&read_set);
			FD_SET(m_tcp, &read_set);
			FD_SET(m_udp, &read_set);

			SOCKET max_fd = std::max(m_tcp, m_udp);
			for (auto& [sock, unused_ctx] : m_clients)
			{
				FD_SET(sock, &read_set);
				max_fd = std::max(max_fd, sock);
			}

			timeval tv{0, 0}; // non-blocking poll
			const int ready = ::select(0, &read_set, nullptr, nullptr, &tv);
			if (ready <= 0)
				return;

			// Accept new TCP connections
			if (FD_ISSET(m_tcp, &read_set))
				accept_client();

			// Read UDP datagrams
			if (FD_ISSET(m_udp, &read_set))
				read_udp();

			// Read from existing TCP clients
			std::vector<SOCKET> to_close;
			for (auto& [sock, ctx] : m_clients)
			{
				if (FD_ISSET(sock, &read_set))
				{
					if (not read_tcp(sock, ctx))
						to_close.push_back(sock);
				}
			}

			for (auto sock : to_close)
				disconnect_client(sock);
		}

		// ── send_to ───────────────────────────────────────────────────────────

		bool send_to(const uuid::v4::uuid& id, const message& msg)
		{
			for (auto& [sock, ctx] : m_clients)
			{
				if (ctx.session_id.ab == id.ab and ctx.session_id.cd == id.cd)
					return send_frame(sock, msg);
			}
			return false;
		}

		// ── broadcast ─────────────────────────────────────────────────────────

		void broadcast(const message& msg)
		{
			for (auto& [sock, ctx] : m_clients)
			{
				if (ctx.authenticated)
					send_frame(sock, msg);
			}
		}

		// ── send_realtime (UDP) ───────────────────────────────────────────────

		bool send_realtime_to(const endpoint& target, u16 stream_id, std::span<const u8> data)
		{
			if (m_udp == INVALID_SOCKET)
				return false;

			realtime_payload rtp;
			rtp.stream_id = stream_id;
			rtp.timestamp = as<u64>(
			  std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch())
			  .count());
			rtp.data.assign(data.begin(), data.end());

			auto msg    = make_message(msg_type::realtime, rtp.to_bytes());
			auto frame  = encode(msg);

			auto [storage, addrlen] = target.address.to_sockaddr();
			if (target.address.is_ipv6())
				reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(target.port);
			else
				reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(target.port);

			return ::sendto(
					 m_udp,
					 reinterpret_cast<const char*>(frame.data()),
					 as<int>(frame.size()),
					 0,
					 reinterpret_cast<sockaddr*>(&storage),
					 as<int>(addrlen)) != SOCKET_ERROR;
		}

	private:
		SOCKET     m_tcp{INVALID_SOCKET};
		SOCKET     m_udp{INVALID_SOCKET};
		endpoint   m_bound_ep{};
		u8         m_pow_difficulty{16};

		std::unordered_map<SOCKET, client_context> m_clients;

		connect_fn    m_on_connect;
		disconnect_fn m_on_disconnect;
		message_fn    m_on_message;

		// ── accept_client ─────────────────────────────────────────────────────

		void accept_client()
		{
			sockaddr_storage storage{};
			socklen_t        addrlen = sizeof(storage);
			SOCKET           ns     = ::accept(m_tcp, reinterpret_cast<sockaddr*>(&storage), &addrlen);
			if (ns == INVALID_SOCKET)
				return;

			u_long nonblock = 1;
			::ioctlsocket(ns, FIONBIO, &nonblock);

			client_context ctx;
			ctx.tcp_socket   = ns;
			ctx.remote       = sockaddr_to_endpoint(storage);
			ctx.session_id   = uuid::v4::generate();
			ctx.authenticated = false;
			ctx.challenge    = generate_challenge(m_pow_difficulty);

			// Send PoW challenge immediately
			auto msg = make_challenge_message(ctx.challenge, ctx.next_sequence++);
			send_frame(ns, msg);

			m_clients.emplace(ns, std::move(ctx));
		}

		// ── read_tcp ──────────────────────────────────────────────────────────

		bool read_tcp(SOCKET sock, client_context& ctx)
		{
			std::array<u8, 4096> tmp{};
			const int            n = ::recv(sock, reinterpret_cast<char*>(tmp.data()), as<int>(tmp.size()), 0);

			if (n <= 0)
				return false; // connection closed or error

			ctx.recv_buf.insert(ctx.recv_buf.end(), tmp.begin(), tmp.begin() + n);

			// Drain complete frames
			while (true)
			{
				if (ctx.recv_buf.size() < MIN_FRAME_SIZE)
					break;

				// Peek payload_len (offset 5, 4 bytes LE)
				const u32 plen =
				  as<u32>(ctx.recv_buf[5])
				  | (as<u32>(ctx.recv_buf[6]) << 8)
				  | (as<u32>(ctx.recv_buf[7]) << 16)
				  | (as<u32>(ctx.recv_buf[8]) << 24);

				if (plen > MAX_PAYLOAD_BYTES)
					return false; // abort connection

				const size_t frame_size = HEADER_SIZE + plen + DIGEST_SIZE;
				if (ctx.recv_buf.size() < frame_size)
					break;

				auto frame_span = std::span<const u8>{ctx.recv_buf.data(), frame_size};
				auto result     = decode(frame_span);

				ctx.recv_buf.erase(ctx.recv_buf.begin(), ctx.recv_buf.begin() + as<std::ptrdiff_t>(frame_size));

				if (not result)
					continue; // bad frame — skip

				dispatch_tcp(sock, ctx, *result);
			}

			return true;
		}

		// ── dispatch_tcp ──────────────────────────────────────────────────────

		void dispatch_tcp(SOCKET sock, client_context& ctx, const message& msg)
		{
			if (not ctx.authenticated)
			{
				// Only accept handshake_response before auth
				if (msg.header.type != msg_type::handshake_response)
					return;

				auto resp = handshake_response_payload::from_bytes(msg.payload);
				if (not resp)
				{
					send_frame(sock, make_message(msg_type::handshake_reject, {}, ctx.next_sequence++));
					return;
				}

				pow_response pr;
				pr.nonce   = resp->nonce;
				pr.counter = resp->counter;

				if (not verify_challenge(ctx.challenge, pr))
				{
					send_frame(sock, make_message(msg_type::handshake_reject, {}, ctx.next_sequence++));
					return;
				}

				ctx.authenticated = true;

				// Build accept payload: session UUID as 16 bytes
				handshake_accept_payload ap;
				const u64 ab = ctx.session_id.ab;
				const u64 cd = ctx.session_id.cd;
				for (int i = 0; i < 8; ++i) ap.session_id[i]     = as<u8>((ab >> (i * 8)) & 0xFF);
				for (int i = 0; i < 8; ++i) ap.session_id[8 + i] = as<u8>((cd >> (i * 8)) & 0xFF);

				send_frame(sock, make_message(msg_type::handshake_accept, ap.to_bytes(), ctx.next_sequence++));

				if (m_on_connect)
					m_on_connect(ctx.session_id, ctx.remote);
				return;
			}

			// Disconnect request
			if (msg.header.type == msg_type::disconnect)
			{
				return; // read_tcp will return false on next recv
			}

			// Ping → pong
			if (msg.header.type == msg_type::ping)
			{
				send_frame(sock, make_message(msg_type::pong, msg.payload, ctx.next_sequence++));
				return;
			}

			if (m_on_message)
				m_on_message(ctx.session_id, msg);
		}

		// ── read_udp ──────────────────────────────────────────────────────────

		void read_udp()
		{
			std::array<u8, MAX_PAYLOAD_BYTES + MIN_FRAME_SIZE> buf{};
			sockaddr_storage storage{};
			socklen_t        addrlen = sizeof(storage);

			const int n = ::recvfrom(
			  m_udp,
			  reinterpret_cast<char*>(buf.data()),
			  as<int>(buf.size()),
			  0,
			  reinterpret_cast<sockaddr*>(&storage),
			  &addrlen);

			if (n <= 0)
				return;

			auto result = decode(std::span<const u8>{buf.data(), as<size_t>(n)});
			if (not result)
				return;

			if (m_on_message)
			{
				// Match UDP sender to a known authenticated client by IP/port
				auto sender = sockaddr_to_endpoint(storage);
				for (auto& [unused_sock, ctx] : m_clients)
				{
					if (ctx.authenticated and ctx.remote.address == sender.address)
					{
						m_on_message(ctx.session_id, *result);
						break;
					}
				}
			}
		}

		// ── disconnect_client ─────────────────────────────────────────────────

		void disconnect_client(SOCKET sock)
		{
			auto it = m_clients.find(sock);
			if (it == m_clients.end())
				return;

			if (it->second.authenticated and m_on_disconnect)
				m_on_disconnect(it->second.session_id);

			::closesocket(sock);
			m_clients.erase(it);
		}

		// ── send_frame ────────────────────────────────────────────────────────

		bool send_frame(SOCKET sock, const message& msg)
		{
			auto frame = encode(msg);
			int  sent  = 0;
			int  total = as<int>(frame.size());
			const char* ptr = reinterpret_cast<const char*>(frame.data());

			while (sent < total)
			{
				int n = ::send(sock, ptr + sent, total - sent, 0);
				if (n == SOCKET_ERROR)
					return false;
				sent += n;
			}
			return true;
		}
	};

} // namespace deckard::net
