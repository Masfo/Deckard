#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.config;
import deckard.debug;

TEST_CASE("config", "[config]")
{
	using namespace deckard::config;
	SECTION("key and value no section")
	{
		//
		std::string c(R"(
key01 = value01 # kv comment
# just comment
)");

		config2 cfg(c);



		_       = 0;

	}
}
