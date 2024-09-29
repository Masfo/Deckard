#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;

using namespace deckard::math;

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

	SECTION("vec3 add/sub/mul/div")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		REQUIRE((v1 + v2).vec4_part() == vec4(-1.23312f, 0.14046f, 0.82849f, 1.18328f));
		REQUIRE((v1 - v2).vec4_part() == vec4(-0.20345f, 0.48078f, 0.06038f, -0.31137f));
		REQUIRE((v1 * v2).vec4_part() == vec4(-0.56632f, 0.20500f, 0.78171f, -0.16183f));
		REQUIRE((v1 / 0.75f).vec4_part() == vec4(-0.95772f, 0.41416f, 0.59258f, 0.58127f));
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
}
