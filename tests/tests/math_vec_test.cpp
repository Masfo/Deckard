#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.debug;
import deckard.helpers;
import deckard.math;
import deckard.types;
import deckard.vec;


import std;

using namespace Catch::Matchers;

using namespace std::string_literals;
using namespace deckard::math;
using namespace deckard;

TEST_CASE("ivec2", "[vec][ivec2][math]")
{
	SECTION("constructor")
	{
		const ivec2 v0{2};
		REQUIRE(v0.x == 2);
		REQUIRE(v0.y == 2);

		const ivec2 v01{2, 9};
		REQUIRE(v01.x == 2);
		REQUIRE(v01.y == 9);

		ivec2 v(1);
		REQUIRE(v.x == 1);
		REQUIRE(v.y == 1);

		ivec2 v2(1, 0);
		REQUIRE(v2.has_zero() == true);

		ivec2 v3(0);
		REQUIRE(v3.is_zero() == true);
	}

	SECTION("compares")
	{
		// lt
		const ivec2 a{0, 0};
		const ivec2 b{1, 1};

		REQUIRE(a < b);
		REQUIRE(a != b);

		const ivec2 c{2, 2};
		const ivec2 d{2, 2};
		REQUIRE((c < d) == false);
		REQUIRE(c == d);

		const ivec2 e{1, 1};
		const ivec2 f{1, 1};
		REQUIRE(false == (e < f));
		REQUIRE(true == (e <= f));
		REQUIRE(false == (e > f));
		REQUIRE(true == (e >= f));
		REQUIRE(e == f);

		const ivec2 g{1, 1};
		const ivec2 h{2, 1};
		REQUIRE(h >= g);
		REQUIRE(h > g);
	}

	SECTION("basic math")
	{
		const ivec2 v1{2};
		const ivec2 v2{2, 3};

		const ivec2 add = v1 + v2;
		const ivec2 sub = v1 - v2;
		const ivec2 mul = v1 * v2;
		const ivec2 div = v1 / v2;

		const ivec2 add_result{4, 5};
		const ivec2 sub_result{0, -1};
		const ivec2 mul_result{4, 6};
		const ivec2 div_result{1, 0};

		REQUIRE(true == add.equals(add_result));
		REQUIRE(true == sub.equals(sub_result));
		REQUIRE(true == mul.equals(mul_result));
		REQUIRE(true == div.equals(div_result));


		ivec2 sres(4);
		sres /= 2;
		REQUIRE(sres.equals(ivec2(2)));
		sres *= 3;
		REQUIRE(sres.equals(ivec2(6)));
		sres += 1;
		REQUIRE(sres.equals(ivec2(7)));
		sres -= 5;
		REQUIRE(sres.equals(ivec2(2)));

		// ++ --
		ivec2 ppv(1, 2);
		REQUIRE(ppv == ivec2(1, 2));
		ivec2 post = ppv++;
		REQUIRE(post == ivec2(1, 2));
		ivec2 pre = ++ppv;
		REQUIRE(pre == ivec2(3, 4));


		post = ppv--;
		REQUIRE(post == ivec2(3, 4));
		pre = --ppv;
		REQUIRE(pre == ivec2(1, 2));
	}

	SECTION("ivec2 other functions")
	{
		//
		const ivec2 v1{2};
		const ivec2 v2{9, -1};

		const ivec2 minimum = min(v1, v2);
		REQUIRE(true == minimum.equals(ivec2{2, -1}));

		const ivec2 maximum = max(v1, v2);
		REQUIRE(true == maximum.equals(ivec2{9, 2}));

		const auto absolute = abs(v2);
		REQUIRE(true == absolute.equals({9, 1}));

		const auto dist = distance(v1, v2);
		REQUIRE(dist == 10);

		const ivec2 clamped = clamp(v2, 7, 5);
		REQUIRE(true == clamped.equals(ivec2{7, 5}));
	}
	SECTION("ivec2 cross/dot/length/normal/project/angle")
	{
		//
		const ivec2 a{3, 4};
		const ivec2 b{1, -2};

		const auto dotted = dot(a, b);
		REQUIRE(dotted == -5);


		const ivec2 c{1, 2};
		const ivec2 d{3, 4};
		const auto  crossed = cross(c, d);
		REQUIRE(crossed == -2);

		// length
		const ivec2 lvec{3, 5};
		const auto  len = length(lvec);
		REQUIRE(len == 8);

		// normalize inplace
		// ivec2 normalize_inplace = lvec;
		// normalize_inplace.normalize();
		// REQUIRE(normalize_inplace == ivec2(1,1));

		// normalize
		// const ivec2 normalized = lvec.normalize();
		// REQUIRE(true == normalized.equals(ivec2{1, 1}));


		// project
		// const ivec2 pA{3, 4};
		// const ivec2 pB{2, -1};
		//
		// const ivec2 projected = project(pA, pB);
		// REQUIRE(true == projected.equals(ivec2{2, 4}));
		//
		// const ivec2 projected2 = project(pB, pA);
		// REQUIRE(true == projected2.equals(ivec2{2, 2}));
		//
		// // angle
		// const ivec2 angleA{3, 4};
		// const ivec2 angleB{2, -1};
		// const f32 angle_between = angle(angleA, angleB);
		// REQUIRE_THAT(angle_between, WithinAbs(79.69515f, 0.00001));
		// 		// reflected
		// const ivec2 dir{9, -1};
		// const ivec2 normal{3, 5};
		// auto       reflected = reflect(dir, normal);
		// int         k         = 0;
		// //REQUIRE(reflected == vec2(-136.382004f, -237.593002f));
	}
}

// vec2
TEST_CASE("vec 2", "[vec][vec2][math]")
{
	//


	SECTION("constructor")
	{
		vec2 v{1};
		REQUIRE(v.x == 1.0f);
		REQUIRE(v.y == 1.0f);

		vec2 v2{1.0f, 2.0f};
		REQUIRE(v2.x == 1.0f);
		REQUIRE(v2.y == 2.0f);

		vec2 v3{1.0f, 2.0f};
		v3 = -v3;
		REQUIRE(v3.x == -1.0f);
		REQUIRE(v3.y == -2.0f);


		// inf
		// vec2 vinf = vec2::inf();
		// REQUIRE(vinf.is_inf() == true);
		// 
		// vinf = vec2(0, std::numeric_limits<float>::infinity());
		// REQUIRE(vinf.has_inf() == true);
		// 
		// // nan
		// vec2 vnan = vec2::nan();
		// REQUIRE(vnan.is_nan() == true);
		// 
		// vec2 vnan2(0, std::numeric_limits<float>::quiet_NaN());
		// REQUIRE(vnan2.has_nan() == true);

		// zero
		vec2 vzero = vec2::zero();
		REQUIRE(vzero.is_zero() == true);

		vzero = vec2(0, 1);
		REQUIRE(vzero.has_zero() == true);
	}


	SECTION("compares")
	{
		// lt
		const vec2 a{0.0, 0.0};
		const vec2 b{1.0, 1.0};

		REQUIRE(a < b);
		REQUIRE(a != b);

		const vec2 c{2.0, 2.0};
		const vec2 d{2.0, 2.0};
		REQUIRE((c < d) == false);
		REQUIRE(c == d);

		const vec2 e{1.0, 1.0};
		const vec2 f{1.0, 1.0};
		REQUIRE(false == (e < f));
		REQUIRE(true == (e <= f));
		REQUIRE(false == (e > f));
		REQUIRE(true == (e >= f));
		REQUIRE(e == f);

		const vec2 g{1.0, 1.0};
		const vec2 h{2.0, 1.0};
		REQUIRE(h >= g);
		REQUIRE(h > g);
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


		vec2 sres(4.0);
		sres /= 2.0f;
		REQUIRE(sres.equals(vec2(2.0f)));
		sres *= 3.0f;
		REQUIRE(sres.equals(vec2(6.0f)));
		sres += 1.0f;
		REQUIRE(sres.equals(vec2(7.0f)));
		sres -= 5.0f;
		REQUIRE(sres.equals(vec2(2.0f)));

		// ++ --
		vec2 ppv(1, 2);
		REQUIRE(ppv == vec2(1, 2));
		vec2 post = ppv++;
		REQUIRE(post == vec2(1, 2));
		vec2 pre = ++ppv;
		REQUIRE(pre == vec2(3, 4));


		post = ppv--;
		REQUIRE(post == vec2(3, 4));
		pre = --ppv;
		REQUIRE(pre == vec2(1, 2));
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
		REQUIRE_THAT(dist, WithinAbs(7.6157732f, 0.000001));

		const vec2 clamped = clamp(v2, 2.0f, 3.0f);
		REQUIRE(true == clamped.equals(vec2{3.0f, 2.0f}));
	}

	SECTION("vec2 cross/dot/length/normal/project/angle")
	{
		//
		const vec2 a{3.0f, 4.0f};
		const vec2 b{1.0f, -2.0f};

		const auto dotted = dot(a, b);
		REQUIRE_THAT(dotted, WithinAbs(-5.0f, 0.0000001));


		const vec2  c{1.0f, 2.0f};
		const vec2  d{3.0f, 4.0f};
		const float crossed = cross(c, d);
		REQUIRE_THAT(crossed, WithinAbs(-2.0f, 0.0000001));

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

		// reflected
		const vec2 dir{9.0f, -1.0f};
		const vec2 normal{3.14f, 5.11f};
		vec2       reflected = reflect(dir, normal);
		REQUIRE(reflected == vec2(-136.382004f, -237.593002f));
	}
}

// vec3
TEST_CASE("vec 3", "[vec][vec3][math]")
{
	//
	SECTION("constructor")
	{
		vec3 v{1.0f};
		REQUIRE(v.x == 1.0f);
		REQUIRE(v.y == 1.0f);
		REQUIRE(v.z == 1.0f);

		vec3 v2{1.0f, 2.0f};
		REQUIRE(v2.x == 1.0f);
		REQUIRE(v2.y == 2.0f);
		REQUIRE(v2.z == 0.0f);

		vec3 v3{1.0f, 2.0f, 3.0f};
		REQUIRE(v3.x == 1.0f);
		REQUIRE(v3.y == 2.0f);
		REQUIRE(v3.z == 3.0f);

		vec3 v4{1.0f, 2.0f, 3.0f};
		v4 = -v4;
		REQUIRE(v4.x == -1.0f);
		REQUIRE(v4.y == -2.0f);
		REQUIRE(v4.z == -3.0f);


		vec3 v5(0.0f);
		REQUIRE(v5.is_zero() == true);

		v5 = vec3(0.0f, 2.0f, 3.0);
		REQUIRE(v5.has_zero() == true);


		//vec3 v6 = vec3(1.0f).safe_divide(v5);
		//REQUIRE(v6.has_inf() == true);
		//REQUIRE(v6.is_inf() == true);


		vec2 v8(6.0f, 2.0f);
		vec3 v9 = vec3{v8, -1.0f};
		REQUIRE(v9.x == 6.0f);
		REQUIRE(v9.y == 2.0f);
		REQUIRE(v9.z == -1.0f);


		// inf
		// vec3 vinf = vec3::inf();
		// REQUIRE(vinf.is_inf() == true);
		// 
		// vinf = vec3(0, 1, std::numeric_limits<float>::infinity());
		// REQUIRE(vinf.has_inf() == true);
		// 
		// // nan
		// vec3 vnan = vec3::nan();
		// REQUIRE(vnan.is_nan() == true);
		// vnan = vec3(std::numeric_limits<float>::quiet_NaN(), 1, 2);
		// REQUIRE(vnan.has_nan() == true);
		// 
		// vec3 vnan2(0, 1, std::numeric_limits<float>::quiet_NaN());
		// REQUIRE(vnan2.has_nan() == true);

		// zero
		vec3 vzero = vec3::zero();
		REQUIRE(vzero.is_zero() == true);


		vzero = vec3(1, 0, 1);
		REQUIRE(vzero.has_zero() == true);

	//	REQUIRE(vec3(1.0).is_nan() == false);
	//	REQUIRE(vec3(1.0).has_nan() == false);
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


		vec3 sres(4.0);
		sres /= 2.0f;
		REQUIRE(sres.equals(vec3(2.0f)));
		sres *= 3.0f;
		REQUIRE(sres.equals(vec3(6.0f)));
		sres += 1.0f;
		REQUIRE(sres.equals(vec3(7.0f)));
		sres -= 5.0f;
		REQUIRE(sres.equals(vec3(2.0f)));

		// ++ --
		vec3 ppv(1, 2, 3);
		REQUIRE(ppv == vec3(1, 2, 3));
		vec3 post = ppv++;
		REQUIRE(post == vec3(1, 2, 3));
		vec3 pre = ++ppv;
		REQUIRE(pre == vec3(3, 4, 5));


		post = ppv--;
		REQUIRE(post == vec3(3, 4, 5));
		pre = --ppv;
		REQUIRE(pre == vec3(1, 2, 3));
	}

	SECTION("compares")
	{
		// lt
		const vec3 a{0.0, 0.0, 0.0};
		const vec3 b{1.0, 1.0, 1.0};

		REQUIRE(a < b);
		REQUIRE(a != b);

		const vec3 c{2.0, 2.0, 2.0};
		const vec3 d{2.0, 2.0, 2.0};
		REQUIRE((c < d) == false);
		REQUIRE(c == d);

		const vec3 e{1.0, 1.0, 1.0};
		const vec3 f{1.0, 1.0, 1.0};
		REQUIRE(false == (e < f));
		REQUIRE(true == (e <= f));
		REQUIRE(false == (e > f));
		REQUIRE(true == (e >= f));
		REQUIRE(e == f);

		const vec3 g{1.0, 1.0, 1.0};
		const vec3 h{2.0, 1.0, 1.0};
		REQUIRE(h >= g);
		REQUIRE(h > g);
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
		REQUIRE_THAT(dist, WithinAbs(7.8740077f, 0.0000001));

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
		REQUIRE_THAT(dotted, WithinAbs(11.719999f, 0.000001));

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

		// reflected
		const vec3 dir{9.0f, -1.0f, 4.0f};
		const vec3 normal{3.14f, 5.11f, -1.34f};
		vec3       reflected = reflect(dir, normal);
		REQUIRE(reflected == vec3(-102.721199f, -182.813797f, 51.677200f));
	}
}

// vec4
TEST_CASE("vec 4", "[vec][vec4][math]")
{
	//
	SECTION("constructor")
	{
		vec4 v{1.0f};
		REQUIRE(v.x == 1.0f);
		REQUIRE(v.y == 1.0f);
		REQUIRE(v.z == 1.0f);
		REQUIRE(v.w == 1.0f);

		vec4 v2{1.0f, 2.0f};
		REQUIRE(v2.x == 1.0f);
		REQUIRE(v2.y == 2.0f);
		REQUIRE(v2.z == 0.0f);
		REQUIRE(v2.w == 0.0f);

		vec4 v3{1.0f, 2.0f, 3.0f};
		REQUIRE(v3.x == 1.0f);
		REQUIRE(v3.y == 2.0f);
		REQUIRE(v3.z == 3.0f);
		REQUIRE(v3.w == 0.0f);

		vec4 v4{1.0f, 2.0f, 3.0f, 4.0f};
		REQUIRE(v4.x == 1.0f);
		REQUIRE(v4.y == 2.0f);
		REQUIRE(v4.z == 3.0f);
		REQUIRE(v4.w == 4.0f);


		vec4 v5{1.0f, 2.0f, 3.0f, 4.0f};
		v5 = -v5;
		REQUIRE(v5.x == -1.0f);
		REQUIRE(v5.y == -2.0f);
		REQUIRE(v5.z == -3.0f);
		REQUIRE(v5.w == -4.0f);

		v5 = +v5;
		REQUIRE(v5 == v5);

		vec4 v6(0.0f);
		REQUIRE(v6.has_zero() == true);
		REQUIRE(v6.is_zero() == true);


		// vec4 v7 = vec4(1.0f).safe_divide(v6);
		// REQUIRE(v7.has_inf() == true);
		// REQUIRE(v7.is_inf() == true);

		vec3 v8(6.0f, 2.0f, 9.0f);
		vec4 v9 = vec4{v8, -1.0f};
		REQUIRE(v9.x == 6.0f);
		REQUIRE(v9.y == 2.0f);
		REQUIRE(v9.z == 9.0f);
		REQUIRE(v9.w == -1.0f);

		std::array<float, 4> vo{2.0f, 4.0f, 6.0f, 8.0f};
		vec4                 v10;
		v10 << vo.data();
		REQUIRE(v10 == vec4(2.0f, 4.0f, 6.0f, 8.0f));
		vo[0] = 4.0f;
		v10 <<= vo.data();
		REQUIRE(v10 == vec4(4.0f, 4.0f, 6.0f, 8.0f));


		std::array<float, 4> vo2{};
		v10 >> vo2.data();
		REQUIRE(vo2[0] == 4.0f);
		REQUIRE(vo2[1] == 4.0f);
		REQUIRE(vo2[2] == 6.0f);
		REQUIRE(vo2[3] == 8.0f);
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

		// v op equ scalar
		vec4 sres(4.0);
		sres /= 2.0f;
		REQUIRE(sres.equals(vec4(2.0f)));
		sres *= 3.0f;
		REQUIRE(sres.equals(vec4(6.0f)));
		sres += 1.0f;
		REQUIRE(sres.equals(vec4(7.0f)));
		sres -= 5.0f;
		REQUIRE(sres.equals(vec4(2.0f)));

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

		// div zero
		// const vec4 div_v_zero = safe_divide(v1, vec4{0.0});
		// const vec4 div_s_zero = safe_divide(v1, 0.0f);
		// REQUIRE(true == div_v_zero.has_inf());
		// REQUIRE(true == div_s_zero.has_inf());

		// ++ --
		vec4 ppv(1, 2, 3, 4);
		REQUIRE(ppv == vec4(1, 2, 3, 4));
		vec4 post = ppv++;
		REQUIRE(post == vec4(1, 2, 3, 4));
		vec4 pre = ++ppv;
		REQUIRE(pre == vec4(3, 4, 5, 6));


		post = ppv--;
		REQUIRE(post == vec4(3, 4, 5, 6));
		pre = --ppv;
		REQUIRE(pre == vec4(1, 2, 3, 4));
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
		REQUIRE_THAT(dist, WithinAbs(11.958261f, 0.000001));

		const vec4 clamped = clamp(v2, 2.0f, 3.0f);
		REQUIRE(true == clamped.equals(vec4{3.0f, 2.0f, 3.0f, 2.0f}));
	}

	SECTION("vec4 cross/dot/length/normal/project/reflect")
	{
		// cross
		const vec4 a{3.0, -3.0, 1.0, 2.0};
		const vec4 b{4.0, 9.0, 2.0, 2.0};

		const auto crossed = cross(a, b);
		REQUIRE(true == crossed.equals({-15.0, -2.0, 39.0}));

		// dot
		const float dotted = dot(a, b);
		REQUIRE_THAT(dotted, WithinAbs(-9.0f, 0.000001));


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

		const vec4 dir{9.0f, -1.0f, 4.0f, -7.0f};
		const vec4 normal{3.14f, 5.11f, -1.34f, 8.1f};
		vec4       reflected = reflect(dir, normal);
		REQUIRE(reflected == vec4(253.354828f, 396.660248f, -100.278816f, 623.342102f));
	}

	SECTION("compares")
	{
		// lt
		const vec4 a{0.0, 0.0, 0.0, 0.0};

		const vec4 b{1.0, 1.0, 1.0, 1.0};

		vec4 c2 = b - b;

		REQUIRE(a == c2);

		REQUIRE(a < b);
		REQUIRE(a != b);

		const vec4 c{1.0, 1.0, 1.0, 1.0};
		const vec4 d{1.0, 1.0, 1.0, 2.0};
		REQUIRE(c < d);
		REQUIRE(c != d);

		const vec4 e{1.0, 1.0, 1.0, 1.0};
		const vec4 f{1.0, 1.0, 1.0, 1.0};
		REQUIRE(false == (e < f));
		REQUIRE(true == (e <= f));
		REQUIRE(false == (e > f));
		REQUIRE(true == (e >= f));
		REQUIRE(e == f);

		const vec4 g{1.0, 1.0, 1.0, 1.0};
		const vec4 h{2.0, 1.0, 1.0, 1.0};
		REQUIRE(h >= g);
		REQUIRE(h > g);

		// inf
		//vec4 vinf = vec4::inf();
		//REQUIRE(vinf.is_inf() == true);
		//
		//vinf = vec4(0, 1, 2, std::numeric_limits<float>::infinity());
		//REQUIRE(vinf.has_inf() == true);

		// nan
		// vec4 vnan = vec4::nan();
		// REQUIRE(vnan.is_nan() == true);
		// 
		// vnan = vec4(std::numeric_limits<float>::quiet_NaN(), 1, 2, 3);
		// REQUIRE(vnan.has_nan() == true);
		// 
		// REQUIRE(vec4(1.0).is_nan() == false);
		// REQUIRE(vec4(1.0).has_nan() == false);
		// 
		// 
		// vec4 vnan2(0, 1, 3, std::numeric_limits<float>::quiet_NaN());
		// REQUIRE(vnan2.has_nan() == true);

		// zero
		vec4 vzero = vec4::zero();
		REQUIRE(vzero.is_zero() == true);

		vzero = vec4(0, 1, 1, 1);
		REQUIRE(vzero.has_zero() == true);
	}
}

// Format
TEST_CASE("vec_n format", "[vec][math]")
{
	SECTION("vec2")
	{
		const vec2 v{2.123f, 3.141f};

		std::string result = std::format("{}", v);
		REQUIRE(result == "vec2(2.12300, 3.14100)"s);
	}
	SECTION("vec3")
	{
		const vec3 v{2.123f, 3.141f, 4.169f};

		std::string result = std::format("{}", v);
		REQUIRE(result == "vec3(2.12300, 3.14100, 4.16900)"s);
	}

	SECTION("vec4")
	{
		const vec4 v{2.123f, 3.141f, 4.169f, 5.f};

		std::string result = std::format("{}", v);
		REQUIRE(result == "vec4(2.12300, 3.14100, 4.16900, 5.00000)"s);
	}
}

TEST_CASE("sin/cos benchmark")
{
#if 0
	using namespace deckard;

	std::vector<float> values;

	std::mt19937                          mt{};
	std::uniform_real_distribution<float> dist{-10240.0f, 10240.0f};

	mt.seed(123);
#ifndef _DEBUG
	constexpr int width = 100'000;
#else
	constexpr int width = 100;
#endif
	for (auto _ : upto(width))
	{
		_;
		values.emplace_back(dist(mt));
	}

	float sin_diff = 0.0f;
	float cos_diff = 0.0f;
	for (const auto& v : values)
	{
		sin_diff += std::sinf(v) - sse::sin(v);
		cos_diff += std::cosf(v) - sse::cos(v);
	}
	REQUIRE_THAT(sin_diff, WithinAbs(0.0f, 0.00002));
	REQUIRE_THAT(cos_diff, WithinAbs(0.0f, 0.00002));


	BENCHMARK("std::sin")
	{
		float std_result = 0.0f;
		for (float v : values)
		{
			std_result += std::sinf(v);
		}
		return std_result;
	};

	BENCHMARK("std::cos")
	{
		float std_result = 0.0f;
		for (float v : values)
		{
			std_result += std::cosf(v);
		}
		return std_result;
	};

	BENCHMARK("sse::sin")
	{
		float sse_result = 0.0f;
		for (float v : values)
		{
			sse_result += sse::sin(v);
		}
		return sse_result;
	};

	BENCHMARK("sse::cos")
	{
		float sse_result = 0.0f;
		for (float v : values)
		{
			sse_result += sse::cos(v);
		}
		return sse_result;
	};
#endif
}

TEST_CASE("sqrt test", "[math]")
{
#if 0
	using namespace deckard;

	std::vector<float> sse;
	std::vector<float> generic;

	std::mt19937                          mt{};
	std::uniform_real_distribution<float> dist{0.0f, 1024.0f};

	mt.seed(123);
#ifndef _DEBUG
	constexpr int width = 100'000;
#else
	constexpr int width = 100;
#endif
	for (auto _ : upto(width))
	{
		_;
		sse.emplace_back(dist(mt));
	}

	generic = sse;

	BENCHMARK("generic sqrt")
	{
		float result = 0.0f;
		for (float v : generic)
		{
			result += std::sqrtf(v);
		}
		return result;
	};

	BENCHMARK("sse sqrt")
	{
		float result = 0.0f;
		for (float v : sse)
		{
			result += sse::sqrt(v);
		}
		return result;
	};
#endif
}

TEST_CASE("vec4 benchmark", "[vec][benchmark]")
{
#if 0
	using namespace deckard;
	using namespace deckard::math;

	std::mt19937                          mt{};
	std::uniform_real_distribution<float> dist{-1'024.0f, 1'024.0f};

	std::vector<sse::vec4>       sse;
	std::vector<vec_n<float, 4>> generic;
#ifndef _DEBUG
	constexpr int width = 100'000;
#else
	constexpr int width = 100;
#endif

	mt.seed(123);
	for (auto _ : upto(width))
	{
		_;
		sse::vec4 v1(dist(mt), dist(mt), dist(mt), dist(mt));
		sse.emplace_back(v1);
	}
	mt.seed(123);
	for (auto _ : upto(width))
	{
		_;
		vec_n<float, 4> v1(dist(mt), dist(mt), dist(mt), dist(mt));
		generic.emplace_back(v1);
	}

	auto gen_result = *generic.begin();
	BENCHMARK("generic vec4")
	{

		for (const auto& v : generic)
		{
			gen_result += v;
			gen_result *= v;
			gen_result -= v;
			gen_result /= v;
			gen_result = reflect(gen_result, vec_n<float, 4>(1.0, 0.0, 1.0, 0.0));
			gen_result.normalize();
		}
		return gen_result;
	};

	auto sse_result = *sse.begin();
	BENCHMARK("sse vec4")
	{
		for (const auto& v : sse)
		{
			sse_result += v;
			sse_result *= v;
			sse_result -= v;
			sse_result /= v;
			sse_result = reflect(sse_result, vec4(1.0, 0.0, 1.0, 0.0));
			sse_result.normalize();
		}

		return sse_result;
	};

	REQUIRE(gen_result[0] == sse_result[0]);
	REQUIRE(gen_result[1] == sse_result[1]);
	REQUIRE(gen_result[2] == sse_result[2]);
	REQUIRE(gen_result[3] == sse_result[3]);
#endif
}
