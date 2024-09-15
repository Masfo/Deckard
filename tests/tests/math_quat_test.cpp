#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace deckard::math;

TEST_CASE("quatertion", "[quaternion]")
{
	SECTION("")
	{
		quat q;

		q.test();

		//
		REQUIRE(1 == 1);
	}
}
