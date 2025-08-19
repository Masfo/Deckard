#include <catch2/catch_test_macros.hpp>


import std;
import deckard.commandline;

TEST_CASE("commandline", "[commandline][cli]")
{
	using namespace deckard;
	SECTION("initialize")
	{
		CHECK(1 == 1);
	}
}
