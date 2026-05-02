export module deckard.hmac;

import std;
import deckard.types;
import deckard.as;
import deckard.sha;
import deckard.helpers;

namespace deckard::hmac
{
	template<Hasher Hasher, Digester Digest, u32 BLOCK_SIZE>
	Digest generic_hmac(std::span<const u8> key, std::span<const u8> message)
	{
		std::array<u8, BLOCK_SIZE> key_block{};
		if (key.size() > BLOCK_SIZE)
		{
			Hasher tmp;
			tmp.update(key);
			auto hashed = tmp.finalize();

			std::copy(hashed.data().begin(), hashed.data().end(), key_block.begin());
		}
		else
		{
			std::copy(key.begin(), key.end(), key_block.begin());
		}

		for (auto& byte : key_block)
			byte ^= u8{0x36};

		Hasher inner;
		inner.update(key_block);
		inner.update(message);
		auto inner_digest = inner.finalize();

		for (auto& byte : key_block)
			byte ^= u8{0x36} ^ u8 { 0x5c };

		Hasher outer;
		outer.update(key_block);
		outer.update(inner_digest.data());
		return outer.finalize();
	}

	// SHA1-HMAC
	namespace sha1
	{
		using hasher             = deckard::sha1::hasher;
		using digest             = deckard::sha1::digest;
		constexpr u32 BLOCK_SIZE = 64;

		export auto hash(std::span<const u8> key, std::span<const u8> msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(key, msg);
		}

		export auto hash(std::string_view key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(to_span(key), to_span(msg));
		}

		export auto hash(std::span<const u8> key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(key, to_span(msg));
		}

		export std::string quickhash(std::span<const u8> key, std::span<const u8> msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::span<const u8> key, std::string_view msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::string_view key, std::string_view msg)
		{
			return quickhash(to_span(key), to_span(msg));
		}
	} // namespace sha1

	// HMAC-SHA256
	namespace sha256
	{
		using hasher             = deckard::sha256::hasher;
		using digest             = deckard::sha256::digest;
		constexpr u32 BLOCK_SIZE = 64;

		export auto hash(std::span<const u8> key, std::span<const u8> msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(key, msg);
		}

		export auto hash(std::string_view key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(to_span(key), to_span(msg));
		}

		export auto hash(std::span<const u8> key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(key, to_span(msg));
		}

		export std::string quickhash(std::span<const u8> key, std::span<const u8> msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::span<const u8> key, std::string_view msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::string_view key, std::string_view msg)
		{
			return quickhash(to_span(key), to_span(msg));
		}
	} // namespace sha256

	// HMAC-SHA512
	namespace sha512
	{
		using hasher             = deckard::sha512::hasher;
		using digest             = deckard::sha512::digest;
		constexpr u32 BLOCK_SIZE = 128;

		export auto hash(std::span<const u8> key, std::span<const u8> msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(key, msg);
		}

		export auto hash(std::string_view key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(to_span(key), to_span(msg));
		}

		export auto hash(std::span<const u8> key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(key, to_span(msg));
		}

		export std::string quickhash(std::span<const u8> key, std::span<const u8> msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::span<const u8> key, std::string_view msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::string_view key, std::string_view msg)
		{
			return quickhash(to_span(key), to_span(msg));
		}
	} // namespace sha512

} // namespace deckard::hmac
