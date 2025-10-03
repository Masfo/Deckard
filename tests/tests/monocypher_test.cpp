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


	}
}
