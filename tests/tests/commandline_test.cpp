#include <catch2/catch_test_macros.hpp>


import std;
import deckard.commandline;

TEST_CASE("commandline", "[commandline][cli]")
{
	using namespace deckard;
	SECTION("initialize")
	{
		cli cli;
		
		
		// -d, --debug
		cli.option("d", "debug", "Enable debug");



		cli.parse("deckard.exe -d");


		CHECK(1 == 1);
	}
}
