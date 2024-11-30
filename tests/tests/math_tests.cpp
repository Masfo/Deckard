#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace deckard::math;
using namespace Catch::Matchers;

TEST_CASE("index from", "[index_from]")
{
	SECTION("index_from_2d")
	{
		REQUIRE(0 == index_from_2d(0, 0, 3));
		REQUIRE(4 == index_from_2d(1, 1, 3));
		REQUIRE(8 == index_from_2d(2, 2, 3));
	}

	SECTION("index_from_3d")
	{
		REQUIRE(0 == index_from_3d(0, 0, 0, 3, 3));
		REQUIRE(13 == index_from_3d(1, 1, 1, 3, 3));
		REQUIRE(26 == index_from_3d(2, 2, 2, 3, 3));
	}
}

TEST_CASE("radians/degrees", "[radians][degrees]")
{
	SECTION("deg to radians")
	{
		REQUIRE_THAT(to_radians(360.0), Catch::Matchers::WithinAbs(2.0 * std::numbers::pi, 0.000001));

		REQUIRE_THAT(to_radians(180.0), Catch::Matchers::WithinAbs(std::numbers::pi, 0.000001));
		REQUIRE_THAT(to_radians(90.0), Catch::Matchers::WithinAbs(std::numbers::pi / 2.0, 0.000001));
		REQUIRE_THAT(to_radians(0.0), Catch::Matchers::WithinAbs(0.0, 0.000001));
	}
	SECTION("degree")
	{
		REQUIRE_THAT(to_degrees(2.0 * std::numbers::pi), Catch::Matchers::WithinAbs(360.00, 0.000001));

		REQUIRE_THAT(to_degrees(std::numbers::pi), Catch::Matchers::WithinAbs(180.0, 0.000001));
		REQUIRE_THAT(to_degrees(std::numbers::pi / 2.0), Catch::Matchers::WithinAbs(90.0, 0.000001));
		REQUIRE_THAT(to_degrees(1.0), Catch::Matchers::WithinAbs(57.29577951, 0.000001));
	}
}

TEST_CASE("math.utility", "[math]")
{

	SECTION("remap")
	{
		REQUIRE_THAT(remap(0.5f, 0.0f, 1.0f, 20.0f, 40.0f), Catch::Matchers::WithinAbs(30.0f, 0.0001));
		REQUIRE(550 == remap(5, 0, 10, 100, 1'000));
	}
	SECTION("mod")
	{
		REQUIRE(10 == mod(10, 100));
		REQUIRE(10 == mod(110, 100));
		REQUIRE(3 == mod(-11, 7));
	}

	SECTION("digits") 
	{
		REQUIRE(1 == digits(0));
		REQUIRE(1 == digits(1));

		REQUIRE(3 == digits(999));
		REQUIRE(6 == digits(999999));
	}
}
