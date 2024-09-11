#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;
using namespace deckard::math;
using namespace Catch::Matchers;

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

	SECTION("lookat_rh")
	{
		mat4 lat = lookat_rh(
		  vec3(4, 10, 3), //
		  vec3(0, 0, 0),  //
		  vec3(0, 1, 0)   //
		);

		auto        fmt = std::format("{}", lat);
		std::string test(
		  "mat4((0.60000, -0.71554, 0.35777, 0.00000),\n"
		  "     (0.00000, 0.44721, 0.89443, 0.00000),\n"
		  "     (-0.80000, -0.53666, 0.26833, 0.00000),\n"
		  "     (0.00000, -0.00000, -11.18034, 1.00000))");

		REQUIRE(fmt == test);
	}

	SECTION("transpose")
	{
		//
		mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		m               = transpose(m);
		auto        fmt = std::format("{}", m);
		std::string test(
		  "mat4((1.00000, 5.00000, 9.00000, 13.00000),\n"
		  "     (2.00000, 6.00000, 10.00000, 14.00000),\n"
		  "     (3.00000, 7.00000, 11.00000, 15.00000),\n"
		  "     (4.00000, 8.00000, 12.00000, 16.00000))");
		REQUIRE(fmt == test);
	}

	SECTION("scale")
	{
		// clang-format off
	mat4 scal{
		    1, 2, 4, 8,
			0.2f, 0.4f, 0.6f, 0.8f, 
			1.2f, 2.4f, 3.6f, 4.8f, 
			8, 8, 8,8};
	// clang-format off

		scal               = scale(scal, vec3(2.0f, 3.0f, 4.0f));
		auto        fmt = std::format("{}", scal);
		std::string test(
		  "mat4((2.00000, 4.00000, 8.00000, 16.00000),\n"
		  "     (0.60000, 1.20000, 1.80000, 2.40000),\n"
		  "     (4.80000, 9.60000, 14.40000, 19.20000),\n"
		  "     (8.00000, 8.00000, 8.00000, 8.00000))");
		REQUIRE(fmt == test);
	}

	
	SECTION("translate")
	{
		// clang-format off
		mat4 transl{
				1,1,1,1,
				2,2,2,2,
				3,3,3,3,
				10,10,10,10
		};
	// clang-format off

		transl               = translate(transl, vec3(2.0f, 30.0f, 4.0f));
		auto        fmt = std::format("{}", transl);
		std::string test(
		  "mat4((1.00000, 1.00000, 1.00000, 1.00000),\n"
		  "     (2.00000, 2.00000, 2.00000, 2.00000),\n"
		  "     (3.00000, 3.00000, 3.00000, 3.00000),\n"
		  "     (84.00000, 84.00000, 84.00000, 84.00000))");
		REQUIRE(fmt == test);
	}

	
	SECTION("Rotate X")
	{
		mat4 rot = rotate(mat4(1.0f), 2.5f, vec3(-1.0f, 0.0f, 0.0f));
		auto fmt = std::format("{}", rot);

		std::string test(
		  "mat4((1.00000, 0.00000, 0.00000, 0.00000),\n"
		  "     (0.00000, -0.80114, -0.59847, 0.00000),\n"
		  "     (0.00000, 0.59847, -0.80114, 0.00000),\n"
		  "     (0.00000, 0.00000, 0.00000, 1.00000))");

		REQUIRE(fmt == test);
	}

		
	SECTION("Rotate Y")
	{

		mat4 rot = rotate(mat4(1.0f), -0.5f, vec3(0.0f, 1.0f, 0.0f));
		auto fmt = std::format("{}", rot);

		std::string test(
		  "mat4((0.87758, 0.00000, 0.47943, 0.00000),\n"
		  "     (0.00000, 1.00000, 0.00000, 0.00000),\n"
		  "     (-0.47943, 0.00000, 0.87758, 0.00000),\n"
		  "     (0.00000, 0.00000, 0.00000, 1.00000))");

		REQUIRE(fmt == test);
	}

		SECTION("Rotate Z")
	{

		mat4 rot = rotate(mat4(1.0f), 3.75f, vec3(0.0f, 0.0f, 1.0f));
		auto fmt = std::format("{}", rot);

		std::string test(
		  "mat4((-0.82056, -0.57156, 0.00000, 0.00000),\n"
		  "     (0.57156, -0.82056, 0.00000, 0.00000),\n"
		  "     (0.00000, 0.00000, 1.00000, 0.00000),\n"
		  "     (0.00000, 0.00000, 0.00000, 1.00000))");

		REQUIRE(fmt == test);
	}

	SECTION("full MVP")
	{

		mat4 Projection = perspective(to_rad(45.0f), 4.0f / 3.0f, 0.1f, 100.f);
		mat4 ViewTranslate = translate(mat4(1.0f), vec3(0.0f, 0.0f, -5.0f));
		mat4 ViewRotateX = rotate(ViewTranslate, 2.5f, vec3(-1.0f, 0.0f, 0.0f));
		mat4 View = rotate(ViewRotateX, -2.0f, vec3(0.0f, 1.0f, 0.0f));
		mat4 Model = scale(mat4(1.0f), vec3(0.5f));
		mat4 MVP = Projection * View * Model;


		auto fmt = std::format("{}", MVP);
		std::string test(
		  "mat4((-0.37675, 0.65689, 0.36497, 0.36424),\n"
		  "     (0.00000, -0.96707, 0.29984, 0.29924),\n"
		  "     (-0.82321, -0.30063, -0.16703, -0.16670),\n"
		  "     (0.00000, 0.00000, 4.80981, 5.00000))");

		REQUIRE(fmt == test);
	}


	SECTION("perspective")
	{
		auto persp = perspective(to_rad(85.0f), 1920.0f/1080.0f, 0.1f, 100.0f);

		auto        fmt = std::format("{}", persp);
		std::string test(
		  "mat4((0.61386, 0.00000, 0.00000, 0.00000),\n"
		  "     (0.00000, 1.09131, 0.00000, 0.00000),\n"
		  "     (0.00000, 0.00000, -1.00200, -1.00000),\n"
		  "     (0.00000, 0.00000, -0.20020, 0.00000))");
		REQUIRE(fmt == test);
	}

	SECTION("ortho")
	{
		auto orthopers = ortho( 0, 400, 0, 400, -1, 1 );

		auto        fmt = std::format("{}", orthopers);
		std::string test(
		  "mat4((0.00500, 0.00000, 0.00000, 0.00000),\n"
		  "     (0.00000, 0.00500, 0.00000, 0.00000),\n"
		  "     (0.00000, 0.00000, -1.00000, 0.00000),\n"
		  "     (-1.00000, -1.00000, -0.00000, 1.00000))");
		REQUIRE(fmt == test);
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
