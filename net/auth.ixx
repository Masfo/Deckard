export module deckard.net:auth;

import :protocol;
import deckard.types;
import deckard.as;
import deckard.sha;
import deckard.random;
import deckard.helpers;

import std;

// Proof-of-Work authentication
//
// Purpose: anti-spam / anti-DoS connection throttle. NOT a cryptographic
// authentication scheme — no secrets are exchanged in the handshake.
//
// Protocol:
//   1. Server sends pow_challenge  (32-byte random nonce + difficulty in bits).
//   2. Client brute-forces a u32 counter such that:
//        SHA-256(nonce[32] || LE32(counter))
//      has at least `difficulty` leading zero bits.
//   3. Server verifies by recomputing the same hash.
//
// Default difficulty 16 requires ~65 536 SHA-256 calls on average.

namespace deckard::net
{
	export struct pow_challenge
	{
		std::array<u8, 32> nonce{};
		u8                 difficulty{16}; // leading zero bits required
	};

	export struct pow_response
	{
		std::array<u8, 32> nonce{};   // echo of the challenge nonce
		u32                counter{}; // winning counter value
	};

	namespace detail
	{
		// Write v as 4 LE bytes into buf starting at offset 0.
		inline std::array<u8, 4> le32_bytes(u32 v)
		{
			return {
			  as<u8>(v & 0xFF),
			  as<u8>((v >> 8) & 0xFF),
			  as<u8>((v >> 16) & 0xFF),
			  as<u8>((v >> 24) & 0xFF),
			};
		}

		// Count the number of leading zero bits in a SHA-256 digest.
		inline u32 leading_zero_bits(const sha256::digest& d)
		{
			u32 count = 0;
			for (auto byte : d.data())
			{
				const auto lz = std::countl_zero(byte);
				count += lz;
				if (lz < 8)
					break;
			}
			return count;
		}

		inline sha256::digest pow_hash(std::span<const u8, 32> nonce, u32 counter)
		{
			auto           counter_bytes = le32_bytes(counter);
			sha256::hasher h;
			std::array<u8, 32> nonce_copy;
			std::copy(nonce.begin(), nonce.end(), nonce_copy.begin());
			h.update(nonce_copy);
			h.update(counter_bytes);
			return h.finalize();
		}
	} // namespace detail

	// Generate a fresh challenge. Uses deckard.random (pcg32) to fill the nonce.
	export [[nodiscard]] pow_challenge generate_challenge(u8 difficulty = 16)
	{
		pow_challenge c;
		c.difficulty = difficulty;
		// Fill 32 bytes using 8 × u32 from pcg::rand32()
		for (int i = 0; i < 8; ++i)
		{
			const u32 r = random::pcg::rand32();

			write_le(c.nonce, i * 4, r);
			//c.nonce[i * 4 + 0] = as<u8>(r & 0xFF);
			//c.nonce[i * 4 + 1] = as<u8>((r >> 8) & 0xFF);
			//c.nonce[i * 4 + 2] = as<u8>((r >> 16) & 0xFF);
			//c.nonce[i * 4 + 3] = as<u8>((r >> 24) & 0xFF);
		}
		return c;
	}

	// Brute-force a counter satisfying the challenge. Returns std::nullopt on
	// exhaustion (counter wraps past UINT32_MAX without a solution — astronomically
	// unlikely for difficulty <= 30).
	export [[nodiscard]] std::optional<pow_response> solve_challenge(const pow_challenge& challenge)
	{
		auto nonce_span = std::span<const u8, 32>{challenge.nonce};

		for (u32 counter = 0; ; ++counter)
		{
			auto digest = detail::pow_hash(nonce_span, counter);

			if (detail::leading_zero_bits(digest) >= challenge.difficulty)
			{
				pow_response r;
				r.nonce   = challenge.nonce;
				r.counter = counter;
				return r;
			}

			if (counter == std::numeric_limits<u32>::max())
				break;
		}
		return std::nullopt;
	}

	// Verify that a response satisfies the original challenge.
	export [[nodiscard]] bool verify_challenge(const pow_challenge& challenge, const pow_response& response)
	{
		// Nonce must match
		if (response.nonce != challenge.nonce)
			return false;

		auto nonce_span = std::span<const u8, 32>{response.nonce};
		auto digest     = detail::pow_hash(nonce_span, response.counter);
		return detail::leading_zero_bits(digest) >= challenge.difficulty;
	}

	// ── Message helpers ───────────────────────────────────────────────────────
	// Build ready-to-encode protocol messages from auth payloads.

	export [[nodiscard]] message make_challenge_message(const pow_challenge& c, u32 seq = 0)
	{
		handshake_challenge_payload p;
		p.nonce      = c.nonce;
		p.difficulty = c.difficulty;
		return make_message(msg_type::handshake_challenge, p.to_bytes(), seq);
	}

	export [[nodiscard]] message make_response_message(const pow_response& r, u32 seq = 0)
	{
		handshake_response_payload p;
		p.nonce   = r.nonce;
		p.counter = r.counter;
		return make_message(msg_type::handshake_response, p.to_bytes(), seq);
	}

} // namespace deckard::net
