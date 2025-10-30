#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.math;
using namespace deckard::math;
using namespace Catch::Matchers;
using namespace std::string_literals;

TEST_CASE("matrix generic", "[matrix]")
{
	SECTION("default constructor") 
	{ 
		mat4 identity;
		CHECK(identity[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(identity[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		CHECK(identity[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("constructors")
	{
		mat4 identity(1.0f);

		CHECK(identity[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(identity[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		CHECK(identity[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		CHECK(m[0] == vec4{1.0f, 2.0f, 3.0f, 4.0f});
		CHECK(m[1] == vec4{5.0f, 6.0f, 7.0f, 8.0f});
		CHECK(m[2] == vec4{9.0f, 10.0f, 11.0f, 12.0f});
		CHECK(m[3] == vec4{13.0f, 14.0f, 15.0f, 16.0f});
	}

	SECTION("multiply with identity")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m * mat4::identity();

		CHECK(mul[0] == vec4{1.0f, 2.0f, 3.0f, 4.0f});
		CHECK(mul[1] == vec4{5.0f, 6.0f, 7.0f, 8.0f});
		CHECK(mul[2] == vec4{9.0f, 10.0f, 11.0f, 12.0f});
		CHECK(mul[3] == vec4{13.0f, 14.0f, 15.0f, 16.0f});
	}

	SECTION("multiply with self")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m * m;

		CHECK(mul[0] == vec4{90.0f, 100.0f, 110.0f, 120.0f});
		CHECK(mul[1] == vec4{202.0f, 228.0f, 254.0f, 280.0f});
		CHECK(mul[2] == vec4{314.0f, 356.0f, 398.0f, 440.0f});
		CHECK(mul[3] == vec4{426.0f, 484.0f, 542.0f, 600.0f});
	}

	SECTION("add")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m + m;
		CHECK(mul[0] == vec4{2.0f, 4.0f, 6.0f, 8.0f});
		CHECK(mul[1] == vec4{10.0f, 12.0f, 14.0f, 16.0f});
		CHECK(mul[2] == vec4{18.0f, 20.0f, 22.0f, 24.0f});
		CHECK(mul[3] == vec4{26.0f, 28.0f, 30.0f, 32.0f});
	}

	SECTION("sub")
	{
		const mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat4       mul = m - m;
		CHECK(mul[0] == vec4::zero());
		CHECK(mul[1] == vec4::zero());
		CHECK(mul[2] == vec4::zero());
		CHECK(mul[3] == vec4::zero());
	}

	SECTION("transpose")
	{
		const mat4 real{1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16};
		mat4       m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

		auto tranposed = transpose(m);
		CHECK(real == tranposed);
	}

	SECTION("multiply vec4")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		const vec4 v(2.0f, 3.5f, 4.5f, 6.0f);

		CHECK(m * v == vec4{138.0f, 154.0f, 170.0f, 186.0f});
		CHECK(v * m == vec4{46.5f, 110.5f, 174.5f, 238.5f});
	}


	SECTION("divide by scalar")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		const auto div2 = m / 2;
		CHECK(div2[0] == vec4{0.5f, 1.0f, 1.5f, 2.0f});
		CHECK(div2[1] == vec4{2.5f, 3.0f, 3.5f, 4.0f});
		CHECK(div2[2] == vec4{4.5f, 5.0f, 5.5f, 6.0f});
		CHECK(div2[3] == vec4{6.5f, 7.0f, 7.5f, 8.0f});
	}

	SECTION("negate")
	{
		const mat4 a(-4, mat4::fill);
		mat4       m(4, mat4::fill);


		CHECK(a == -m);
	}

	SECTION("equals")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.0f};
		const mat4 m2{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.0001f};

		CHECK(m == m);
		CHECK(m != m2);
	}

	SECTION("lookat_rh")
	{
		mat4 lat = lookat_rh(
		  vec3(4, 10, 3), //
		  vec3(0, 0, 0),  //
		  vec3(0, 1, 0)   //
		);


		CHECK(lat[0] == vec4{0.59999f, -0.71554f, 0.35777f, 0.0f});
		CHECK(lat[1] == vec4{0.0f, 0.44721f, 0.89442f, 0.0f});
		CHECK(lat[2] == vec4{-0.80000f, -0.53665f, 0.26832f, 0.0f});
		CHECK(lat[3] == vec4{0.0f, -0.0f, -11.18033f, 1.0f});
	}

	SECTION("scale")
	{
		mat4 scal{1.0f, 2.0f, 4.0f, 8.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.2f, 2.4f, 3.6f, 4.8f, 8.0f, 8.0f, 8.0f, 8.0f};
		scal = scale(scal, vec3(2.0f, 3.0f, 4.0f));

		CHECK(scal[0] == vec4{2.0f, 4.0f, 8.0f, 16.0f});
		CHECK(scal[1] == vec4{0.6f, 1.2f, 1.8f, 2.4f});
		CHECK(scal[2] == vec4{4.8f, 9.6f, 14.4f, 19.2f});
		CHECK(scal[3] == vec4{8.0f, 8.0f, 8.0f, 8.0f});
	}

	SECTION("translate")
	{
		mat4 transl{1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 10, 10, 10, 10};
		transl = translate(transl, vec3(2.0f, 30.0f, 4.0f));

		CHECK(transl[0] == vec4{1.0f, 1.0f, 1.0f, 1.0f});
		CHECK(transl[1] == vec4{2.0f, 2.0f, 2.0f, 2.0f});
		CHECK(transl[2] == vec4{3.0f, 3.0f, 3.0f, 3.0f});
		CHECK(transl[3] == vec4{84.0f, 84.0f, 84.0f, 84.0f});

		mat4 ViewTranslate = translate(mat4(1.0f), vec3(0.0f, 0.0f, -5.0f));
		CHECK(transl[0] == vec4{1.0f, 1.0f, 1.0f, 1.0f});
		CHECK(transl[1] == vec4{2.0f, 2.0f, 2.0f, 2.0f});
		CHECK(transl[2] == vec4{3.0f, 3.0f, 3.0f, 3.0f});
		CHECK(transl[3] == vec4{84.0f, 84.0f, 84.0f, 84.0f});
	}

	SECTION("perspective")
	{
		auto persp = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

		CHECK(persp[0] == vec4{0.61386f, 0.0f, 0.0f, 0.0f});
		CHECK(persp[1] == vec4{0.0f, 1.09131f, 0.0f, 0.0f});
		CHECK(persp[2] == vec4{0.0f, 0.0f, -1.00200f, -1.0f});
		CHECK(persp[3] == vec4{0.0f, 0.0f, -0.20020f, 0.0f});
	}

	SECTION("ortho")
	{
		auto orthopers = ortho(0, 400, 0, 400, -1, 1);

		CHECK(orthopers[0] == vec4{0.00500f, 0.0f, 0.0f, 0.0f});
		CHECK(orthopers[1] == vec4{0.0f, 0.00500f, 0.0f, 0.0f});
		CHECK(orthopers[2] == vec4{0.0f, 0.0f, -1.0f, 0.0f});
		CHECK(orthopers[3] == vec4{-1.0f, -1.0f, -0.0f, 1.0f});
	}

	SECTION("ortho(w/h)")
	{
		auto orthopers = ortho(400.0f, 400.0f);

		CHECK(orthopers[0] == vec4{0.00500f, 0.0f, 0.0f, 0.0f});
		CHECK(orthopers[1] == vec4{0.0f, 0.00500f, 0.0f, 0.0f});
		CHECK(orthopers[2] == vec4{0.0f, 0.0f, -1.0f, 0.0f});
		CHECK(orthopers[3] == vec4{-1.0f, -1.0f, -0.0f, 1.0f});
	}


	SECTION("frustum")
	{
		auto fr = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

		CHECK(fr[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(fr[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(fr[2] == vec4{0.0f, 0.0f, -1.02020f, -1.0f});
		CHECK(fr[3] == vec4{0.0f, 0.0f, -2.02020f, 0.0f});
	}

	SECTION("Rotate X")
	{

		mat4 rot = rotate(mat4(1.0f), 2.5f, vec3(-1.0f, 0.0f, 0.0f));


		CHECK(rot[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(rot[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		CHECK(rot[2] == vec4{0.0f, 0.59847f, -0.80114f, 0.0f});
		CHECK(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("Rotate Y")
	{
		mat4 rot = rotate(mat4(1.0f), -0.5f, vec3(0.0f, 1.0f, 0.0f));

		CHECK(rot[0] == vec4{0.87758f, 0.0f, 0.47943f, 0.0f});
		CHECK(rot[1] == vec4{0.0f, 1.0f, 0.0, 0.0f});
		CHECK(rot[2] == vec4{-0.47943f, 0.0f, 0.87758f, 0.0f});
		CHECK(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("Rotate Z")
	{
		mat4 rot = rotate(mat4(1.0f), 3.75f, vec3(0.0f, 0.0f, 1.0f));

		CHECK(rot[0] == vec4{-0.82056f, -0.57156f, 0.0f, 0.0f});
		CHECK(rot[1] == vec4{0.57156f, -0.82056f, 0.0, 0.0f});
		CHECK(rot[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		CHECK(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}


	SECTION("inverse")
	{
		mat4 Projection = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		mat4 inv        = inverse(Projection);

		CHECK(inv[0] == vec4{1.62903f, 0.0f, -0.0f, 0.0f});
		CHECK(inv[1] == vec4{0.0f, 0.91633f, 0.0f, -0.0f});
		CHECK(inv[2] == vec4{-0.0f, 0.0f, -0.0f, -4.99500f});
		CHECK(inv[3] == vec4{0.0f, -0.0f, -1.0f, 5.00500f});
	}

	SECTION("determinant")
	{
		mat4 Projection = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		mat4 inv        = inverse(Projection);

		CHECK_THAT(determinant(inv), Catch::Matchers::WithinAbs(-7.45620f, 0.00001f));
	}


	SECTION("full MVP")
	{

		mat4 Projection = perspective(to_radians(45.0f), 4.0f / 3.0f, 0.1f, 100.f);
		CHECK(Projection[0] == vec4{1.810660f, 0.0f, 0.0f, 0.0f});
		CHECK(Projection[1] == vec4{0.0f, 2.41421f, 0.0f, 0.0f});
		CHECK(Projection[2] == vec4{0.0f, 0.0f, -1.00200f, -1.0f});
		CHECK(Projection[3] == vec4{0.0f, 0.0f, -0.20020f, 0.0f});

		mat4 ViewTranslate = translate(mat4(1.0f), vec3(0.0f, 0.0f, -5.0f));
		CHECK(ViewTranslate[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(ViewTranslate[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(ViewTranslate[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		CHECK(ViewTranslate[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});

		mat4 ViewRotateX = rotate(ViewTranslate, 2.5f, vec3(-1.0f, 0.0f, 0.0f));
		CHECK(ViewRotateX[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(ViewRotateX[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		CHECK(ViewRotateX[2] == vec4{0.0f, 0.59847f, -0.80114f, 0.0f});
		CHECK(ViewRotateX[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});

		mat4 View = rotate(ViewRotateX, -2.0f, vec3(0.0f, 1.0f, 0.0f));
		CHECK(View[0] == vec4{-0.41614f, 0.54418f, -0.72847f, 0.0f});
		CHECK(View[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		CHECK(View[2] == vec4{-0.90929f, -0.24905f, 0.33339f, 0.0f});
		CHECK(View[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});


		mat4 Model = scale(mat4(1.0f), vec3(0.5f));
		CHECK(Model[0] == vec4{0.5f, 0.0f, 0.0f, 0.0f});
		CHECK(Model[1] == vec4{0.0f, 0.5f, 0.0f, 0.0f});
		CHECK(Model[2] == vec4{0.0f, 0.0f, 0.5f, 0.0f});
		CHECK(Model[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		mat4 MVP = Projection * View * Model;
		CHECK(MVP[0] == vec4{-0.37675f, 0.65689f, 0.36496f, 0.36423f});
		CHECK(MVP[1] == vec4{0.0f, -0.96706f, 0.29983f, 0.29923f});
		CHECK(MVP[2] == vec4{-0.82321f, -0.30063f, -0.16703f, -0.16670f});
		CHECK(MVP[3] == vec4{0.0f, 0.0f, 4.80981f, 5.0f});


		auto inv = inverse(MVP);
		CHECK(inv[0] == vec4{-0.45966f, 0.0f, -1.00438f, 0.0f});
		CHECK(inv[1] == vec4{0.450821f, -0.66368f, -0.20632f, 0.0f});
		CHECK(inv[2] == vec4{36.38748f, 29.89369f, -16.65300f, -4.99500f});
		CHECK(inv[3] == vec4{-35.00337f, -28.75659f, 16.01955f, 5.00500f});
	}

	SECTION("project/unproject")
	{
		vec3 point(1.0f, -2.0f, 3.0f);

		mat4 model = translate(mat4(1.0f), vec3(0.0f, 0.0f, -10.0f));
		vec4 viewport(0.0f, 0.0f, 640.0f, 360.0f);
		mat4 projection = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);


		vec3 projected   = project(point, model, projection, viewport);
		vec3 unprojected = unproject(projected, model, projection, viewport);

		CHECK(projected == vec3(365.71429f, 128.57142f, 0.86580f));

		CHECK(point == unprojected);
	}


	SECTION("project plane")
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


		CHECK(mouse_world_nearplane == vec3(-4.7432f, -21.0652f, -33.4807f));
		CHECK(mouse_world_farplane == vec3(-1109.41699f, -18156.4727f, -35302.8594f));
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

		CHECK(fmt == test);
	}
}
