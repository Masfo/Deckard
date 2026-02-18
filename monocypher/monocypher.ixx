module;
#include <monocypher.h>
#include <optional/monocypher-ed25519.h>

export module deckard.monocypher;
import std;
import deckard.types;
import deckard.random;
import deckard.helpers;

// const uint8_t their_pk[32]{};    /* Their public key          */
// uint8_t your_sk[32]{};       /* Your secret key           */
// uint8_t your_pk[32]{};       /* Your public key           */
// uint8_t shared_secret[32]{}; /* Shared secret (NOT a key) */
// deckard::random::random_bytes({your_sk, 32});
// crypto_x25519_public_key(your_pk, your_sk);
// crypto_x25519(shared_secret, your_sk, their_pk);
// /* Wipe secrets if they are no longer needed */
// crypto_wipe(your_sk, 32);
//
// uint8_t            shared_keys[64]; /* Two shared session keys */
// crypto_blake2b_ctx ctx;
// crypto_blake2b_init(&ctx, 64);
// crypto_blake2b_update(&ctx, shared_secret, 32);
// crypto_blake2b_update(&ctx, your_pk, 32);
// crypto_blake2b_update(&ctx, their_pk, 32);
// crypto_blake2b_final(&ctx, shared_keys);
// const uint8_t* key_1 = shared_keys;      /* Shared key 1 */
// const uint8_t* key_2 = shared_keys + 32; /* Shared key 2 */
// /* Wipe secrets if they are no longer needed */
// crypto_wipe(shared_secret, 32);
//
namespace deckard::monocypher
{
	// TODO: easy to use helpers

	export template<std::integral T>
	constexpr bool crypto_verify(std::span<const T> a, const T value) noexcept
	{
		using Acc = std::make_unsigned_t<std::conditional_t<(sizeof(T) < 4), u32, T>>;
		Acc diff  = 0;

		for (size_t i = 0; i < a.size(); ++i)
			diff |= static_cast<Acc>(a[i] ^ value);

		return diff == 0;
	}

	export template<std::integral T, size_t N>
	constexpr int crypto_verify(std::span<const T, N> a, std::span<const T, N> b) noexcept
	{
		using Acc = std::make_unsigned_t<std::conditional_t<(sizeof(T) < 4), u32, T>>;
		Acc diff  = 0;

		for (size_t i = 0; i < N; ++i)
			diff |= static_cast<Acc>(a[i] ^ b[i]);

		return static_cast<int>(diff != 0);
	}

	export template<size_t N>
	constexpr int crypto_verify(std::span<const u8, N> a, std::span<const u8, N> b) noexcept
	{
		return crypto_verify<u8, N>(a, b);
	}

	export template<typename T = u8>
	constexpr int crypto_verify(std::span<const T> a, std::span<const T> b) noexcept
	{
		assert::check(a.size() == b.size(), "crypto_verify: input spans must have the same size");
		using Acc = std::make_unsigned_t<std::conditional_t<(sizeof(T) < 4), u32, T>>;

		Acc diff = 0;
		for (size_t i = 0; i < a.size(); ++i)
			diff |= static_cast<Acc>(a[i] ^ b[i]);

		return static_cast<int>(diff != 0);
	}
	{
		std::array<u8, 32> key{};

		bool operator==(const publickey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }
		auto operator<=>(const publickey& rhs) const { return std::memcmp(key.data(), rhs.key.data(), key.size()); }
	};

	export struct privatekey
	{
		std::array<u8, 32> key{};

		bool operator==(const privatekey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }

	};

	export struct sharedkey
	{
		std::array<u8, 32> key{};

		bool operator==(const sharedkey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }
	};

	export struct sessionkey
	{
		std::array<u8, 32> key{};

		bool operator==(const sessionkey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }
	};

	void initialize_privatekey(privatekey& privkey) { random::cryptographic_random_bytes(privkey.key); }

	export template<typename T>
	concept KeyType =
	  std::is_same_v<std::remove_cvref_t<T>, privatekey> or std::is_same_v<std::remove_cvref_t<T>, sharedkey> or
	  std::is_same_v<std::remove_cvref_t<T>, publickey> or std::is_same_v<std::remove_cvref_t<T>, sessionkey>;

	export template<KeyType T>
	void wipe_key(T& key)
	{
		crypto_wipe(key.key.data(), key.key.size());
	}

	export void create_private_and_public_keys(publickey& pubkey, privatekey& privkey)
	{
		initialize_privatekey(privkey);

		crypto_x25519_public_key(pubkey.key.data(), privkey.key.data());
	}

	export void create_shared_key(sharedkey& skey, privatekey& your_private_key, const publickey& their_public_key)
	{
		crypto_x25519(skey.key.data(), your_private_key.key.data(), their_public_key.key.data());

		wipe_key(your_private_key);
	}

	export [[nodiscard]] sessionkey create_session_key(const sharedkey& shared_key, const publickey& your_public_key, const publickey& their_public_key, u32 counter)
	{
		std::array<u8, 32> out;
		std::array<u8, 4>  ctr{};
		write_le(std::span<u8>{ctr}, 0, counter);

		// Sort public keys to ensure both parties derive the same key for the same counter
		const publickey* p1 = &your_public_key;
		const publickey* p2 = &their_public_key;
		if (*p1 > *p2)
		{
			std::swap(p1, p2);
		}

		crypto_blake2b_ctx ctx;
		crypto_blake2b_init(&ctx, out.size());
		crypto_blake2b_update(&ctx, shared_key.key.data(), shared_key.key.size());
		crypto_blake2b_update(&ctx, p1->key.data(), p1->key.size());
		crypto_blake2b_update(&ctx, p2->key.data(), p2->key.size());
		crypto_blake2b_update(&ctx, ctr.data(), ctr.size());
		crypto_blake2b_final(&ctx, out.data());

		sessionkey key;
		std::ranges::copy(out, key.key.begin());

		crypto_wipe(out.data(), out.size());
		crypto_wipe(ctr.data(), ctr.size());

		return key;
	}


} // namespace deckard::monocypher
