#include <catch2/catch_test_macros.hpp>


import std;
import deckard.grid;

using namespace deckard::grid;

TEST_CASE("grid<bool>", "[grid]")
{
	SECTION("init")
	{
		//
		grid<bool> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8);

		grid<bool> g2(9, 9);
		REQUIRE(g2.size_in_bytes() == 11);
	}
}
