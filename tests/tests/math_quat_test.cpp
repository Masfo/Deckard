﻿#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace Catch::Matchers;

using namespace deckard::math;
using namespace std::string_literals;

TEST_CASE("quatertion", "[quaternion]")
{
	SECTION("default constructor")
	{
		quat q;
		CHECK(q[0] == 0.0f);
		CHECK(q[1] == 0.0f);
		CHECK(q[2] == 0.0f);
		CHECK(q[3] == 1.0f);
	}

	SECTION("init vec3")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		CHECK(q == quat(-0.71828f, 0.31062f, 0.44443f, 0.43595f));
	}

	SECTION("vec3 neg/pos")
	{
		quat q(1.0f, -2.0f, -3.0f, 4.0f);

		CHECK(-q == quat(-1.0f, 2.0f, 3.0f, -4.0f));
		CHECK(+q == quat(1.0f, -2.0f, -3.0f, 4.0f));
	}

	SECTION("vec3 add/sub/mul/div/scale")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK((v1 + v2) == quat(-1.23312f, 0.14046f, 0.82849f, 1.18328f));
		CHECK((v1 - v2) == quat(-0.20345f, 0.48078f, 0.06038f, -0.31137f));
		CHECK((v1 * v2) == quat(-0.56632f, 0.20500f, 0.78171f, -0.16183f));
		
		CHECK((v1 / 0.75f) == quat( -0.95772f, 0.41416f, 0.59258f, 0.58127f));
		CHECK((0.75f / v1) == quat(-0.95772f, 0.41416f, 0.59258f, 0.58127f));

		CHECK((v1 * 1.25f) == quat(-0.89786f, 0.38828f, 0.55554f, 0.54494f));
		CHECK((1.25f * v1) == quat(-0.89786f, 0.38828f, 0.55554f, 0.54494f));
	}

#if 0




	SECTION("inplace")
	{
		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));
		quat       v3 = v1;

		CHECK(v3 == quat(0.43595f, -0.71828f, 0.31062f, 0.44443f));


		v3 *= 2.0f;
		CHECK(v3 == quat(0.87191f, -1.43657f, 0.62124f, 0.88887f));

		v3 += v1;
		CHECK(v3 == quat(1.30786f, -2.15486f, 0.93187f, 1.33331f));
		v3 -= v2;
		CHECK(v3 == quat(0.56053f, -1.64003f, 1.10202f, 0.94925f));

		v3 *= v2;
		CHECK(v3 == quat(-0.60249f, -0.92946f, 0.86934f, 1.77110f));

		v3 /= 0.75f;
		CHECK(v3 == quat(-0.80332f, -1.23928f, 1.15912f, 2.36147f));
	}

	SECTION("inverse")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(inverse(v1 + v2) == quat(0.32625f, 0.34000f, -0.03873f, -0.22843f));
	}

	SECTION("cross")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(cross(v1, v2) == quat(-0.16183f, -0.56632f, 0.20500f, 0.78171f));
	}

	SECTION("length")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK_THAT(length(v1 + v2), Catch::Matchers::WithinAbs(1.90443f, 0.00001f));
	}

	SECTION("dot")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK_THAT(dot(v1, v2), Catch::Matchers::WithinAbs(0.81343f, 0.00001f));
	}

	SECTION("normalize")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(normalize(v1 + v2) == quat(0.62133f, -0.64750f, 0.07376f, 0.43503f));
	}


	SECTION("conjugate")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		CHECK(q.conjugate() == quat(0.43595f, 0.71828f, -0.31062f, -0.44443f));
	}

	SECTION("rotate")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		quat qrot = rotate(q, 45.0f, vec3(0, 1, 0));

		CHECK(qrot == quat(-0.22939f, 0.84380f, -0.48365f, -0.03820f));
	}

	SECTION("lerp")
	{

		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));

		quat l = lerp(v1, v2, 0.0f);
		CHECK(l == quat(0.43595f, -0.71828f, 0.31062f, 0.44443f));

		l = lerp(v1, v2, 0.5f);
		CHECK(l == quat(0.59163f, -0.61656f, 0.07023f, 0.41424f));

		l = lerp(v1, v2, 1.0f);
		CHECK(l == quat(0.74732f, -0.51483f, -0.17015f, 0.38405f));
	}

	SECTION("slerp")
	{
		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));

		quat l = slerp(v1, v2, 0.0f);
		CHECK(l == quat(0.43595f, -0.71828f, 0.31062f, 0.44443f));

		l = slerp(v1, v2, 0.5f);
		CHECK(l == quat(0.62133f, -0.64750f, 0.07376f, 0.43503f));

		l = slerp(v1, v2, 1.0f);
		CHECK(l == quat(0.74732f, -0.51483f, -0.17015f, 0.38405f));
	}


	SECTION("to rotation mat4")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		mat4 result(
		  0.00000f,
		  0.00000f,
		  0.00000f,
		  1.00000f,

		  0.41198f,
		  -0.05873f,
		  -0.90930f,
		  0.00000f,

		  -0.83374f,
		  -0.42692f,
		  -0.35018f,
		  0.00000f,

		  -0.36763f,
		  0.90238f,
		  -0.22485f,
		  0.00000f);

		mat4 q_mat = q.get_rotation_matrix();

		CHECK(result == q_mat);
	}


	SECTION("format")
	{
		vec3        v(1.0f, 2.0f, 3.0f);
		quat        q(v);
		std::string f = std::format("{}", q);

		CHECK(f == "quat(0.43595, -0.71829, 0.31062, 0.44444)"s);
	}
#endif
}
