#include <catch2/catch_test_macros.hpp>


import deckard.types;
import deckard.types;

using namespace deckard;

TEST_CASE("utility", "[utility]")
{
	SECTION("limits")
	{
		//
		CHECK(524'287 == limits::bits_to_unsigned_max(19));
		CHECK(262'143 == limits::bits_to_signed_max(19));

		CHECK(127 == limits::bits_to_signed_max(8));
		CHECK(524'287 == limits::bits_to_unsigned_max(19));

		CHECK(-128 == limits::bits_to_signed_min(8));
		CHECK(-262'144 == limits::bits_to_signed_min(19));
	}
}
