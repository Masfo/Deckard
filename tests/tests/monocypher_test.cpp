#include <catch2/catch_test_macros.hpp>


import std;
import deckard.monocypher;

using namespace deckard::monocypher;

TEST_CASE("monocypher", "[monocypher]")
{
	SECTION("create sharedkeys")
	{
		const privatekey emptykey;

		publickey  alice_publickey;
		privatekey alice_privatekey;
		sharedkey  alice_sharedkey;

		publickey  bob_publickey;
		privatekey bob_privatekey;
		sharedkey  bob_sharedkey;

		create_private_and_public_keys(alice_publickey, alice_privatekey);
		create_private_and_public_keys(bob_publickey, bob_privatekey);

		create_shared_key(alice_sharedkey, alice_privatekey, bob_publickey);
		create_shared_key(bob_sharedkey, bob_privatekey, alice_publickey);

		CHECK(emptykey == alice_privatekey);
		CHECK(emptykey == bob_privatekey);

		CHECK(alice_sharedkey == bob_sharedkey);

		const sharedkey  empty_sharedkey;
		const sessionkey empty_sessionkey;

		auto alice_k0 = create_session_key(alice_sharedkey, alice_publickey, bob_publickey, 0);
		auto bob_k0   = create_session_key(bob_sharedkey, bob_publickey, alice_publickey, 0);
		CHECK(alice_k0 == bob_k0);
		CHECK(alice_k0 != empty_sessionkey);
		CHECK(bob_k0 != empty_sessionkey);

		auto alice_k1 = create_session_key(alice_sharedkey, alice_publickey, bob_publickey, 1);
		auto bob_k1   = create_session_key(bob_sharedkey, bob_publickey, alice_publickey, 1);
		CHECK(alice_k1 == bob_k1);
		CHECK(alice_k1 != empty_sessionkey);
		CHECK(bob_k1 != empty_sessionkey);
		CHECK(alice_k1 != alice_k0);

		wipe_key(alice_sharedkey);
		wipe_key(bob_sharedkey);
		CHECK(alice_sharedkey == empty_sharedkey);
		CHECK(bob_sharedkey == empty_sharedkey);

		SECTION("counter based")
		{
			publickey  alice_publickey2;
			privatekey alice_privatekey2;
			publickey  bob_publickey2;
			privatekey bob_privatekey2;

			create_private_and_public_keys(alice_publickey2, alice_privatekey2);
			create_private_and_public_keys(bob_publickey2, bob_privatekey2);

			sharedkey alice_sharedkey2;
			sharedkey bob_sharedkey2;

			create_shared_key(alice_sharedkey2, alice_privatekey2, bob_publickey2);
			create_shared_key(bob_sharedkey2, bob_privatekey2, alice_publickey2);

			auto k0_a = create_session_key(alice_sharedkey2, alice_publickey2, bob_publickey2, 0);
			auto k0_b = create_session_key(bob_sharedkey2, bob_publickey2, alice_publickey2, 0);
			CHECK(k0_a == k0_b);
			CHECK(k0_a != empty_sessionkey);

			auto k1_a = create_session_key(alice_sharedkey2, alice_publickey2, bob_publickey2, 1);
			auto k1_b = create_session_key(bob_sharedkey2, bob_publickey2, alice_publickey2, 1);
			CHECK(k1_a == k1_b);
			CHECK(k1_a != k0_a);

			wipe_key(alice_sharedkey2);
			wipe_key(bob_sharedkey2);
			CHECK(alice_sharedkey2 == empty_sharedkey);
			CHECK(bob_sharedkey2 == empty_sharedkey);
		}
	}
}
