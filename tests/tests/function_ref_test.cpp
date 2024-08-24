
#include <catch2/catch_test_macros.hpp>

import deckard;
import std;

using namespace deckard;

int function_call = 0;
int method_call   = 0;

struct MyClass
{
	int method(int x)
	{
		method_call += 1;
		return x + 42;
	}
};

bool f(bool b)
{
	function_call += 1;
	return not b;
}

TEST_CASE("function_ref", "[function_ref][utils]")
{

	// function
	{
		function_ref<bool(bool)> fr = f;
		REQUIRE(true == fr(false));
		REQUIRE(1 == function_call);
	}

	// lambda
	{
		function_ref<int(int)> fr = [](int x) { return x + 42; };
		REQUIRE(fr(8) == 50);
	}

	{
		const int              value = 10;
		function_ref<int(int)> fr    = [&value](int x) { return x + 42 + value; };
		REQUIRE(fr(8) == 60);
	}

	// method
	{
		MyClass                          C;
		function_ref<int(MyClass&, int)> fr = &MyClass::method;
		REQUIRE(fr(C, 8) == 50);
		REQUIRE(1 == method_call);
	}
}
