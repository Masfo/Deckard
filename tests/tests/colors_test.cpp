#include <catch2/catch_test_macros.hpp>


import std;
import deckard.colors;
using namespace deckard;

TEST_CASE("colors", "[colors][rgba][rgb]")
{
	SECTION("rgb to hsv")
	{
		//CHECK(rgb(0, 0, 0) == hsv(0.0f, 0.0f, 0.0f));
		//
		//CHECK(rgb(255, 0, 0) == hsv(0.0f, 100.0f, 100.0f));
		//CHECK(rgb(0, 255, 0) == hsv(120.0f, 100.0f, 100.0f));
		//CHECK(rgb(0, 0, 255) == hsv(240.0f, 100.0f, 100.0f));
		//
		//CHECK(rgb(255, 0, 255) == hsv(300.0f, 100.0f, 100.0f));
		//CHECK(rgb(255, 255, 255) == hsv(0.0f, 0.0f, 100.0f));
		//
		//
		//CHECK(rgb(0, 128, 192) == hsv(200.0f, 100.0f, 75.3f));
	}
}
