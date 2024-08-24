
#include <catch2/catch_test_macros.hpp>

import deckard;
import std;

using namespace deckard;

namespace details
{
	int function_call_count = 0;
	int method_call_count   = 0;
}; // namespace details

struct MyClass
{
	int method(int x)
	{
		details::method_call_count += 1;
		return x + 42;
	}
};

int function_call(int v)
{
	details::function_call_count += 1;
	return v * 3;
}

int call_function_call(const function_ref<int(int)>& f, int value) { return f(value); }

TEST_CASE("function_ref", "[function_ref][utils]")
{

	// function
	{
		function_ref<int(int)> fr = function_call;
		REQUIRE(3 == fr(1));
		REQUIRE(1 == details::function_call_count);

		REQUIRE(call_function_call(function_call, 5) == 15);
		REQUIRE(2 == details::function_call_count);
	}

	// lambda
	{
		function_ref<int(int)> fr = [](int x) { return x + 42; };
		REQUIRE(fr(8) == 50);


		const int value = 10;
		fr              = [&value](int x) { return x + 42 + value; };
		REQUIRE(fr(8) == 60);

		auto l = [](int x)
		{
			details::function_call_count += 1;
			return x * 2;
		};
		REQUIRE(call_function_call(l, 5) == 10);
		REQUIRE(3 == details::function_call_count);
	}

	// method
	{
		MyClass                          C;
		function_ref<int(MyClass&, int)> fr = &MyClass::method;
		REQUIRE(fr(C, 8) == 50);
		REQUIRE(1 == details::method_call_count);
	}
}
