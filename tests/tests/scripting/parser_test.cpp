#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.parser;
import std;

using namespace deckard;

TEST_CASE("parser", "[parser]")
{

	//
	// CHECK_EQ(1, 1);
	SECTION("no tokens")
	{
		//
		REQUIRE(1 == 1);
	}
}
