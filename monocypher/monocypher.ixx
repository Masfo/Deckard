module;
#include <monocypher.h>
#include <optional/monocypher-ed25519.h>

export module deckard.monocypher;
import std;
import deckard.types;

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



}
