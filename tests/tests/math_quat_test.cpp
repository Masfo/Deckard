#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace deckard::math;

TEST_CASE("quatertion", "[quaternion]")
{
	SECTION("identity")
	{
		quat q;

		REQUIRE(q[0] == 0.0f);
		REQUIRE(q[1] == 0.0f);
		REQUIRE(q[2] == 0.0f);
		REQUIRE(q[3] == 1.0f);
	}
}
