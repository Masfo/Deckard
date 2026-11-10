#include <catch2/catch_all.hpp>


import std;
import deckard.math;
import deckard.helpers;
using namespace deckard;

using namespace deckard::math;
using namespace Catch;

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
		CHECK(to_radians(360.0) == Approx(2.0 * std::numbers::pi).margin(0.000001));

		CHECK(to_radians(180.0) == Approx(std::numbers::pi).margin(0.000001));
		CHECK(to_radians(90.0) == Approx(std::numbers::pi / 2.0).margin(0.000001));
		CHECK(to_radians(0.0) == Approx(0.0).margin(0.000001));
	}
	SECTION("degree")
	{
		CHECK(to_degrees(2.0 * std::numbers::pi) == Approx(360.00).margin(0.000001));

		CHECK(to_degrees(std::numbers::pi) == Approx(180.0).margin(0.000001));
		CHECK(to_degrees(std::numbers::pi / 2.0) == Approx(90.0).margin(0.000001));
		CHECK(to_degrees(1.0) == Approx(57.29577951).margin(0.000001));
	}
}

TEST_CASE("math.utility", "[math]")
{

	SECTION("safe math floating")
	{
		CHECK(safe_mul(2.0f, 3.0f) == 6.0f);
		CHECK(safe_add(2.0f, 3.0f) == 5.0f);
		CHECK(safe_sub(5.0f, 3.0f) == 2.0f);
		CHECK(safe_div(10.0, 2.0) == 5.0);
		CHECK(safe_div(-7.0f, 3.0f) == Approx(-2.3333333f).margin(0.000001f));
		CHECK(safe_floor_div(7.0f, 3.0f) == 2.0f);
		CHECK(safe_floor_div(-7.0f, 3.0f) == -3.0f);
		CHECK(safe_floor_div(7.0f, -3.0f) == -3.0f);
		CHECK(safe_floor_div(-7.0f, -3.0f) == 2.0f);
		CHECK(safe_floor_div(6.0f, 3.0f) == 2.0f);
		CHECK(safe_ceil_div(7.0f, 3.0f) == 3.0f);
		CHECK(safe_ceil_div(-7.0f, 3.0f) == -2.0f);
		CHECK(safe_ceil_div(7.0f, -3.0f) == -2.0f);
		CHECK(safe_ceil_div(-7.0f, -3.0f) == 3.0f);
		CHECK(safe_ceil_div(6.0f, 3.0f) == 2.0f);

		const auto max = std::numeric_limits<f32>::max();
		CHECK_FALSE(safe_mul(max, 2.0f).has_value());
		CHECK_FALSE(safe_add(max, max).has_value());
		CHECK_FALSE(safe_sub(max, -max).has_value());
		CHECK_FALSE(safe_div(10.0f, 0.0f).has_value());
		CHECK_FALSE(safe_div(10.0, 0.0).has_value());
		CHECK_FALSE(safe_floor_div(10.0f, 0.0f).has_value());
		CHECK_FALSE(safe_ceil_div(10.0f, 0.0f).has_value());
		CHECK_FALSE(safe_div(max, std::numeric_limits<f32>::min()).has_value());
	}

	SECTION("safe math integral")
	{
		static_assert(safe_mul(2, 3).value() == 6);
		static_assert(safe_add(2, 3).value() == 5);
		static_assert(safe_sub(5, 3).value() == 2);
		static_assert(safe_div(-7, 3).value() == -2);
		static_assert(safe_floor_div(-7, 3).value() == -3);
		static_assert(safe_floor_div(7, -3).value() == -3);
		static_assert(safe_ceil_div(7, 3).value() == 3);
		static_assert(safe_ceil_div(-7, -3).value() == 3);

		CHECK(safe_mul(2, 3) == 6);
		CHECK(safe_add(2, 3) == 5);
		CHECK(safe_sub(5, 3) == 2);
		CHECK(safe_div(10, 2) == 5);
		CHECK(safe_div(-7, 3) == -2);
		CHECK(safe_floor_div(7, 3) == 2);
		CHECK(safe_floor_div(-7, 3) == -3);
		CHECK(safe_floor_div(7, -3) == -3);
		CHECK(safe_floor_div(-7, -3) == 2);
		CHECK(safe_floor_div(-6, 3) == -2);
		CHECK(safe_ceil_div(7, 3) == 3);
		CHECK(safe_ceil_div(-7, 3) == -2);
		CHECK(safe_ceil_div(7, -3) == -2);
		CHECK(safe_ceil_div(-7, -3) == 3);
		CHECK(safe_ceil_div(6, 3) == 2);

		const auto max = std::numeric_limits<i32>::max();
		const auto min = std::numeric_limits<i32>::min();
		CHECK_FALSE(safe_mul(min, -1).has_value());
		CHECK_FALSE(safe_add(max, 1).has_value());
		CHECK_FALSE(safe_add(min, -1).has_value());
		CHECK_FALSE(safe_sub(min, 1).has_value());
		CHECK_FALSE(safe_sub(max, -1).has_value());
		CHECK_FALSE(safe_div(min, -1).has_value());
		CHECK_FALSE(safe_div(10, 0).has_value());
		CHECK_FALSE(safe_floor_div(min, -1).has_value());
		CHECK_FALSE(safe_floor_div(10, 0).has_value());
		CHECK_FALSE(safe_ceil_div(min, -1).has_value());
		CHECK_FALSE(safe_ceil_div(10, 0).has_value());

		const auto unsigned_max = std::numeric_limits<u32>::max();
		CHECK(safe_mul(2u, 3u) == 6u);
		CHECK(safe_add(2u, 3u) == 5u);
		CHECK(safe_sub(5u, 3u) == 2u);
		CHECK(safe_div(10u, 2u) == 5u);
		CHECK(safe_floor_div(10u, 3u) == 3u);
		CHECK(safe_ceil_div(10u, 3u) == 4u);
		CHECK_FALSE(safe_mul(unsigned_max, 2u).has_value());
		CHECK_FALSE(safe_add(unsigned_max, 1u).has_value());
		CHECK_FALSE(safe_sub(0u, 1u).has_value());
		CHECK_FALSE(safe_div(10u, 0u).has_value());
	}


	SECTION("align")
	{
		CHECK(align_integer(7, 8) == 8);
		CHECK(align_integer(7, 16) == 16);
		CHECK(align_integer(7, 4) == 8);
		CHECK(align_integer(124, 2) == 124);
	}

	SECTION("remap")
	{
		CHECK(remap(0.5f, 0.0f, 1.0f, 20.0f, 40.0f) == Approx(30.0f).margin(0.0001));
		CHECK(550 == remap(5, 0, 10, 100, 1000));
	}
	SECTION("mod")
	{
		CHECK(10 == mod(10, 100));
		CHECK(10 == mod(110, 100));
		CHECK(3 == mod(-11, 7));
	}

	SECTION("mod float")
	{
		CHECK(10.0f == mod(10.0f, 100.0f));
		CHECK(10.0f == mod(110.0f, 100.0f));
		CHECK(3.0f == mod(-11.0f, 7.0f));
	}

	SECTION("digits")
	{
		CHECK(1 == count_digits(0));
		CHECK(1 == count_digits(1));

		CHECK(3 == count_digits(999));
		CHECK(6 == count_digits(999'999));
	}

}
