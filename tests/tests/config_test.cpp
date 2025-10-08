#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.config;
import deckard.debug;

TEST_CASE("config", "[config]")
{
	using namespace deckard;
	SECTION("")
	{
		//
		std::string c(R"(
[section] # section comment
key01 = value01 # kv comment

# just comment
)");


		_       = 0;

	}
}
