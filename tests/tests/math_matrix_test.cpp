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
	}

	SECTION("multiply")
	{
		const std::array<float, 18> res_arr{
		  90.0f, 100.0f, 110.0f, 120.0f, 202.0f, 228.0f, 254.0f, 280.0f, 314.0f, 356.0f, 398.0f, 440.0f, 426.0f, 484.0f, 542.0f, 600.0f};


		const mat4 result(res_arr.data());


		std::array<float, 18> arr{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		mat4                  m(arr.data());
		mat4                  m2 = m;


		mat4 mul = m * m2;
		REQUIRE(mul == result);

		//
		mat4 mul_ident = mul * mat4(1.0f);
		REQUIRE(mul_ident == result);


		//
		const std::array<float, 16> twos{2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
		const mat4                  mat2(twos.data());

		const std::array<float, 16> meq_res{16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
		const mat4                  meq16(meq_res.data());

		mat4 mul_eq(mat2);

		mul_eq *= mat2;

		REQUIRE(mul_eq == meq16);


		// sub
		const std::array<float, 16> meq_18res{18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18};
		const mat4                  meq18(meq_18res.data());
		mul_eq += mat2;

		REQUIRE(mul_eq == meq18);

		mul_eq -= mat2;
		REQUIRE(mul_eq == meq16);
	}
}
