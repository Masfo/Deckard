#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace deckard::math;

TEST_CASE("ray", "[math][primitives][ray]")
{
	SECTION("at")
	{
		ray r(vec3(1.0f, 2.0f, 3.0f), vec3(0.0f, 0.0f, 1.0f));
		vec3 result = r.at(5.0f);
		REQUIRE(result == vec3(1.0f, 2.0f, 8.0f));
	}
}
