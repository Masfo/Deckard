#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import deckard.math.vec;
import std;

using namespace Catch::Matchers;

using namespace std::string_literals;
using namespace deckard::math;

// vec2
TEST_CASE("vec 2", "[vec][vec2][math]")
{
	//
	SECTION("constructor")
	{
		vec2 v{1.0f};

		REQUIRE(v[0] == 1.0f);
		REQUIRE(v[1] == 1.0f);
	}

	SECTION("basic math")
	{
		const vec2 v1{2.0f};
		const vec2 v2{2.0f, 3.0f};

		const vec2 add = v1 + v2;
		const vec2 sub = v1 - v2;
		const vec2 mul = v1 * v2;
		const vec2 div = v1 / v2;

		const vec2 add_result{4.0f, 5.0f};
		const vec2 sub_result{0.0f, -1.0f};
		const vec2 mul_result{4.0f, 6.0f};
		const vec2 div_result{1.0f, 0.6666666f};

		const vec2 div_diff = div.diff(div_result);


		REQUIRE(true == add.equals(add_result));
		REQUIRE(true == sub.equals(sub_result));
		REQUIRE(true == mul.equals(mul_result));
		REQUIRE(true == div.equals(div_result));
	}

	SECTION("vec2 other functions")
	{
		//
		const vec2 v1{2.0f};
		const vec2 v2{9.0f, -1.0f};

		const vec2 minimum = min(v1, v2);
		REQUIRE(true == minimum.equals(vec2{2.0f, -1.0f}));

		const vec2 maximum = max(v1, v2);
		REQUIRE(true == maximum.equals(vec2{9.0f, 2.0f}));

		const auto absolute = abs(v2);
		REQUIRE(true == absolute.equals({9.0f, 1.0f}));

		const auto dist = distance(v1, v2);
		REQUIRE_THAT(dist, WithinAbs(10.0, 0.000001));

		const vec2 clamped = clamp(v2, {2.0f}, {3.0f});
		REQUIRE(true == clamped.equals(vec2{3.0f, 2.0f}));
	}
}

// vec3
TEST_CASE("vec 3", "[vec][vec3][math]")
{
	//
	SECTION("constructor")
	{
		vec3 v{1.0f};

		REQUIRE(v[0] == 1.0f);
		REQUIRE(v[1] == 1.0f);
		REQUIRE(v[2] == 1.0f);
	}

	SECTION("basic math")
	{
		const vec3 v1{2.0f};
		const vec3 v2{2.0f, 3.0f, 4.0f};

		const vec3 add = v1 + v2;
		const vec3 sub = v1 - v2;
		const vec3 mul = v1 * v2;
		const vec3 div = v1 / v2;

		const vec3 add_result{4.0f, 5.0f, 6.0f};
		const vec3 sub_result{0.0f, -1.0f, -2.0f};
		const vec3 mul_result{4.0f, 6.0f, 8.0f};
		const vec3 div_result{1.0f, 0.6666666f, 0.500f};

		const vec3 div_diff = div.diff(div_result);


		REQUIRE(true == add.equals(add_result));
		REQUIRE(true == sub.equals(sub_result));
		REQUIRE(true == mul.equals(mul_result));
		REQUIRE(true == div.equals(div_result));
	}


	SECTION("vec3 other functions")
	{
		//
		const vec3 v1{2.0f};
		const vec3 v2{9.0f, -1.0f, 4.0f};

		const vec3 minimum = min(v1, v2);
		REQUIRE(true == minimum.equals(vec3{2.0f, -1.0f, 2.0f}));

		const vec3 maximum = max(v1, v2);
		REQUIRE(true == maximum.equals(vec3{9.0f, 2.0f, 4.0f}));

		const auto absolute = abs(v2);
		REQUIRE(true == absolute.equals({9.0f, 1.0f, 4.0f}));

		const auto dist = distance(v1, v2);
		REQUIRE_THAT(dist, WithinAbs(12.0, 0.000001));

		const vec3 clamped = clamp(v2, {2.0f}, {3.0f});
		REQUIRE(true == clamped.equals(vec3{3.0f, 2.0f, 3.0f}));
	}
}

// vec3
TEST_CASE("vec 4", "[vec][vec4][math]")
{
	//
	SECTION("constructor")
	{
		vec4 v{1.0f};

		REQUIRE(v[0] == 1.0f);
		REQUIRE(v[1] == 1.0f);
		REQUIRE(v[2] == 1.0f);
		REQUIRE(v[3] == 1.0f);
	}

	SECTION("basic math")
	{
		const vec4 v1{2.0f};
		const vec4 v2{2.0f, 3.0f, 4.0f, 5.0f};

		const vec4 add = v1 + v2;
		const vec4 sub = v1 - v2;
		const vec4 mul = v1 * v2;
		const vec4 div = v1 / v2;

		const vec4 add_result{4.0f, 5.0f, 6.0f, 7.0f};
		const vec4 sub_result{0.0f, -1.0f, -2.0f, -3.0f};
		const vec4 mul_result{4.0f, 6.0f, 8.0f, 10.0f};
		const vec4 div_result{1.0f, 0.6666666f, 0.500f, 0.400f};

		const vec4 div_diff = div.diff(div_result);


		REQUIRE(true == add.equals(add_result));
		REQUIRE(true == sub.equals(sub_result));
		REQUIRE(true == mul.equals(mul_result));
		REQUIRE(true == div.equals(div_result));
	}

	SECTION("vec4 other functions")
	{
		//
		const vec4 v1{2.0f};
		const vec4 v2{9.0f, -1.0f, 4.0f, -7.0f};

		const vec4 minimum = min(v1, v2);
		REQUIRE(true == minimum.equals(vec4{2.0f, -1.0f, 2.0f, -7.0f}));

		const vec4 maximum = max(v1, v2);
		REQUIRE(true == maximum.equals(vec4{9.0f, 2.0f, 4.0f, 2.0f}));

		const auto absolute = abs(v2);
		REQUIRE(true == absolute.equals({9.0f, 1.0f, 4.0f, 7.0f}));

		const auto dist = distance(v1, v2);
		REQUIRE_THAT(dist, WithinAbs(21.0, 0.000001));

		const vec4 clamped = clamp(v2, {2.0f}, {3.0f});
		REQUIRE(true == clamped.equals(vec4{3.0f, 2.0f, 3.0f, 2.0f}));
	}
}

// Format
TEST_CASE("vec_n format", "[vec][math]")
{
	SECTION("vec2")
	{
		const vec2 v{2.123f, 3.141f};

		std::string result = std::format("{}", v);
		REQUIRE(result == "vec2(2.123, 3.141)"s);
	}
	SECTION("vec3")
	{
		const vec3 v{2.123f, 3.141f, 4.169f};

		std::string result = std::format("{}", v);
		REQUIRE(result == "vec3(2.123, 3.141, 4.169)"s);
	}

	SECTION("vec4")
	{
		const vec4 v{2.123f, 3.141f, 4.169f, 5.f};

		std::string result = std::format("{}", v);
		REQUIRE(result == "vec4(2.123, 3.141, 4.169, 5.000)"s);
	}
}
