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

	SECTION("safe divide")
	{
		CHECK(5 == safe_divide(10, 2));
		CHECK(safe_divide(10.0f, 3.3f) == Approx(3.03030f).margin(0.000001f));
		
		CHECK(safe_divide(0, 10) == 0);
		CHECK(safe_divide(0.0f, 2.0f) == 0);
		CHECK(safe_divide(0.0, 4.4) == 0);



		CHECK_FALSE(safe_divide(10, 0).has_value());
		CHECK_FALSE(safe_divide(10.0f, 0.0f).has_value());
		CHECK_FALSE(safe_divide(10.0, 0.0).has_value());



	}

}
