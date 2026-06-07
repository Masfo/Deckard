module;
#include <winsock2.h>
#include <ws2tcpip.h>

export module deckard.net:client;

import :protocol;
import :auth;
import :ip;

import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.scope_exit;

import std;

namespace deckard::net
{
	export enum class client_state : u8
	{
		idle,
		connecting,
		authenticating, // PoW solve in progress
		connected,
		disconnected,
	};

	export class client
	{
	public:
		using connect_fn    = std::function<void()>;
		using disconnect_fn = std::function<void()>;
		using message_fn    = std::function<void(const message&)>;

		client()  = default;
		~client() { disconnect(); }

		client(const client&)            = delete;
		client& operator=(const client&) = delete;

		// ── Configuration ─────────────────────────────────────────────────────

		void on_connect(connect_fn fn)       { m_on_connect    = std::move(fn); }
		void on_disconnect(disconnect_fn fn) { m_on_disconnect = std::move(fn); }
		void on_message(message_fn fn)       { m_on_message    = std::move(fn); }

		// ── connect (TCP + PoW handshake) ─────────────────────────────────────

		bool connect(const endpoint& ep)
		{
			if (m_state != client_state::idle and m_state != client_state::disconnected)
				return false;

			auto [storage, addrlen] = ep.address.to_sockaddr();
			if (ep.address.is_ipv6())
				reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(ep.port);
			else
				reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(ep.port);

			m_tcp = ::socket(storage.ss_family, SOCK_STREAM, IPPROTO_TCP);
			if (m_tcp == INVALID_SOCKET)
			{
				dbg::eprintln("client::connect socket failed");
				return false;
			}

			// Non-blocking connect
			u_long nonblock = 1;
			::ioctlsocket(m_tcp, FIONBIO, &nonblock);

			::connect(m_tcp, reinterpret_cast<sockaddr*>(&storage), as<int>(addrlen));
			// WSAEWOULDBLOCK is expected — connection is in progress

			// Wait up to 5 s for TCP connect to complete
			timeval tv{5, 0};
			fd_set wset, eset;
			FD_ZERO(&wset); FD_SET(m_tcp, &wset);
			FD_ZERO(&eset); FD_SET(m_tcp, &eset);

			const int ready = ::select(0, nullptr, &wset, &eset, &tv);
			if (ready <= 0 or FD_ISSET(m_tcp, &eset))
			{
				dbg::eprintln("client::connect TCP connect timed out or failed");
				::closesocket(m_tcp);
				m_tcp  = INVALID_SOCKET;
				m_state = client_state::disconnected;
				return false;
			}

			m_server_ep = ep;
			m_state     = client_state::authenticating;
			m_seq       = 0;
			m_recv_buf.clear();
			// PoW challenge will arrive from server; handled in tick()
			return true;
		}

		bool connect_udp(const endpoint& ep)
		{
			if (m_udp != INVALID_SOCKET)
				return true;

			auto [storage, addrlen] = ep.address.to_sockaddr();
			if (ep.address.is_ipv6())
				reinterpret_cast<sockaddr_in6&>(storage).sin6_port = htons(ep.port);
			else
				reinterpret_cast<sockaddr_in&>(storage).sin_port = htons(ep.port);

			m_udp = ::socket(storage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
			if (m_udp == INVALID_SOCKET)
				return false;

			if (::connect(m_udp, reinterpret_cast<sockaddr*>(&storage), as<int>(addrlen)) == SOCKET_ERROR)
			{
				::closesocket(m_udp);
				m_udp = INVALID_SOCKET;
				return false;
			}

			u_long nonblock = 1;
			::ioctlsocket(m_udp, FIONBIO, &nonblock);

			m_udp_ep = ep;
			return true;
		}

		void disconnect()
		{
			if (m_tcp != INVALID_SOCKET)
			{
				if (m_state == client_state::connected)
					send(make_message(msg_type::disconnect, {}, m_seq++));

				::closesocket(m_tcp);
				m_tcp = INVALID_SOCKET;
			}
			if (m_udp != INVALID_SOCKET)
			{
				::closesocket(m_udp);
				m_udp = INVALID_SOCKET;
			}

			if (m_state == client_state::connected and m_on_disconnect)
				m_on_disconnect();

			m_state = client_state::disconnected;
			m_recv_buf.clear();
			m_pending_challenge.reset();
		}

		client_state state() const { return m_state; }

		// ── tick() ────────────────────────────────────────────────────────────
		// Must be called regularly from the application loop.

		void tick()
		{
			if (m_tcp == INVALID_SOCKET)
				return;

			fd_set read_set, except_set;
			FD_ZERO(&read_set);   FD_SET(m_tcp, &read_set);
			FD_ZERO(&except_set); FD_SET(m_tcp, &except_set);

			timeval tv{0, 0};
			const int ready = ::select(0, &read_set, nullptr, &except_set, &tv);

			if (ready < 0 or FD_ISSET(m_tcp, &except_set))
			{
				disconnect();
				return;
			}

			if (ready > 0 and FD_ISSET(m_tcp, &read_set))
			{
				if (not read_tcp())
				{
					disconnect();
					return;
				}
			}
		}

		// ── send (TCP) ────────────────────────────────────────────────────────

		bool send(const message& msg)
		{
			if (m_tcp == INVALID_SOCKET)
				return false;

			auto        frame = encode(msg);
			int         sent  = 0;
			const int   total = as<int>(frame.size());
			const char* ptr   = reinterpret_cast<const char*>(frame.data());

			while (sent < total)
			{
				int n = ::send(m_tcp, ptr + sent, total - sent, 0);
				if (n == SOCKET_ERROR)
					return false;
				sent += n;
			}
			return true;
		}

		// ── send_realtime (UDP) ───────────────────────────────────────────────

		bool send_realtime(u16 stream_id, std::span<const u8> data)
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

			auto        frame = encode(make_message(msg_type::realtime, rtp.to_bytes(), m_seq++));
			const char* ptr   = reinterpret_cast<const char*>(frame.data());

			return ::send(m_udp, ptr, as<int>(frame.size()), 0) != SOCKET_ERROR;
		}

		// ── subscribe / unsubscribe (UDP control via TCP) ─────────────────────

		bool subscribe_stream(u16 stream_id)
		{
			realtime_subscribe_payload p{stream_id};
			return send(make_message(msg_type::realtime_subscribe, p.to_bytes(), m_seq++));
		}

		bool unsubscribe_stream(u16 stream_id)
		{
			realtime_subscribe_payload p{stream_id};
			return send(make_message(msg_type::realtime_unsubscribe, p.to_bytes(), m_seq++));
		}

		// ── ping ──────────────────────────────────────────────────────────────

		bool ping()
		{
			if (m_state != client_state::connected)
				return false;
			ping_payload p;
			p.timestamp = as<u64>(
			  std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch())
			  .count());
			return send(make_message(msg_type::ping, p.to_bytes(), m_seq++));
		}

	private:
		SOCKET                    m_tcp{INVALID_SOCKET};
		SOCKET                    m_udp{INVALID_SOCKET};
		endpoint                  m_server_ep{};
		endpoint                  m_udp_ep{};
		client_state              m_state{client_state::idle};
		u32                       m_seq{};
		std::vector<u8>           m_recv_buf;
		std::optional<pow_challenge> m_pending_challenge;

		connect_fn    m_on_connect;
		disconnect_fn m_on_disconnect;
		message_fn    m_on_message;

		// ── read_tcp ──────────────────────────────────────────────────────────

		bool read_tcp()
		{
			std::array<u8, 4096> tmp{};
			const int n = ::recv(m_tcp, reinterpret_cast<char*>(tmp.data()), as<int>(tmp.size()), 0);

			if (n <= 0)
				return false;

			m_recv_buf.insert(m_recv_buf.end(), tmp.begin(), tmp.begin() + n);

			while (true)
			{
				if (m_recv_buf.size() < MIN_FRAME_SIZE)
					break;

				const u32 plen =
				  as<u32>(m_recv_buf[5])
				  | (as<u32>(m_recv_buf[6]) << 8)
				  | (as<u32>(m_recv_buf[7]) << 16)
				  | (as<u32>(m_recv_buf[8]) << 24);

				if (plen > MAX_PAYLOAD_BYTES)
					return false;

				const size_t frame_size = HEADER_SIZE + plen + DIGEST_SIZE;
				if (m_recv_buf.size() < frame_size)
					break;

				auto span   = std::span<const u8>{m_recv_buf.data(), frame_size};
				auto result = decode(span);

				m_recv_buf.erase(m_recv_buf.begin(), m_recv_buf.begin() + as<std::ptrdiff_t>(frame_size));

				if (not result)
					continue;

				dispatch(*result);
			}

			return true;
		}

		// ── dispatch ──────────────────────────────────────────────────────────

		void dispatch(const message& msg)
		{
			switch (msg.header.type)
			{
				case msg_type::handshake_challenge:
				{
					auto p = handshake_challenge_payload::from_bytes(msg.payload);
					if (not p)
						return;

					// Solve PoW (synchronous — typically <1 ms at difficulty 16)
					pow_challenge c;
					c.nonce      = p->nonce;
					c.difficulty = p->difficulty;
					auto resp = solve_challenge(c);
					if (not resp)
						return;

					send(make_response_message(*resp, m_seq++));
					break;
				}

				case msg_type::handshake_accept:
					m_state = client_state::connected;
					if (m_on_connect)
						m_on_connect();
					break;

				case msg_type::handshake_reject:
					disconnect();
					break;

				case msg_type::ping:
					send(make_message(msg_type::pong, msg.payload, m_seq++));
					break;

				case msg_type::disconnect:
					disconnect();
					break;

				default:
					if (m_on_message and m_state == client_state::connected)
						m_on_message(msg);
					break;
			}
		}
	};

} // namespace deckard::net
