#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;
using namespace deckard::math;
using namespace Catch::Matchers;
using namespace std::string_literals;

TEST_CASE("matrix generic", "[matrix]")
{

	SECTION("constructors")
	{
		mat4 identity(1.0f);

		REQUIRE(identity[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		REQUIRE(identity[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		REQUIRE(identity[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		REQUIRE(identity[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		REQUIRE(m[0] == vec4{1.0f, 2.0f, 3.0f, 4.0f});
		REQUIRE(m[1] == vec4{5.0f, 6.0f, 7.0f, 8.0f});
		REQUIRE(m[2] == vec4{9.0f, 10.0f, 11.0f, 12.0f});
		REQUIRE(m[3] == vec4{13.0f, 14.0f, 15.0f, 16.0f});
	}

	SECTION("multiply with identity")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m * mat4::identity();

		REQUIRE(mul[0] == vec4{1.0f, 2.0f, 3.0f, 4.0f});
		REQUIRE(mul[1] == vec4{5.0f, 6.0f, 7.0f, 8.0f});
		REQUIRE(mul[2] == vec4{9.0f, 10.0f, 11.0f, 12.0f});
		REQUIRE(mul[3] == vec4{13.0f, 14.0f, 15.0f, 16.0f});
	}

	SECTION("multiply with self")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m * m;

		REQUIRE(mul[0] == vec4{90.0f, 100.0f, 110.0f, 120.0f});
		REQUIRE(mul[1] == vec4{202.0f, 228.0f, 254.0f, 280.0f});
		REQUIRE(mul[2] == vec4{314.0f, 356.0f, 398.0f, 440.0f});
		REQUIRE(mul[3] == vec4{426.0f, 484.0f, 542.0f, 600.0f});
	}

	SECTION("add")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m + m;
		REQUIRE(mul[0] == vec4{2.0f, 4.0f, 6.0f, 8.0f});
		REQUIRE(mul[1] == vec4{10.0f, 12.0f, 14.0f, 16.0f});
		REQUIRE(mul[2] == vec4{18.0f, 20.0f, 22.0f, 24.0f});
		REQUIRE(mul[3] == vec4{26.0f, 28.0f, 30.0f, 32.0f});
	}

	SECTION("sub")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m - m;
		REQUIRE(mul[0] == vec4::zero());
		REQUIRE(mul[1] == vec4::zero());
		REQUIRE(mul[2] == vec4::zero());
		REQUIRE(mul[3] == vec4::zero());
	}

	SECTION("transpose")
	{
		const mat4 real{1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16};
		mat4       m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

		auto tranposed = transpose(m);
		REQUIRE(real == tranposed);
	}

	SECTION("multiply vec4")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		const vec4 v(2.0f, 3.5f, 4.5f, 6.0f);

		REQUIRE(m * v == vec4{138.0f, 154.0f, 170.0f, 186.0f});
		REQUIRE(v * m == vec4{46.5f, 110.5f, 174.5f, 238.5f});
	}


	SECTION("divide by scalar")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		const auto div2 = m / 2;
		REQUIRE(div2[0] == vec4{0.5f, 1.0f, 1.5f, 2.0f});
		REQUIRE(div2[1] == vec4{2.5f, 3.0f, 3.5f, 4.0f});
		REQUIRE(div2[2] == vec4{4.5f, 5.0f, 5.5f, 6.0f});
		REQUIRE(div2[3] == vec4{6.5f, 7.0f, 7.5f, 8.0f});
	}

	SECTION("negate")
	{
		const mat4 a(-4, mat4::fill);
		mat4       m(4, mat4::fill);


		REQUIRE(a == -m);
	}

	SECTION("equals")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.0f};
		const mat4 m2{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.001f};

		REQUIRE(m == m);
		REQUIRE(m != m2);
	}

	SECTION("lookat_rh")
	{
		mat4 lat = lookat_rh(
		  vec3(4, 10, 3), //
		  vec3(0, 0, 0),  //
		  vec3(0, 1, 0)   //
		);


		REQUIRE(lat[0] == vec4{0.59999f, -0.71554f, 0.35777f, 0.0f});
		REQUIRE(lat[1] == vec4{0.0f, 0.44721f, 0.89442f, 0.0f});
		REQUIRE(lat[2] == vec4{-0.80000f, -0.53665f, 0.26832f, 0.0f});
		REQUIRE(lat[3] == vec4{0.0f, -0.0f, -11.18033f, 1.0f});
	}

	SECTION("scale")
	{
		mat4 scal{1.0f, 2.0f, 4.0f, 8.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.2f, 2.4f, 3.6f, 4.8f, 8.0f, 8.0f, 8.0f, 8.0f};
		scal = scale(scal, vec3(2.0f, 3.0f, 4.0f));

		REQUIRE(scal[0] == vec4{2.0f, 4.0f, 8.0f, 16.0f});
		REQUIRE(scal[1] == vec4{0.6f, 1.2f, 1.8f, 2.4f});
		REQUIRE(scal[2] == vec4{4.8f, 9.6f, 14.4f, 19.2f});
		REQUIRE(scal[3] == vec4{8.0f, 8.0f, 8.0f, 8.0f});
	}

	SECTION("translate")
	{
		mat4 transl{1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 10, 10, 10, 10};
		transl = translate(transl, vec3(2.0f, 30.0f, 4.0f));

		REQUIRE(transl[0] == vec4{1.0f, 1.0f, 1.0f, 1.0f});
		REQUIRE(transl[1] == vec4{2.0f, 2.0f, 2.0f, 2.0f});
		REQUIRE(transl[2] == vec4{3.0f, 3.0f, 3.0f, 3.0f});
		REQUIRE(transl[3] == vec4{84.0f, 84.0f, 84.0f, 84.0f});

		mat4 ViewTranslate = translate(mat4(1.0f), vec3(0.0f, 0.0f, -5.0f));
		REQUIRE(transl[0] == vec4{1.0f, 1.0f, 1.0f, 1.0f});
		REQUIRE(transl[1] == vec4{2.0f, 2.0f, 2.0f, 2.0f});
		REQUIRE(transl[2] == vec4{3.0f, 3.0f, 3.0f, 3.0f});
		REQUIRE(transl[3] == vec4{84.0f, 84.0f, 84.0f, 84.0f});
	}

	SECTION("perspective")
	{
		auto persp = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

		REQUIRE(persp[0] == vec4{0.61386f, 0.0f, 0.0f, 0.0f});
		REQUIRE(persp[1] == vec4{0.0f, 1.09131f, 0.0f, 0.0f});
		REQUIRE(persp[2] == vec4{0.0f, 0.0f, -1.00200f, -1.0f});
		REQUIRE(persp[3] == vec4{0.0f, 0.0f, -0.20020f, 0.0f});
	}

	SECTION("ortho")
	{
		auto orthopers = ortho(0, 400, 0, 400, -1, 1);

		REQUIRE(orthopers[0] == vec4{0.00500f, 0.0f, 0.0f, 0.0f});
		REQUIRE(orthopers[1] == vec4{0.0f, 0.00500f, 0.0f, 0.0f});
		REQUIRE(orthopers[2] == vec4{0.0f, 0.0f, -1.0f, 0.0f});
		REQUIRE(orthopers[3] == vec4{-1.0f, -1.0f, -0.0f, 1.0f});
	}


	SECTION("frustum")
	{
		auto fr = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

		REQUIRE(fr[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		REQUIRE(fr[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		REQUIRE(fr[2] == vec4{0.0f, 0.0f, -1.02020f, -1.0f});
		REQUIRE(fr[3] == vec4{0.0f, 0.0f, -2.02020f, 0.0f});
	}

	SECTION("Rotate X")
	{

		mat4 rot = rotate(mat4(1.0f), 2.5f, vec3(-1.0f, 0.0f, 0.0f));


		REQUIRE(rot[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		REQUIRE(rot[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		REQUIRE(rot[2] == vec4{0.0f, 0.59847f, -0.80114f, 0.0f});
		REQUIRE(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("Rotate Y")
	{
		mat4 rot = rotate(mat4(1.0f), -0.5f, vec3(0.0f, 1.0f, 0.0f));

		REQUIRE(rot[0] == vec4{0.87758f, 0.0f, 0.47943f, 0.0f});
		REQUIRE(rot[1] == vec4{0.0f, 1.0f, 0.0, 0.0f});
		REQUIRE(rot[2] == vec4{-0.47943f, 0.0f, 0.87758f, 0.0f});
		REQUIRE(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("Rotate Z")
	{
		mat4 rot = rotate(mat4(1.0f), 3.75f, vec3(0.0f, 0.0f, 1.0f));

		REQUIRE(rot[0] == vec4{-0.82056f, -0.57156f, 0.0f, 0.0f});
		REQUIRE(rot[1] == vec4{0.57156f, -0.82056f, 0.0, 0.0f});
		REQUIRE(rot[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		REQUIRE(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}


	SECTION("inverse")
	{
		mat4 Projection = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		mat4 inv        = inverse(Projection);

		REQUIRE(inv[0] == vec4{1.62903f, 0.0f, -0.0f, 0.0f});
		REQUIRE(inv[1] == vec4{0.0f, 0.91633f, 0.0f, -0.0f});
		REQUIRE(inv[2] == vec4{-0.0f, 0.0f, -0.0f, -4.99500f});
		REQUIRE(inv[3] == vec4{0.0f, -0.0f, -1.0f, 5.00500f});
	}

	SECTION("determinant")
	{
		mat4 Projection = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		mat4 inv        = inverse(Projection);

		REQUIRE_THAT(determinant(inv), Catch::Matchers::WithinAbs(-7.45620f, 0.00001f));
	}


	SECTION("full MVP")
	{

		mat4 Projection = perspective(to_radians(45.0f), 4.0f / 3.0f, 0.1f, 100.f);
		REQUIRE(Projection[0] == vec4{1.810660f, 0.0f, 0.0f, 0.0f});
		REQUIRE(Projection[1] == vec4{0.0f, 2.41421f, 0.0f, 0.0f});
		REQUIRE(Projection[2] == vec4{0.0f, 0.0f, -1.00200f, -1.0f});
		REQUIRE(Projection[3] == vec4{0.0f, 0.0f, -0.20020f, 0.0f});

		mat4 ViewTranslate = translate(mat4(1.0f), vec3(0.0f, 0.0f, -5.0f));
		REQUIRE(ViewTranslate[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		REQUIRE(ViewTranslate[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		REQUIRE(ViewTranslate[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		REQUIRE(ViewTranslate[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});

		mat4 ViewRotateX = rotate(ViewTranslate, 2.5f, vec3(-1.0f, 0.0f, 0.0f));
		REQUIRE(ViewRotateX[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		REQUIRE(ViewRotateX[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		REQUIRE(ViewRotateX[2] == vec4{0.0f, 0.59847f, -0.80114f, 0.0f});
		REQUIRE(ViewRotateX[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});

		mat4 View = rotate(ViewRotateX, -2.0f, vec3(0.0f, 1.0f, 0.0f));
		REQUIRE(View[0] == vec4{-0.41614f, 0.54418f, -0.72847f, 0.0f});
		REQUIRE(View[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		REQUIRE(View[2] == vec4{-0.90929f, -0.24905f, 0.33339f, 0.0f});
		REQUIRE(View[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});


		mat4 Model = scale(mat4(1.0f), vec3(0.5f));
		REQUIRE(Model[0] == vec4{0.5f, 0.0f, 0.0f, 0.0f});
		REQUIRE(Model[1] == vec4{0.0f, 0.5f, 0.0f, 0.0f});
		REQUIRE(Model[2] == vec4{0.0f, 0.0f, 0.5f, 0.0f});
		REQUIRE(Model[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		mat4 MVP = Projection * View * Model;
		REQUIRE(MVP[0] == vec4{-0.37675f, 0.65689f, 0.36496f, 0.36423f});
		REQUIRE(MVP[1] == vec4{0.0f, -0.96706f, 0.29983f, 0.29923f});
		REQUIRE(MVP[2] == vec4{-0.82321f, -0.30063f, -0.16703f, -0.16670f});
		REQUIRE(MVP[3] == vec4{0.0f, 0.0f, 4.80981f, 5.0f});


		auto inv = inverse(MVP);
		REQUIRE(inv[0] == vec4{-0.45966f, 0.0f, -1.00438f, 0.0f});
		REQUIRE(inv[1] == vec4{0.450821f, -0.66368f, -0.20632f, 0.0f});
		REQUIRE(inv[2] == vec4{36.38748f, 29.89369f, -16.65300f, -4.99500f});
		REQUIRE(inv[3] == vec4{-35.00337f, -28.75659f, 16.01955f, 5.00500f});
	}

	SECTION("project/unproject")
	{
		vec3 point(1.0f, -2.0f, 3.0f);

		mat4 model = translate(mat4(1.0f), vec3(0.0f, 0.0f, -10.0f));
		vec4 viewport(0.0f, 0.0f, 640.0f, 360.0f);
		mat4 projection = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);


		vec3 projected   = project(point, model, projection, viewport);
		vec3 unprojected = unproject(projected, model, projection, viewport);

		REQUIRE(projected == vec3(365.71429f, 128.57142f, 0.86580f));

		REQUIRE(point == unprojected);
	}


	SECTION("unproject")
	{
		vec2  mouse{100.0f, 124.0f};
		float width      = 1920.0f;
		float height     = 1080.0f;
		mat4  Projection = perspective(to_radians(85.0f), width / height, 0.1f, 100.0f);

		mat4 ViewTranslate = translate(mat4(1.0f), vec3(0.0f, 0.0f, -5.0f));
		mat4 ViewRotateX   = rotate(ViewTranslate, 2.5f, vec3(-1.0f, 0.0f, 0.0f));
		mat4 View          = rotate(ViewRotateX, -2.0f, vec3(0.0f, 1.0f, 0.0f));

		vec3 mouse_world_nearplane = unproject(vec3(mouse.x * width, mouse.y * height, 0.0f), View, Projection, vec4(0, 0, width, height));

		vec3 mouse_world_farplane = unproject(vec3(mouse.x * width, mouse.y * height, 1.0f), View, Projection, vec4(0, 0, width, height));


		REQUIRE(mouse_world_nearplane == vec3(-4.7432f, -21.0652f, -33.4807f));
		REQUIRE(mouse_world_farplane == vec3(-1109.41699f, -18156.4727f, -35302.8594f));
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
