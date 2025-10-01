#include <catch2/catch_test_macros.hpp>


import std;
import deckard.monocypher;

using namespace deckard::monocypher;

TEST_CASE("monocypher", "[monocypher]")
{
	SECTION("create sharedkeys")
	{
		const privatekey emptykey;

		publickey  my_publickey;
		privatekey my_privatekey;
		sharedkey  my_sharedkey;

		publickey  their_publickey;
		privatekey their_privatekey;
		sharedkey  their_sharedkey;

		create_private_and_public_keys(my_publickey, my_privatekey);
		create_private_and_public_keys(their_publickey, their_privatekey);

		create_shared_key(my_sharedkey, my_privatekey, their_publickey);
		create_shared_key(their_sharedkey, their_privatekey, my_publickey);

		CHECK(emptykey == my_privatekey);
		CHECK(emptykey == their_privatekey);

		CHECK(my_sharedkey == their_sharedkey);


	}
}
