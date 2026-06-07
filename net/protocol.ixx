module;
#include <winsock2.h>

export module deckard.net:protocol;

import std;
import deckard.types;
import deckard.as;
import deckard.sha;

namespace deckard::net
{
	// ── Wire format ───────────────────────────────────────────────────────────
	// [magic:       2 bytes = 0xDE 0xCA          ]
	// [version:     1 byte  = 0x01               ]
	// [msg_type:    1 byte                        ]
	// [flags:       1 byte                        ]
	// [payload_len: 4 bytes, u32 LE               ]
	// [sequence:    4 bytes, u32 LE               ]
	// [payload:     payload_len bytes             ]
	// [sha256:      32 bytes, over all bytes above]
	//
	// Total fixed overhead: 13 header + 32 digest = 45 bytes.
	// Max payload: 64 KiB.

	export constexpr u8  PROTOCOL_VERSION  = 0x01;
	export constexpr u16 PROTOCOL_MAGIC    = 0xDECA;
	export constexpr u32 MAX_PAYLOAD_BYTES = 64u * 1024u;
	export constexpr u32 HEADER_SIZE       = 13u; // magic(2)+ver(1)+type(1)+flags(1)+len(4)+seq(4)
	export constexpr u32 DIGEST_SIZE       = 32u;
	export constexpr u32 MIN_FRAME_SIZE    = HEADER_SIZE + DIGEST_SIZE;

	// ── Message type ──────────────────────────────────────────────────────────

	export enum class msg_type : u8
	{
		// Handshake
		handshake_challenge  = 0x01,
		handshake_response   = 0x02,
		handshake_accept     = 0x03,
		handshake_reject     = 0x04,

		// Chat
		chat     = 0x10,
		chat_ack = 0x11,

		// File transfer
		file_begin = 0x20,
		file_chunk = 0x21,
		file_end   = 0x22,
		file_ack   = 0x23,

		// Realtime data (bidirectional)
		realtime             = 0x30,
		realtime_subscribe   = 0x31,
		realtime_unsubscribe = 0x32,

		// Control
		ping       = 0xF0,
		pong       = 0xF1,
		disconnect = 0xFF,
	};

	// ── Flags bitmask ─────────────────────────────────────────────────────────

	export enum class msg_flags : u8
	{
		none      = 0x00,
		compressed = 0x01,
		encrypted  = 0x02,
		fragmented = 0x04,
		last_frag  = 0x08,
	};
	consteval void enable_bitmask_operations(msg_flags);

	export constexpr msg_flags operator|(msg_flags a, msg_flags b)
	{
		return static_cast<msg_flags>(static_cast<u8>(a) | static_cast<u8>(b));
	}
	export constexpr msg_flags operator&(msg_flags a, msg_flags b)
	{
		return static_cast<msg_flags>(static_cast<u8>(a) & static_cast<u8>(b));
	}
	export constexpr bool has_flag(msg_flags set, msg_flags flag) { return (set & flag) == flag; }

	// ── Frame header (unpacked representation) ────────────────────────────────

	export struct message_header
	{
		u16       magic{PROTOCOL_MAGIC};
		u8        version{PROTOCOL_VERSION};
		msg_type  type{};
		msg_flags flags{msg_flags::none};
		u32       payload_len{};
		u32       sequence{};
	};
	// Logical size; wire encoding is explicit LE — no #pragma pack needed.

	// ── Full message ──────────────────────────────────────────────────────────

	export struct message
	{
		message_header   header{};
		std::vector<u8>  payload{};
	};

	// ── LE helpers ────────────────────────────────────────────────────────────

	namespace detail
	{
		inline void write_le16(std::vector<u8>& out, u16 v)
		{
			out.push_back(as<u8>(v & 0xFF));
			out.push_back(as<u8>((v >> 8) & 0xFF));
		}

		inline void write_le32(std::vector<u8>& out, u32 v)
		{
			out.push_back(as<u8>(v & 0xFF));
			out.push_back(as<u8>((v >> 8) & 0xFF));
			out.push_back(as<u8>((v >> 16) & 0xFF));
			out.push_back(as<u8>((v >> 24) & 0xFF));
		}

		inline void write_le64(std::vector<u8>& out, u64 v)
		{
			for (int i = 0; i < 8; ++i)
				out.push_back(as<u8>((v >> (i * 8)) & 0xFF));
		}

		inline u16 read_le16(std::span<const u8> s, size_t off)
		{
			return as<u16>(s[off]) | (as<u16>(s[off + 1]) << 8);
		}

		inline u32 read_le32(std::span<const u8> s, size_t off)
		{
			return as<u32>(s[off])
				 | (as<u32>(s[off + 1]) << 8)
				 | (as<u32>(s[off + 2]) << 16)
				 | (as<u32>(s[off + 3]) << 24);
		}

		inline u64 read_le64(std::span<const u8> s, size_t off)
		{
			u64 v = 0;
			for (int i = 0; i < 8; ++i)
				v |= (as<u64>(s[off + i]) << (i * 8));
			return v;
		}
	} // namespace detail

	// ── encode / decode ───────────────────────────────────────────────────────

	export [[nodiscard]] std::vector<u8> encode(const message& msg)
	{
		const u32 plen = as<u32>(msg.payload.size());

		std::vector<u8> frame;
		frame.reserve(HEADER_SIZE + plen + DIGEST_SIZE);

		detail::write_le16(frame, msg.header.magic);
		frame.push_back(msg.header.version);
		frame.push_back(as<u8>(msg.header.type));
		frame.push_back(as<u8>(msg.header.flags));
		detail::write_le32(frame, plen);
		detail::write_le32(frame, msg.header.sequence);

		for (auto b : msg.payload)
			frame.push_back(b);

		// SHA-256 over [magic..payload]
		sha256::hasher h;
		auto           span = std::span<const u8>{frame.data(), frame.size()};
		h.update(span);
		auto digest = h.finalize();
		for (auto b : digest.data())
			frame.push_back(b);

		return frame;
	}

	export [[nodiscard]] std::expected<message, std::string> decode(std::span<const u8> raw)
	{
		if (raw.size() < MIN_FRAME_SIZE)
			return std::unexpected(std::format("Frame too small: {} bytes", raw.size()));

		const u16 magic = detail::read_le16(raw, 0);
		if (magic != PROTOCOL_MAGIC)
			return std::unexpected(std::format("Bad magic: 0x{:04X}", magic));

		const u8 version = raw[2];
		if (version != PROTOCOL_VERSION)
			return std::unexpected(std::format("Unknown protocol version: {}", version));

		const u32 plen = detail::read_le32(raw, 5);
		if (plen > MAX_PAYLOAD_BYTES)
			return std::unexpected(std::format("Payload too large: {} bytes", plen));

		const size_t expected_total = HEADER_SIZE + plen + DIGEST_SIZE;
		if (raw.size() < expected_total)
			return std::unexpected(std::format("Truncated frame: have {} need {}", raw.size(), expected_total));

		// Verify SHA-256 trailer
		const size_t digest_offset = HEADER_SIZE + plen;
		sha256::hasher h;
		auto           covered = std::span<const u8>{raw.data(), digest_offset};
		h.update(covered);
		auto computed = h.finalize();

		for (size_t i = 0; i < DIGEST_SIZE; ++i)
		{
			if (raw[digest_offset + i] != computed.data()[i])
				return std::unexpected("SHA-256 digest mismatch");
		}

		message msg;
		msg.header.magic       = magic;
		msg.header.version     = version;
		msg.header.type        = static_cast<msg_type>(raw[3]);
		msg.header.flags       = static_cast<msg_flags>(raw[4]);
		msg.header.payload_len = plen;
		msg.header.sequence    = detail::read_le32(raw, 9);

		msg.payload.assign(raw.data() + HEADER_SIZE, raw.data() + HEADER_SIZE + plen);

		return msg;
	}

	// ── Payload structs ───────────────────────────────────────────────────────
	// Each provides to_bytes() and from_bytes(). All integers LE.

	// handshake_challenge: nonce[32] + difficulty u8
	export struct handshake_challenge_payload
	{
		std::array<u8, 32> nonce{};
		u8                 difficulty{16};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(33);
			out.insert(out.end(), nonce.begin(), nonce.end());
			out.push_back(difficulty);
			return out;
		}

		static std::expected<handshake_challenge_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 33)
				return std::unexpected("handshake_challenge: truncated");
			handshake_challenge_payload p;
			std::copy(s.begin(), s.begin() + 32, p.nonce.begin());
			p.difficulty = s[32];
			return p;
		}
	};

	// handshake_response: nonce[32] + counter u32 LE
	export struct handshake_response_payload
	{
		std::array<u8, 32> nonce{};
		u32                counter{};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(36);
			out.insert(out.end(), nonce.begin(), nonce.end());
			detail::write_le32(out, counter);
			return out;
		}

		static std::expected<handshake_response_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 36)
				return std::unexpected("handshake_response: truncated");
			handshake_response_payload p;
			std::copy(s.begin(), s.begin() + 32, p.nonce.begin());
			p.counter = detail::read_le32(s, 32);
			return p;
		}
	};

	// handshake_accept: session_id[16] (UUID bytes)
	export struct handshake_accept_payload
	{
		std::array<u8, 16> session_id{};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			return std::vector<u8>(session_id.begin(), session_id.end());
		}

		static std::expected<handshake_accept_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 16)
				return std::unexpected("handshake_accept: truncated");
			handshake_accept_payload p;
			std::copy(s.begin(), s.begin() + 16, p.session_id.begin());
			return p;
		}
	};

	// chat: u16 len + UTF-8 text
	export struct chat_payload
	{
		std::string text;

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(2 + text.size());
			detail::write_le16(out, as<u16>(std::min(text.size(), size_t{0xFFFF})));
			out.insert(out.end(), text.begin(), text.end());
			return out;
		}

		static std::expected<chat_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 2)
				return std::unexpected("chat: truncated header");
			const u16 len = detail::read_le16(s, 0);
			if (s.size() < size_t{2} + len)
				return std::unexpected("chat: truncated text");
			chat_payload p;
			p.text.assign(reinterpret_cast<const char*>(s.data() + 2), len);
			return p;
		}
	};

	// chat_ack: sequence u32 LE
	export struct chat_ack_payload
	{
		u32 sequence{};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			detail::write_le32(out, sequence);
			return out;
		}

		static std::expected<chat_ack_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 4)
				return std::unexpected("chat_ack: truncated");
			return chat_ack_payload{detail::read_le32(s, 0)};
		}
	};

	// file_begin: file_id u32 + total_size u64 + chunk_count u32 + filename (u16 len + bytes)
	export struct file_begin_payload
	{
		u32         file_id{};
		u64         total_size{};
		u32         chunk_count{};
		std::string filename;

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(18 + filename.size());
			detail::write_le32(out, file_id);
			detail::write_le64(out, total_size);
			detail::write_le32(out, chunk_count);
			detail::write_le16(out, as<u16>(std::min(filename.size(), size_t{0xFFFF})));
			out.insert(out.end(), filename.begin(), filename.end());
			return out;
		}

		static std::expected<file_begin_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 18)
				return std::unexpected("file_begin: truncated header");
			file_begin_payload p;
			p.file_id     = detail::read_le32(s, 0);
			p.total_size  = detail::read_le64(s, 4);
			p.chunk_count = detail::read_le32(s, 12);
			const u16 nlen = detail::read_le16(s, 16);
			if (s.size() < size_t{18} + nlen)
				return std::unexpected("file_begin: truncated filename");
			p.filename.assign(reinterpret_cast<const char*>(s.data() + 18), nlen);
			return p;
		}
	};

	// file_chunk: file_id u32 + chunk_index u32 + data (u16 len + bytes)
	export struct file_chunk_payload
	{
		u32              file_id{};
		u32              chunk_index{};
		std::vector<u8>  data;

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(10 + data.size());
			detail::write_le32(out, file_id);
			detail::write_le32(out, chunk_index);
			detail::write_le16(out, as<u16>(std::min(data.size(), size_t{0xFFFF})));
			out.insert(out.end(), data.begin(), data.end());
			return out;
		}

		static std::expected<file_chunk_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 10)
				return std::unexpected("file_chunk: truncated header");
			file_chunk_payload p;
			p.file_id     = detail::read_le32(s, 0);
			p.chunk_index = detail::read_le32(s, 4);
			const u16 dlen = detail::read_le16(s, 8);
			if (s.size() < size_t{10} + dlen)
				return std::unexpected("file_chunk: truncated data");
			p.data.assign(s.data() + 10, s.data() + 10 + dlen);
			return p;
		}
	};

	// file_end: file_id u32 + sha256[32] of full file
	export struct file_end_payload
	{
		u32                file_id{};
		std::array<u8, 32> file_sha256{};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(36);
			detail::write_le32(out, file_id);
			out.insert(out.end(), file_sha256.begin(), file_sha256.end());
			return out;
		}

		static std::expected<file_end_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 36)
				return std::unexpected("file_end: truncated");
			file_end_payload p;
			p.file_id = detail::read_le32(s, 0);
			std::copy(s.begin() + 4, s.begin() + 36, p.file_sha256.begin());
			return p;
		}
	};

	// file_ack: file_id u32 + chunk_index u32 (0xFFFFFFFF = transfer complete)
	export struct file_ack_payload
	{
		u32 file_id{};
		u32 chunk_index{};

		static constexpr u32 COMPLETE = 0xFFFF'FFFFu;

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			detail::write_le32(out, file_id);
			detail::write_le32(out, chunk_index);
			return out;
		}

		static std::expected<file_ack_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 8)
				return std::unexpected("file_ack: truncated");
			return file_ack_payload{detail::read_le32(s, 0), detail::read_le32(s, 4)};
		}
	};

	// realtime: stream_id u16 + timestamp u64 + data (u16 len + bytes)
	export struct realtime_payload
	{
		u16              stream_id{};
		u64              timestamp{};
		std::vector<u8>  data;

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			out.reserve(12 + data.size());
			detail::write_le16(out, stream_id);
			detail::write_le64(out, timestamp);
			detail::write_le16(out, as<u16>(std::min(data.size(), size_t{0xFFFF})));
			out.insert(out.end(), data.begin(), data.end());
			return out;
		}

		static std::expected<realtime_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 12)
				return std::unexpected("realtime: truncated header");
			realtime_payload p;
			p.stream_id = detail::read_le16(s, 0);
			p.timestamp = detail::read_le64(s, 2);
			const u16 dlen = detail::read_le16(s, 10);
			if (s.size() < size_t{12} + dlen)
				return std::unexpected("realtime: truncated data");
			p.data.assign(s.data() + 12, s.data() + 12 + dlen);
			return p;
		}
	};

	// realtime_subscribe / realtime_unsubscribe: stream_id u16
	export struct realtime_subscribe_payload
	{
		u16 stream_id{};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			detail::write_le16(out, stream_id);
			return out;
		}

		static std::expected<realtime_subscribe_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 2)
				return std::unexpected("realtime_subscribe: truncated");
			return realtime_subscribe_payload{detail::read_le16(s, 0)};
		}
	};

	// ping / pong: timestamp u64 LE
	export struct ping_payload
	{
		u64 timestamp{};

		[[nodiscard]] std::vector<u8> to_bytes() const
		{
			std::vector<u8> out;
			detail::write_le64(out, timestamp);
			return out;
		}

		static std::expected<ping_payload, std::string> from_bytes(std::span<const u8> s)
		{
			if (s.size() < 8)
				return std::unexpected("ping: truncated");
			return ping_payload{detail::read_le64(s, 0)};
		}
	};

	// ── Factory helpers ───────────────────────────────────────────────────────

	export message make_message(msg_type type, std::vector<u8> payload, u32 sequence = 0, msg_flags flags = msg_flags::none)
	{
		message msg;
		msg.header.type        = type;
		msg.header.flags       = flags;
		msg.header.sequence    = sequence;
		msg.header.payload_len = as<u32>(payload.size());
		msg.payload            = std::move(payload);
		return msg;
	}

} // namespace deckard::net
