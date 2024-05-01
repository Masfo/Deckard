
#include <catch2/catch_test_macros.hpp>

import deckard.tokenizer;
import std;

using namespace deckard;

TEST_CASE("tokenizer")
{

	//
	// CHECK_EQ(1, 1);
	SECTION("no tokens")
	{
		//
		CHECK(1 == 1);
	}
}
