
#include <catch2/catch_test_macros.hpp>

import deckard;
import std;

using namespace deckard;

bool function_call = false;
bool method_call   = false;

struct b
{
	int voidme(int x) { return x + 42; }
};

bool f(bool b)
{
	function_call = true;
	return not b;
}

TEST_CASE("function_ref", "[function_ref][utils]")
{

	// function
	{
		function_ref<bool(bool)> fr = f;
		REQUIRE(true == fr(false));
		REQUIRE(true == function_call);
	}

	// lambda
	{
		function_ref<int(int)> fr = [](int x) { return x + 42; };
		REQUIRE(fr(8) == 50);
	}

	// method
	{
		b                          o;
		function_ref<int(b&, int)> fr = &b::voidme;
		REQUIRE(fr(o, 8) == 50);
	}
}
