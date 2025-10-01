module;
#include <monocypher.h>
#include <optional/monocypher-ed25519.h>

export module deckard.monocypher;
import std;
import deckard.types;
import deckard.random;

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

	export struct publickey
	{
		std::array<u8, 32> key{};

		bool operator==(const publickey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }
	};

	export struct privatekey
	{
		std::array<u8, 32> key{};

		bool operator==(const privatekey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }

	};

	export struct sharedkey
	{
		std::array<u8, 32> key;

		bool operator==(const sharedkey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }
	};

	export struct sessionkey
	{
		std::array<u8, 32> key;

		bool operator==(const sessionkey& rhs) const { return crypto_verify32(key.data(), rhs.key.data()) == 0; }
	};

	void initialize_privatekey(privatekey& privkey) { random::cryptographic_random_bytes(privkey.key); }

	template<typename T>
	requires (std::is_same_v<T, privatekey> or std::is_same_v<T, sharedkey>)
	void wipe_key(T& key) 
	{
		crypto_wipe(key.key.data(), key.key.size()); 
	}


	void wipe_key(sharedkey& key) { crypto_wipe(key.key.data(), key.key.size()); }


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

	export void create_session_key(sharedkey &sharedkey, const privatekey &your_private_key, const publickey &their_public_key)
	{
		sessionkey         shared_key;

		std::array<u8, 64> shared_keys;

		crypto_blake2b_ctx ctx;
		crypto_blake2b_init(&ctx, 64);
		crypto_blake2b_update(&ctx, sharedkey.key.data(), sharedkey.key.size());
		crypto_blake2b_update(&ctx, your_private_key.key.data(), your_private_key.key.size());
		crypto_blake2b_update(&ctx, their_public_key.key.data(), their_public_key.key.size());
		crypto_blake2b_final(&ctx, shared_keys.data());
		sessionkey     session_key_1, session_key_2;


		std::ranges::copy_n(shared_keys.data(), 32, session_key_1.key.data());
		std::ranges::copy_n(shared_keys.data() + 32, 32, session_key_2.key.data());

		wipe_key(sharedkey);

		// TODO: key ratchet
	}


} // namespace deckard::monocypher
