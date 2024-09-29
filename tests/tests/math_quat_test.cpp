#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace Catch::Matchers;

using namespace deckard::math;
using namespace std::string_literals;

TEST_CASE("quatertion", "[quaternion]")
{
	SECTION("identity")
	{
		quat q;

		REQUIRE(q[0] == 0.0f);
		REQUIRE(q[1] == 0.0f);
		REQUIRE(q[2] == 0.0f);
		REQUIRE(q[3] == 1.0f);
	}

	SECTION("vec3 init")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		REQUIRE(q.vec4_part() == vec4(-0.71828f, 0.31062f, 0.44443f, 0.43595f));
	}

	SECTION("vec3 add/sub/mul/div/scale")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		REQUIRE((v1 + v2).vec4_part() == vec4(-1.23312f, 0.14046f, 0.82849f, 1.18328f));
		REQUIRE((v1 - v2).vec4_part() == vec4(-0.20345f, 0.48078f, 0.06038f, -0.31137f));
		REQUIRE((v1 * v2).vec4_part() == vec4(-0.56632f, 0.20500f, 0.78171f, -0.16183f));
		REQUIRE((v1 / 0.75f).vec4_part() == vec4(-0.95772f, 0.41416f, 0.59258f, 0.58127f));

		REQUIRE((v1 * 1.25f).vec4_part() == vec4(-0.89786f, 0.38828f, 0.55554f, 0.54494f));
		REQUIRE((1.25f * v1).vec4_part() == vec4(-0.89786f, 0.38828f, 0.55554f, 0.54494f));
	}

	SECTION("inplace")
	{
		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));
		quat       v3 = v1;

		REQUIRE(v3.vec4_part() == vec4(-0.71828f, 0.31062f, 0.44443f, 0.43595f));


		v3 *= 2.0f;
		REQUIRE(v3.vec4_part() == vec4(-1.43657f, 0.62124f, 0.88887f, 0.87191f));

		v3 += v1;
		REQUIRE(v3.vec4_part() == vec4(-2.15486f, 0.93187f, 1.33331f, 1.30786f));
		v3 -= v2;
		REQUIRE(v3.vec4_part() == vec4(-1.64003f, 1.10202f, 0.94925f, 0.56053f));

		v3 *= v2;
		REQUIRE(v3.vec4_part() == vec4(-0.92946f, 0.86934f, 1.77110f, -0.60249f));

		v3 /= 0.75f;
		REQUIRE(v3.vec4_part() == vec4(-1.23928f, 1.15912f, 2.36147f, -0.80332f));
	}

	SECTION("inverse")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		REQUIRE(inverse(v1 + v2).vec4_part() == vec4(0.34000f, -0.03873f, -0.22843f, 0.32625f));
	}

	SECTION("length")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		REQUIRE_THAT(length(v1 + v2), Catch::Matchers::WithinAbs(1.90443f, 0.00001f));
	}

	SECTION("dot")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		REQUIRE_THAT(dot(v1, v2), Catch::Matchers::WithinAbs(0.81343f, 0.00001f));
	}

	SECTION("normalize")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		REQUIRE(normalize(v1 + v2).vec4_part() == vec4(-0.64750f, 0.07376f, 0.43503f, 0.62133f));
	}


	SECTION("conjugate")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		REQUIRE(q.conjugate().vec4_part() == vec4(0.71828f, -0.31062f, -0.44443f, 0.43595f));
	}

	SECTION("rotate")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		quat qrot = sse::rotate(q, 45.0f, vec3(0, 1, 0));

		REQUIRE(qrot.vec4_part() == vec4(0.84380f, -0.48365f, -0.03820f, -0.22939f));
	}

	SECTION("to rotation mat4")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		mat4 result(
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
		  0.00000f,
		  0.00000f,
		  0.00000f,
		  0.00000f,
		  1.00000f);

		mat4 q_mat = q.get_rotation_matrix();

		REQUIRE(result == q_mat);
	}


	SECTION("format")
	{
		vec3        v(1.0f, 2.0f, 3.0f);
		quat        q(v);
		std::string f = std::format("{}", q);

		REQUIRE(f == "quat(-0.71829, 0.31062, 0.44444, 0.43595)"s);
	}
}
