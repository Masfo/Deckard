export module deckard.hmac;

import std;
import deckard.types;
import deckard.as;
import deckard.sha;
import deckard.helpers;

namespace deckard::hmac
{


	template<class H>
	concept Hasher = requires(H h, std::span<const std::byte> s) {
		{ H::block_size } -> std::convertible_to<std::size_t>;
		typename H::digest_type;
		{ h.update(s) } -> std::same_as<void>;
		{ h.finalize() } -> std::same_as<typename H::digest_type>;
	};

	template<typename Hasher, typename Digest, u32 BLOCK_SIZE>
	Digest generic_hmac(std::span<u8> key, std::span<u8> message)
	{
		std::array<u8, BLOCK_SIZE> norm_key{};
		if (key.size() > BLOCK_SIZE)
		{
			Hasher tmp;
			tmp.update(key);
			auto hashed = tmp.finalize();

			auto key_bytes = std::as_bytes(hashed.data());
			std::memcpy(norm_key.data(), key_bytes.data(), key_bytes.size());
		}
		else
		{
			auto key_bytes = std::as_bytes(key);
			std::memcpy(norm_key.data(), key_bytes.data(), key_bytes.size());
		}

		std::array<u8, BLOCK_SIZE> ipad{};
		std::array<u8, BLOCK_SIZE> opad{};
		for (std::size_t i = 0; i < BLOCK_SIZE; ++i)
		{
			ipad[i] = norm_key[i] ^ u8 { 0x36 };
			opad[i] = norm_key[i] ^ u8 { 0x5c };
		}

		Hasher inner;
		inner.update(ipad);
		inner.update(message);
		auto inner_digest = inner.finalize();

		Hasher outer;
		outer.update(opad);
		outer.update(inner_digest.data());
		return outer.finalize();
	}

	// SHA1-HMAC
	namespace sha1
	{
		using hasher             = deckard::sha1::hasher;
		using digest             = deckard::sha1::digest;
		constexpr u32 BLOCK_SIZE = 64;

		export auto hash(std::span<u8> key, std::span<u8> msg) { return generic_hmac<hasher, digest, BLOCK_SIZE>(key, msg); }

		export auto hash(std::string_view key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(to_span(key), to_span(msg));
		}

		export auto hash(std::span<u8> key, std::string_view msg) { return generic_hmac<hasher, digest, BLOCK_SIZE>(key, to_span(msg)); }

		export std::string quickhash(std::span<u8> key, std::span<u8> msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::span<u8> key, std::string_view msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::string_view key, std::string_view msg) { return quickhash(to_span(key), to_span(msg)); }
	} // namespace sha1

	// HMAC-SHA256
	namespace sha256
	{
		using hasher             = deckard::sha256::hasher;
		using digest             = deckard::sha256::digest;
		constexpr u32 BLOCK_SIZE = 64;

		export auto hash(std::span<u8> key, std::span<u8> msg) { return generic_hmac<hasher, digest, BLOCK_SIZE>(key, msg); }

		export auto hash(std::string_view key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(to_span(key), to_span(msg));
		}

		export auto hash(std::span<u8> key, std::string_view msg) { return generic_hmac<hasher, digest, BLOCK_SIZE>(key, to_span(msg)); }

		export std::string quickhash(std::span<u8> key, std::span<u8> msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::span<u8> key, std::string_view msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::string_view key, std::string_view msg) { return quickhash(to_span(key), to_span(msg)); }
	} // namespace sha256

	// HMAC-SHA512
	namespace sha512
	{
		using hasher             = deckard::sha512::hasher;
		using digest             = deckard::sha512::digest;
		constexpr u32 BLOCK_SIZE = 128;

		export auto hash(std::span<u8> key, std::span<u8> msg) { return generic_hmac<hasher, digest, BLOCK_SIZE>(key, msg); }

		export auto hash(std::string_view key, std::string_view msg)
		{
			return generic_hmac<hasher, digest, BLOCK_SIZE>(to_span(key), to_span(msg));
		}

		export auto hash(std::span<u8> key, std::string_view msg) { return generic_hmac<hasher, digest, BLOCK_SIZE>(key, to_span(msg)); }

		export std::string quickhash(std::span<u8> key, std::span<u8> msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::span<u8> key, std::string_view msg) { return hash(key, msg).to_string(); }

		export std::string quickhash(std::string_view key, std::string_view msg) { return quickhash(to_span(key), to_span(msg)); }
	} // namespace sha512

} // namespace deckard::hmac
