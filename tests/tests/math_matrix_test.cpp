#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;
using namespace deckard::math;

TEST_CASE("matrix generic", "[matrix]")
{
	SECTION("identity")
	{
		mat4 m = mat4::identity();

		REQUIRE(m[0] == 1.0f);
		REQUIRE(m[5] == 1.0f);
		REQUIRE(m[10] == 1.0f);
		REQUIRE(m[15] == 1.0f);

		REQUIRE(m[1] == 0.0f);
		REQUIRE(m[2] == 0.0f);
		REQUIRE(m[3] == 0.0f);
		REQUIRE(m[4] == 0.0f);
		REQUIRE(m[6] == 0.0f);
		REQUIRE(m[7] == 0.0f);
		REQUIRE(m[8] == 0.0f);
		REQUIRE(m[9] == 0.0f);
		REQUIRE(m[11] == 0.0f);
		REQUIRE(m[12] == 0.0f);
		REQUIRE(m[13] == 0.0f);
		REQUIRE(m[14] == 0.0f);

		REQUIRE(m == mat4(1.0f));

		m[0] = 2.0f;
		REQUIRE(m[0] == 2.0f);

		REQUIRE(m[5] == 1.0f);
		REQUIRE(m[10] == 1.0f);
		REQUIRE(m[15] == 1.0f);

		REQUIRE(m[1] == 0.0f);
		REQUIRE(m[2] == 0.0f);
		REQUIRE(m[3] == 0.0f);
		REQUIRE(m[4] == 0.0f);
		REQUIRE(m[6] == 0.0f);
		REQUIRE(m[7] == 0.0f);
		REQUIRE(m[8] == 0.0f);
		REQUIRE(m[9] == 0.0f);
		REQUIRE(m[11] == 0.0f);
		REQUIRE(m[12] == 0.0f);
		REQUIRE(m[13] == 0.0f);
		REQUIRE(m[14] == 0.0f);
	}

	SECTION("multiply")
	{
		const mat4 result{
		  90.0f, 100.0f, 110.0f, 120.0f, 202.0f, 228.0f, 254.0f, 280.0f, 314.0f, 356.0f, 398.0f, 440.0f, 426.0f, 484.0f, 542.0f, 600.0f};

		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		mat4       m2 = m;


		mat4 mul = m * m2;
		REQUIRE(mul == result);

		// identity
		REQUIRE(result == mat4(1.0f) * mul);
		REQUIRE(result == mul * mat4(1.0f));


		//
		const mat4 mat2(2, mat4::fill);

		const mat4 meq16(16, mat4::fill);

		mat4 mul_eq(mat2);

		mul_eq *= mat2;

		REQUIRE(mul_eq == meq16);
	}

	SECTION("add/sub")
	{
		const mat4 m2(2, mat4::fill);
		const mat4 m4(4, mat4::fill);

		mat4 result(m2);
		result += m2;

		REQUIRE(result == m4);

		result -= m2;
		REQUIRE(result == m2);
	}


	SECTION("div")
	{
		// sub
		const mat4 m8(4, mat4::fill);
		const mat4 half(0.5f, mat4::fill);

		mat4 m(m8);

		m /= mat4(8.0f);

		REQUIRE(m == half);

		m /= mat4(1.0f);
		REQUIRE(m == half);
	}

	SECTION("negate")
	{
		const mat4 a(-4, mat4::fill);
		mat4       m(4, mat4::fill);

		REQUIRE(a == -m);
	}

	SECTION("format")
	{
		const mat4 m(1.0f);
		auto       fmt = std::format("{}", m);

		std::string test(
		  "mat4((1.00000, 0.00000, 0.00000, 0.00000),\n"
		  "     (0.00000, 1.00000, 0.00000, 0.00000),\n"
		  "     (0.00000, 0.00000, 1.00000, 0.00000),\n"
		  "     (0.00000, 0.00000, 0.00000, 1.00000))");

		REQUIRE(fmt == test);
	}
}
