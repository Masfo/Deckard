#include <catch2/catch_test_macros.hpp>


import std;
import deckard.uint128;

TEST_CASE("uint128", "[uint128]")
{
	SECTION("ctor")
	{
		//
		uint128 a(2, 0);
		uint128 b(4, 0);

		CHECK(a + b == uint128(6, 0));

		CHECK(1 == 1);
	}
}
