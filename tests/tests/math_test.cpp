#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import deckard.math.vec;
import deckard.math.utility;
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

		vec2 v2{1.0f, 2.0f};
		REQUIRE(v2[0] == 1.0f);
		REQUIRE(v2[1] == 2.0f);

		vec2 v3{1.0f, 2.0f};
		v3 = -v3;
		REQUIRE(v3[0] == -1.0f);
		REQUIRE(v3[1] == -2.0f);
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
		const vec2 div_result{1.0f, 0.666666627f};

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

		const vec2 clamped = clamp(v2, 2.0f, 3.0f);
		REQUIRE(true == clamped.equals(vec2{3.0f, 2.0f}));
	}

	SECTION("vec2 cross/dot/length/normal/project/angle")
	{
		//
		const vec2 a{3.0f, 4.0f};
		const vec2 b{1.0f, -2.0f};

		const auto dotted = dot(a, b);
		REQUIRE_THAT(dotted, WithinAbs(-5.0f, deckard::math::epsilon<float>));

		const vec2 c{2.0f, 3.0f};
		const vec2 d{4.0f, 1.0f};
		const auto crossed = cross(c, d);
		REQUIRE_THAT(crossed, WithinAbs(-10.0f, deckard::math::epsilon<float>));

		// length
		const vec2 lvec{3.14f, 5.11f};
		const auto len = length(lvec);
		REQUIRE_THAT(len, WithinAbs(5.9976415f, 0.000001));

		// normalize
		const vec2 normalized = lvec.normalized();
		REQUIRE(true == normalized.equals(vec2{0.52353912f, 0.85200160f}));

		// normalize inplace
		vec2 normalize_inplace = lvec;
		normalize_inplace.normalize();
		REQUIRE(normalize_inplace == normalized);

		// project
		const vec2 pA{3.0f, 4.0f};
		const vec2 pB{2.0f, -1.0f};

		const vec2 projected = project(pA, pB);
		REQUIRE(true == projected.equals(vec2{0.8f, -0.4f}));

		const vec2 projected2 = project(pB, pA);
		REQUIRE(true == projected2.equals(vec2{0.23999999f, 0.31999999f}));

		// angle
		const vec2 angleA{3.0f, 4.0f};
		const vec2 angleB{2.0f, -1.0f};
		const auto angle_between = angle(angleA, angleB);
		REQUIRE_THAT(angle_between, WithinAbs(79.69515f, 0.00001));
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

		vec3 v2{1.0f, 2.0f};
		REQUIRE(v2[0] == 1.0f);
		REQUIRE(v2[1] == 2.0f);
		REQUIRE(v2[2] == 0.0f);

		vec3 v3{1.0f, 2.0f, 3.0f};
		REQUIRE(v3[0] == 1.0f);
		REQUIRE(v3[1] == 2.0f);
		REQUIRE(v3[2] == 3.0f);

		vec3 v4{1.0f, 2.0f, 3.0f};
		v4 = -v4;
		REQUIRE(v4[0] == -1.0f);
		REQUIRE(v4[1] == -2.0f);
		REQUIRE(v4[2] == -3.0f);
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
		REQUIRE_THAT(dist, WithinAbs(12.0, deckard::math::epsilon<float>));

		const vec3 clamped = clamp(v2, 2.0f, 3.0f);
		REQUIRE(true == clamped.equals(vec3{3.0f, 2.0f, 3.0f}));
	}

	SECTION("vec3 cross/dot/normal/length/angle")
	{
		//
		const vec3 a{2.1f, 3.2f, 1.3f};
		const vec3 b{4.4f, -1.5f, 5.6f};

		const vec3 crossed = cross(a, b);
		REQUIRE(true == crossed.equals(vec3{19.870000f, -6.0399994f, -17.230001f}));

		const auto dotted = dot(a, b);
		REQUIRE_THAT(dotted, WithinAbs(11.719999f, deckard::math::epsilon<float>));

		// length
		const vec3 lvec{3.14f, 5.11f, -1.34f};
		const auto len = length(lvec);
		REQUIRE_THAT(len, WithinAbs(6.1455106f, 0.000001));

		// normalize
		const vec3 normalized = lvec.normalized();
		REQUIRE(true == normalized.equals(vec3{0.51094210f, 0.83150130f, -0.21804535f}));

		// normalize inplace
		vec3 normalize_inplace = lvec;
		normalize_inplace.normalize();
		REQUIRE(normalize_inplace == normalized);


		// project
		const vec3 pA{3.0f, 4.0f, 1.0f};
		const vec3 pB{2.0f, -1.0f, 5.0f};

		const vec3 projected = project(pA, pB);
		REQUIRE(true == projected.equals(vec3{0.46666663f, -0.23333331f, 1.1666666f}));

		// angle
		const vec3 angleA{3.0f, 4.0f, 1.0f};
		const vec3 angleB{2.0f, -1.0f, 5.0f};
		const auto angle_between = angle(angleA, angleB);
		REQUIRE_THAT(angle_between, WithinAbs(75.48459f, 0.00001));

		// rotate
		const vec3 rotateA(1.0f, 0.0, 0.0f);
		const vec3 axis(0.0f, 0.0, 1.0f);

		float angle = to_radians<float>(180.0f);

		const vec3 rotated = rotate(rotateA, axis, angle);
		REQUIRE(true == rotated.equals(vec3{-1.0f, 0.00000000f, 0.0f}));

		const vec3 rotated2 = rotate(vec3{0.0f, 1.0f, 0.0f}, vec3{0.0f, 0.0f, 1.0f}, to_radians<float>(180));
		REQUIRE(true == rotated2.equals(vec3{0.0f, -1.0f, 0.0f}));
	}
}

// vec4
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

		vec4 v2{1.0f, 2.0f};
		REQUIRE(v2[0] == 1.0f);
		REQUIRE(v2[1] == 2.0f);
		REQUIRE(v2[2] == 0.0f);
		REQUIRE(v2[3] == 0.0f);

		vec4 v3{1.0f, 2.0f, 3.0f};
		REQUIRE(v3[0] == 1.0f);
		REQUIRE(v3[1] == 2.0f);
		REQUIRE(v3[2] == 3.0f);
		REQUIRE(v3[3] == 0.0f);

		vec4 v4{1.0f, 2.0f, 3.0f, 4.0f};
		REQUIRE(v4[0] == 1.0f);
		REQUIRE(v4[1] == 2.0f);
		REQUIRE(v4[2] == 3.0f);
		REQUIRE(v4[3] == 4.0f);


		vec4 v5{1.0f, 2.0f, 3.0f, 4.0f};
		v5 = -v5;
		REQUIRE(v5[0] == -1.0f);
		REQUIRE(v5[1] == -2.0f);
		REQUIRE(v5[2] == -3.0f);
		REQUIRE(v5[3] == -4.0f);
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


		REQUIRE(true == add.equals(add_result));
		REQUIRE(true == sub.equals(sub_result));
		REQUIRE(true == mul.equals(mul_result));
		REQUIRE(true == div.equals(div_result));

		// scalar
		const vec4 add_scalar = v1 + 4;
		const vec4 sub_scalar = v1 - 4;
		const vec4 mul_scalar = v1 * 4;
		const vec4 div_scalar = v1 / 4;

		const vec4 add_scalar_result{6.0f, 6.0f, 6.0f, 6.0f};
		const vec4 sub_scalar_result{-2.0f, -2.0f, -2.0f, -2.0f};
		const vec4 mul_scalar_result{8.0f, 8.0f, 8.0f, 8.0f};
		const vec4 div_scalar_result{0.5f, 0.5f, 0.5f, 0.5f};
		REQUIRE(true == add_scalar.equals(add_scalar_result));
		REQUIRE(true == sub_scalar.equals(sub_scalar_result));
		REQUIRE(true == mul_scalar.equals(mul_scalar_result));
		REQUIRE(true == div_scalar.equals(div_scalar_result));
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
		REQUIRE_THAT(dist, WithinAbs(21.0f, 0.000001));

		const vec4 clamped = clamp(v2, 2.0f, 3.0f);
		REQUIRE(true == clamped.equals(vec4{3.0f, 2.0f, 3.0f, 2.0f}));
	}

	SECTION("vec4 cross/dot/length/normal/project")
	{
		// cross
		const vec4 a{3.0, -3.0, 1.0};
		const vec4 b{4.0, 9.0, 2.0};

		const vec3 crossed = cross(a, b);
		REQUIRE(true == crossed.equals(vec3{-15.0, -2.0, 39.0}));

		// length
		const vec4 lvec{3.14f, 5.11f, -1.34f, 8.1f};
		const auto len = length(lvec);
		REQUIRE_THAT(len, WithinAbs(10.167463f, 0.000001));

		// normalize
		const vec4 normalized = lvec.normalized();
		REQUIRE(true == normalized.equals(vec4{0.30882826f, 0.50258356f, -0.13179294f, 0.79665893f}));

		vec4 normalize_inplace = lvec;
		normalize_inplace.normalize();
		REQUIRE(normalize_inplace == normalized);
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
