#include <catch2/catch_test_macros.hpp>


import std;
import deckard.arrays;

using namespace deckard::grid;

TEST_CASE("array2d<bool>", "[array]")
{
	SECTION("init")
	{
		//
		array2d<bool> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8);

		array2d<bool> g2(9, 9);
		REQUIRE(g2.size_in_bytes() == 11);
	}
}
