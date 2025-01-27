#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;
import deckard.helpers;
using namespace deckard;

using namespace deckard::math;
using namespace Catch::Matchers;

TEST_CASE("index from", "[index_from]")
{
	SECTION("index_from_2d")
	{
		CHECK(0 == index_from_2d(0, 0, 3));
		CHECK(4 == index_from_2d(1, 1, 3));
		CHECK(8 == index_from_2d(2, 2, 3));
	}

	SECTION("index_from_3d")
	{
		CHECK(0 == index_from_3d(0, 0, 0, 3, 3));
		CHECK(13 == index_from_3d(1, 1, 1, 3, 3));
		CHECK(26 == index_from_3d(2, 2, 2, 3, 3));
	}
}

TEST_CASE("radians/degrees", "[radians][degrees]")
{
	SECTION("deg to radians")
	{
		CHECK_THAT(to_radians(360.0), Catch::Matchers::WithinAbs(2.0 * std::numbers::pi, 0.000001));

		CHECK_THAT(to_radians(180.0), Catch::Matchers::WithinAbs(std::numbers::pi, 0.000001));
		CHECK_THAT(to_radians(90.0), Catch::Matchers::WithinAbs(std::numbers::pi / 2.0, 0.000001));
		CHECK_THAT(to_radians(0.0), Catch::Matchers::WithinAbs(0.0, 0.000001));
	}
	SECTION("degree")
	{
		CHECK_THAT(to_degrees(2.0 * std::numbers::pi), Catch::Matchers::WithinAbs(360.00, 0.000001));

		CHECK_THAT(to_degrees(std::numbers::pi), Catch::Matchers::WithinAbs(180.0, 0.000001));
		CHECK_THAT(to_degrees(std::numbers::pi / 2.0), Catch::Matchers::WithinAbs(90.0, 0.000001));
		CHECK_THAT(to_degrees(1.0), Catch::Matchers::WithinAbs(57.29577951, 0.000001));
	}
}

TEST_CASE("math.utility", "[math]")
{
	SECTION("align")
	{ 
		CHECK(align_integer(7, 8) == 8);
		CHECK(align_integer(7, 16) == 16);
		CHECK(align_integer(7, 4) == 8);
		CHECK(align_integer(124, 2) == 124);

	}

	SECTION("remap")
	{
		CHECK_THAT(remap(0.5f, 0.0f, 1.0f, 20.0f, 40.0f), Catch::Matchers::WithinAbs(30.0f, 0.0001));
		CHECK(550 == remap(5, 0, 10, 100, 1'000));
	}
	SECTION("mod")
	{
		CHECK(10 == mod(10, 100));
		CHECK(10 == mod(110, 100));
		CHECK(3 == mod(-11, 7));
	}

	SECTION("digits") 
	{
		CHECK(1 == count_digits(0));
		CHECK(1 == count_digits(1));

		CHECK(3 == count_digits(999));
		CHECK(6 == count_digits(999999));
	}
}
