#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.math;

using namespace Catch::Matchers;

using namespace deckard;
using namespace deckard::math;
using namespace std::string_literals;

TEST_CASE("quatertion", "[quaternion]")
{
	SECTION("default constructor")
	{
		quat q;
		CHECK(q[0] == 1.0f);
		CHECK(q[1] == 0.0f);
		CHECK(q[2] == 0.0f);
		CHECK(q[3] == 0.0f);
	}

	SECTION("init vec3")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		quat expected(0.43595284f, -0.71828705f, 0.31062245f, 0.44443506f);

		CHECK(q == expected);
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

		CHECK((v1 + v2) == quat(1.1832786f, -1.2331222f, 0.14046498f, 0.8284862f));
		CHECK((v1 - v2) == quat(-0.31137294f, -0.20345187f, 0.48077995f, 0.060383886f));
		CHECK((v1 * v2) == quat(-0.1618317f, -0.5663194f, 0.20500372f, 0.7817072f));

		CHECK((v1 / 0.75f) == quat(0.58127f, -0.95772f, 0.41416f, 0.59258f));
		CHECK((0.75f / v1) == quat(0.58127f, -0.95772f, 0.41416f, 0.59258f));

		CHECK((v1 * 1.25f) == quat(0.54494f, -0.89786f, 0.38828f, 0.55554f));
		CHECK((1.25f * v1) == quat(0.54494f, -0.89786f, 0.38828f, 0.55554f));
	}

	SECTION("assign")
	{
		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat       v2 = v1;

		CHECK(v1 == quat(0.43595284f, -0.71828705f, 0.31062245f, 0.44443506f));
		CHECK(v2 == quat(0.43595284f, -0.71828705f, 0.31062245f, 0.44443506f));
	}


	SECTION("inplace")
	{
		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));
		quat       v3 = v1;

		CHECK(v3 == quat(0.43595284f, -0.71828705f, 0.31062245f, 0.44443506f));


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

	SECTION("conjugate")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		CHECK(conjugate(q) == quat(0.43595284f, 0.71828705f, -0.31062245f, -0.44443506f));
	}

	SECTION("dot")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK_THAT(dot(v1, v2), Catch::Matchers::WithinAbs(0.81343f, 0.00001f));
	}

	SECTION("inverse")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(inverse(v1 + v2) == quat(0.3262544f, 0.33999735f, -0.038729105f, -0.22843081f));
	}

	SECTION("cross")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(cross(v1, v2) == quat(-0.1618317f, -0.5663194f, 0.20500372f, 0.7817072f));
	}


	SECTION("normalize")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(v1 == quat(0.43595284f, -0.71828705f, 0.31062245f, 0.44443506f));
		CHECK(v2 == quat(0.7473258f, -0.5148352f, -0.17015748f, 0.38405117f));

		quat v1v2 = v1 + v2;
		CHECK(v1v2 == quat(1.1832786f, -1.2331222f, 0.14046498f, 0.8284862f));

		quat norm = normalize(v1v2);
		CHECK(norm == quat(0.6213291f, -0.6475016f, 0.07375692f, 0.4350308f));
	}

	SECTION("normalize degenenate")
	{
		quat degen(0.0f, 0.0f, 0.0f, 0.0f);
		quat norm = normalize(degen);

		CHECK(norm[0] == 0.0f);
		CHECK(norm[1] == 0.0f);
		CHECK(norm[2] == 0.0f);
		CHECK(norm[3] == 1.0f);
	}

	SECTION("normalize nan")
	{
		f32  n = limits::nan<f32>;
		quat degen(n, n, n, n);
		quat result = normalize(degen);

		CHECK(result[0] == 0.0f);
		CHECK(result[1] == 0.0f);
		CHECK(result[2] == 0.0f);
		CHECK(result[3] == 1.0f);
	}

	SECTION("length")
	{
		quat v1(vec3(1.0f, 2.0f, 3.0f));
		quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK_THAT(length(v1 + v2), Catch::Matchers::WithinAbs(1.90443f, 0.00001f));
	}

	SECTION("rotate")
	{
		const vec3 v(0.0f, 0.0f, 0.0f);
		const quat q(v);

		quat rot = rotate(q, to_radians(90.0f), vec3(1, 0, 0));
		CHECK(rot == quat(0.70710f, 0.70710f, 0.0f, 0.0f));

		quat xrot = rotate_x(q, to_radians(90.0f));
		CHECK(xrot == quat(0.70710f, 0.70710f, 0.0f, 0.0f));

		quat yrot = rotate_y(q, to_radians(90.0f));
		CHECK(yrot == quat(0.70710f, 0.0f, 0.70710f, 0.0f));

		quat zrot = rotate_z(q, to_radians(90.0f));
		CHECK(zrot == quat(0.70710f, 0.0f, 0.0f, 0.70710f));
	}


	SECTION("format")
	{
		quat        q(vec3(1.0f, 2.0f, 3.0f));
		std::string f = std::format("{}", q);

		CHECK(f == "quat(0.43595, -0.71829, 0.31062, 0.44444)"s);
	}

	SECTION("lerp")
	{

		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(v1 == quat(0.43595284f, -0.71828705f, 0.31062245f, 0.44443506f));
		CHECK(v2 == quat(0.7473258f, -0.5148352f, -0.17015748f, 0.38405117f));

		CHECK(lerp(v1, v2, 0.0f) == v1);
		CHECK(lerp(v1, v2, 0.5f) == quat(0.5916393f, -0.6165611f, 0.07023249f, 0.4142431f));
		CHECK(lerp(v1, v2, 1.0f) == v2);
	}

	SECTION("slerp")
	{
		const quat v1(vec3(1.0f, 2.0f, 3.0f));
		const quat v2(vec3(2.0f, 3.0f, 4.0f));

		CHECK(v1 == quat(0.435952f, -0.718287f, 0.310622f, 0.444435f));
		CHECK(v2 == quat(0.747325f, -0.514835f, -0.1701574f, 0.3840511f));

		CHECK(slerp(v1, v2, 0.0f) == v1);
		CHECK(slerp(v1, v2, 0.5f) == quat(0.621329f, -0.647501f, 0.07375692f, 0.435030f));
		CHECK(slerp(v1, v2, 1.0f) == v2);
	}

	SECTION("to rotation mat4")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);

		mat4 result(vec4(0.411982f, -0.0587267f, -0.909297f, 0.00000f),
					vec4(-0.833738f, -0.426918f, -0.350176f, 0.00000f),
					vec4(-0.36763f, 0.902382f, -0.224845f, 0.00000f),
					vec4(0.00000f, 0.00000f, 0.00000f, 1.00000f));

		mat4 qresult = q.to_mat4();

		CHECK(result == qresult);
	}


	SECTION("from rotation mat4")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);
		mat4 m = q.to_mat4();
		quat r = q.from_mat4(m);
		CHECK(r == q);
	}

	SECTION("round-trip matrix")
	{
		vec3 v(1.0f, 2.0f, 3.0f);
		quat q(v);
		quat q2(q.to_mat4());

		vec3 point(1.0f, 0.0f, 0.0f);
		vec3 r1 = q * point;
		vec3 r2 = q2 * point;

		CHECK(r1 == r2);
	}


	SECTION("format")
	{
		vec3        v(1.0f, 2.0f, 3.0f);
		quat        q(v);
		std::string f = std::format("{}", q);

		CHECK(f == "quat(0.43595, -0.71829, 0.31062, 0.44444)"s);
	}
}
