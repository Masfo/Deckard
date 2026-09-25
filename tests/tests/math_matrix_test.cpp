#include <catch2/catch_all.hpp>


import std;
import deckard.types;
import deckard.math;
using namespace deckard;
using namespace deckard::math;
using namespace Catch;
using namespace std::string_literals;

TEST_CASE("matrix3 generic", "[matrix3]")
{
	SECTION("default constructor")
	{
		mat3 identity;
		CHECK(identity[0] == vec3{1.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec3{0.0f, 1.0f, 0.0f});
		CHECK(identity[2] == vec3{0.0f, 0.0f, 1.0f});
		CHECK(identity == mat3{});
		CHECK_THAT(identity.determinant(), Matchers::WithinAbs(1.0f, 0.00001f));

		CHECK(identity == mat3::identity());
	}
	SECTION("fill constructor")
	{
		mat3 filled = mat3::filled(0.0f);
		CHECK(filled[0] == vec3{0.0f, 0.0f, 0.0f});
		CHECK(filled[1] == vec3{0.0f, 0.0f, 0.0f});
		CHECK(filled[2] == vec3{0.0f, 0.0f, 0.0f});
		CHECK_THAT(filled.determinant(), Matchers::WithinAbs(0.0f, 0.00001f));
	}

	SECTION("scalar identity")
	{
		mat3 identity(3.0f);
		CHECK(identity[0] == vec3{3.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec3{0.0f, 3.0f, 0.0f});
		CHECK(identity[2] == vec3{0.0f, 0.0f, 3.0f});
		CHECK_THAT(identity.determinant(), Matchers::WithinAbs(27.0f, 0.00001f));
	}

	SECTION("constructors")
	{
		mat3 identity(1.0f);

		CHECK(identity[0] == vec3{1.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec3{0.0f, 1.0f, 0.0f});
		CHECK(identity[2] == vec3{0.0f, 0.0f, 1.0f});

		mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
		CHECK(m[0] == vec3{1.0f, 2.0f, 3.0f});
		CHECK(m[1] == vec3{4.0f, 5.0f, 6.0f});
		CHECK(m[2] == vec3{7.0f, 8.0f, 9.0f});
	}

	SECTION("multiply with identity")
	{
		const mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
		mat3       mul = m * mat3{};

		CHECK(mul[0] == vec3{1.0f, 2.0f, 3.0f});
		CHECK(mul[1] == vec3{4.0f, 5.0f, 6.0f});
		CHECK(mul[2] == vec3{7.0f, 8.0f, 9.0f});
	}

	SECTION("multiply with self")
	{
		const mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
		mat3       mul = m * m;

		CHECK(mul[0] == vec3{30.0f, 36.0f, 42.0f});
		CHECK(mul[1] == vec3{66.0f, 81.0f, 96.0f});
		CHECK(mul[2] == vec3{102.0f, 126.0f, 150.0f});
	}

	SECTION("add")
	{
		const mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
		mat3       mul = m + m;
		CHECK(mul[0] == vec3{2.0f, 4.0f, 6.0f});
		CHECK(mul[1] == vec3{8.0f, 10.0f, 12.0f});
		CHECK(mul[2] == vec3{14.0f, 16.0f, 18.0f});
	}

	SECTION("sub")
	{
		const mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
		mat3       mul = m - m;
		CHECK(mul[0] == vec3::zero());
		CHECK(mul[1] == vec3::zero());
		CHECK(mul[2] == vec3::zero());
	}

	SECTION("transpose")
	{
		const mat3 real{1, 4, 7, 2, 5, 8, 3, 6, 9};
		mat3       m{1, 2, 3, 4, 5, 6, 7, 8, 9};

		auto tranposed = transpose(m);
		CHECK(real == tranposed);
	}

	SECTION("multiply vec3")
	{
		const mat3 m{1, 2, 3, 4, 5, 6, 7, 8, 9};
		const vec3 v(2.0f, 3.5f, 4.5f);

		CHECK(m * v == vec3{47.5f, 57.5f, 67.5f});
		CHECK(v * m == vec3{22.5f, 52.5f, 82.5f});
	}

	SECTION("divide by scalar")
	{
		const mat3 m{1, 2, 3, 4, 5, 6, 7, 8, 9};
		const auto div2 = m / 2;
		CHECK(div2[0] == vec3{0.5f, 1.0f, 1.5f});
		CHECK(div2[1] == vec3{2.0f, 2.5f, 3.0f});
		CHECK(div2[2] == vec3{3.5f, 4.0f, 4.5f});
	}

	SECTION("negate")
	{
		const mat3 a = mat3::filled(-4.0f);
		mat3       m = mat3::filled(4.0f);

		CHECK(a == -m);
	}

	SECTION("equals")
	{
		const mat3 m{1, 2, 3, 4, 5, 6, 7, 8, 9.0f};
		const mat3 m2{1, 2, 3, 4, 5, 6, 7, 8, 9.005f};

		CHECK(m == m);
		CHECK(m != m2);
	}

	SECTION("scale")
	{
		mat3 scal{1.0f, 2.0f, 4.0f, 0.2f, 0.4f, 0.6f, 1.2f, 2.4f, 3.6f};
		scal = scale(scal, vec3(2.0f, 3.0f, 4.0f));

		CHECK(scal[0] == vec3{2.0f, 4.0f, 8.0f});
		CHECK(scal[1] == vec3{0.6f, 1.2f, 1.8f});
		CHECK(scal[2] == vec3{4.8f, 9.6f, 14.4f});
	}

	SECTION("Rotate X")
	{
		mat3 rot = rotate(mat3(1.0f), 2.5f, vec3(-1.0f, 0.0f, 0.0f));

		CHECK(rot[0] == vec3{1.0f, 0.0f, 0.0f});
		CHECK(rot[1] == vec3{0.0f, -0.80114f, -0.59847f});
		CHECK(rot[2] == vec3{0.0f, 0.59847f, -0.80114f});
	}

	SECTION("Rotate Y")
	{
		mat3 rot = rotate(mat3(1.0f), -0.5f, vec3(0.0f, 1.0f, 0.0f));

		CHECK(rot[0] == vec3{0.87758f, 0.0f, 0.47943f});
		CHECK(rot[1] == vec3{0.0f, 1.0f, 0.0f});
		CHECK(rot[2] == vec3{-0.47943f, 0.0f, 0.87758f});
	}

	SECTION("Rotate Z")
	{
		mat3 rot = rotate(mat3(1.0f), 3.75f, vec3(0.0f, 0.0f, 1.0f));

		CHECK(rot[0] == vec3{-0.82056f, -0.57156f, 0.0f});
		CHECK(rot[1] == vec3{0.57156f, -0.82056f, 0.0f});
		CHECK(rot[2] == vec3{0.0f, 0.0f, 1.0f});
	}

	SECTION("inverse")
	{
		// clang-format off
		const mat3 m(
			2.0f, 0.0f, 0.0f,
			0.0f, 3.0f, 0.0f,
			0.0f, 0.0f, 5.0f);
		// clang-format on
		mat3 inv = inverse(m);

		CHECK(inv[0] == vec3{0.5f, 0.0f, 0.0f});
		CHECK(inv[1] == vec3{0.0f, 0.33333f, 0.0f});
		CHECK(inv[2] == vec3{0.0f, 0.0f, 0.2f});
	}

	SECTION("determinant")
	{
		// clang-format off
		const mat3 subfactor = mat3(
			  5.0f, 2.0f, 6.0f,
			  0.0f, 6.0f, 2.0f,
			  3.0f, 8.0f, 1.0f);
		// clang-format on

		CHECK_THAT(determinant(subfactor), Matchers::WithinAbs(-146.0f, 0.00001f));

		CHECK_THAT(determinant(subfactor) * determinant(inverse(subfactor)), Matchers::WithinAbs(1.0f, 0.00001f));

		// clang-format off
		const mat3 m(
			2.0f, 0.0f, 0.0f,
			0.0f, 3.0f, 0.0f,
			0.0f, 0.0f, 5.0f);
		// clang-format on

		CHECK_THAT(m.determinant(), Matchers::WithinAbs(30.0f, 0.00001f));
		CHECK_THAT(m.inverse().determinant(), Matchers::WithinAbs(0.033333f, 0.00001f));

		// clang-format off
		const mat3 m01(
			1.0f, 2.0f, 3.0f,   // row 0
			1.0f, 2.0f, 3.0f,   // row 1 — same -> det 0.0f
			0.0f, 1.0f, 0.0f);
		// clang-format on

		CHECK_THAT(m01.determinant(), Matchers::WithinAbs(0.0f, 0.00001f));
	}
}

TEST_CASE("matrix4 generic", "[matrix4]")
{
	SECTION("default constructor")
	{
		mat4 identity;
		CHECK(identity[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(identity[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		CHECK(identity[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		CHECK(identity == mat4{});
		CHECK_THAT(identity.determinant(), Matchers::WithinAbs(1.0f, 0.00001f));

		CHECK(identity == mat4::identity());
	}

	SECTION("fill constructor")
	{
		mat4 filled = mat4::filled(0.0f);

		CHECK(filled[0] == vec4{0.0f, 0.0f, 0.0f, 0.0f});
		CHECK(filled[1] == vec4{0.0f, 0.0f, 0.0f, 0.0f});
		CHECK(filled[2] == vec4{0.0f, 0.0f, 0.0f, 0.0f});
		CHECK(filled[3] == vec4{0.0f, 0.0f, 0.0f, 0.0f});
		CHECK_THAT(filled.determinant(), Matchers::WithinAbs(0.0f, 0.00001f));
	}

	SECTION("scalar identity")
	{
		mat4 identity(3.0f);
		CHECK(identity[0] == vec4{3.0f, 0.0f, 0.0f, 0.0f});
		CHECK(identity[1] == vec4{0.0f, 3.0f, 0.0f, 0.0f});
		CHECK(identity[2] == vec4{0.0f, 0.0f, 3.0f, 0.0f});
		CHECK(identity[3] == vec4{0.0f, 0.0f, 0.0f, 3.0f});
		CHECK_THAT(identity.determinant(), Matchers::WithinAbs(81.0f, 0.00001f));
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
		mat4       mul = m * mat4{};

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
		const mat4 a = mat4::filled(-4.0f);
		mat4       m = mat4::filled(4.0f);


		CHECK(a == -m);
	}

	SECTION("equals")
	{
		const mat4 m{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.0f};
		const mat4 m2{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16.005f};

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


		CHECK(lat[0] == vec4{0.6f, -0.71554f, 0.357771f, 0.0f});
		CHECK(lat[1] == vec4{0.0f, 0.44721f, 0.89442f, 0.0f});
		CHECK(lat[2] == vec4{-0.8f, -0.536656f, 0.268328f, 0.0f});
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
		CHECK(persp[2] == vec4{0.0f, 0.0f, -1.0010f, -1.0f});
		CHECK(persp[3] == vec4{0.0f, 0.0f, -0.1001f, 0.0f});
	}


	SECTION("ortho Vulkan y-down Z-range")
	{
		f32  width  = 800.0f;
		f32  height = 640.0f;
		mat4 proj   = ortho(width, height);                   // Uses znear=0, zfar=1 default

		CHECK(proj[0] == vec4{0.0025f, 0.0f, 0.0f, 0.0f});
		CHECK(proj[1] == vec4{0.0f, -0.003125f, 0.0f, 0.0f}); // Y scale (negative for Y-down)
		CHECK(proj[2] == vec4{0.0f, 0.0f, -1.0f, 0.0f});      // Z scale (Vulkan: Z in [0, 1])
		CHECK(proj[3] == vec4{-1.0f, 1.0f, 0.0f, 1.0f});      // X/Y translation

		// World origin maps to screen top-left
		vec3          world(0.0f, 0.0f, 0.0f);
		viewport<f32> vp{0.0f, 0.0f, width, height};
		vec3          screen = project(world, mat4::identity(), proj, vp);

		CHECK(screen.x == Approx(0.0f));
		CHECK(screen.y == Approx(0.0f));
		CHECK(screen.z == Approx(0.0f));
	}

	//
	SECTION("ortho(width, height) — origin is top-left, Y grows down")
	{

		f32           width  = 800.0f;
		f32           height = 640.0f;
		const mat4    proj   = ortho(width, height);
		viewport<f32> vp{0.0, 0.0f, width, height};

		{ // top-left
			vec3 world(0.0f, 0.0f);

			vec3 screen = project(world, {}, proj, vp);
			CHECK(screen.x == Approx(0.0f));
			CHECK(screen.y == Approx(0.0f));
		}

		{ // center
			vec3 world(width / 2.0f, height / 2.0f);

			vec3 screen = project(world, {}, proj, vp);
			CHECK(screen.x == Approx(width / 2.0f));
			CHECK(screen.y == Approx(height / 2.0f));
		}

		{ // bottom-right
			vec3 world(width, height);

			vec3 screen = project(world, {}, proj, vp);
			CHECK(screen.x == Approx(width));
			CHECK(screen.y == Approx(height));
		}
	}


	SECTION("ortho(w/h)")
	{
		auto orthopers = ortho(400.0f, 400.0f);

		CHECK(orthopers[0] == vec4{0.005f, 0.0f, 0.0f, 0.0f});
		CHECK(orthopers[1] == vec4{0.0f, -0.005f, 0.0f, 0.0f});
		CHECK(orthopers[2] == vec4{0.0f, 0.0f, -1.0f, 0.0f});
		CHECK(orthopers[3] == vec4{-1.0f, 1.0f, 0.0f, 1.0f});
	}


	SECTION("frustum")
	{
		auto fr = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

		CHECK(fr[0] == vec4{1.0f, 0.0f, 0.0f, 0.0f});
		CHECK(fr[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(fr[2] == vec4{0.0f, 0.0f, -1.01010f, -1.0f});
		CHECK(fr[3] == vec4{0.0f, 0.0f, -1.01010f, 0.0f});
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
		CHECK(rot[1] == vec4{0.0f, 1.0f, 0.0f, 0.0f});
		CHECK(rot[2] == vec4{-0.47943f, 0.0f, 0.87758f, 0.0f});
		CHECK(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("Rotate Z")
	{
		mat4 rot = rotate(mat4(1.0f), 3.75f, vec3(0.0f, 0.0f, 1.0f));

		CHECK(rot[0] == vec4{-0.82056f, -0.57156f, 0.0f, 0.0f});
		CHECK(rot[1] == vec4{0.57156f, -0.82056f, 0.0f, 0.0f});
		CHECK(rot[2] == vec4{0.0f, 0.0f, 1.0f, 0.0f});
		CHECK(rot[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}


	SECTION("inverse")
	{
		mat4 Projection = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		mat4 inv        = inverse(Projection);

		CHECK(inv[0] == vec4{1.62903f, 0.0f, 0.0f, 0.0f});
		CHECK(inv[1] == vec4{0.0f, 0.91633f, 0.0f, -0.0f});
		CHECK(inv[2] == vec4{-0.0f, 0.0f, -0.0f, -9.99f});
		CHECK(inv[3] == vec4{0.0f, -0.0f, -1.0f, 10.0f});
	}

	SECTION("determinant")
	{
		mat4 Projection = perspective(to_radians(85.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
		mat4 inv        = inverse(Projection);

		CHECK_THAT(determinant(inv), Matchers::WithinAbs(-14.91241f, 0.00001f));

		// clang-format off
		const mat4 subfactor02 = mat4(
			  5.0f, 2.0f, 6.0f, 1.0f,
			  0.0f, 6.0f, 2.0f, 0.0f,
			  3.0f, 8.0f, 1.0f, 4.0f,
			  1.0f, 8.0f, 0.0f, 2.0f);

		CHECK_THAT(determinant(subfactor02), Matchers::WithinAbs(118.0f, 0.00001f));

		CHECK_THAT(determinant(subfactor02) * determinant(inverse(subfactor02)), Matchers::WithinAbs(1.0f, 0.00001f));

		const mat4 m(
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 3.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 7.0f);

		CHECK_THAT(m.determinant(), Matchers::WithinAbs(210.0f, 0.00001f));
		CHECK_THAT(m.inverse().determinant(), Matchers::WithinAbs(0.0047619f, 0.00001f));


		const mat4 m01(
			1.0f, 2.0f, 3.0f, 4.0f,   // row 0
			1.0f, 2.0f, 3.0f, 4.0f,   // row 1 — same -> det 0.0f
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f);
		// clang-format on

		CHECK_THAT(m01.determinant(), Matchers::WithinAbs(0.0f, 0.00001f));
	}


	SECTION("full MVP")
	{

		mat4 Projection = perspective(to_radians(45.0f), 4.0f / 3.0f, 0.1f, 100.f);
		CHECK(Projection[0] == vec4{1.810660f, 0.0f, 0.0f, 0.0f});
		CHECK(Projection[1] == vec4{0.0f, 2.41421f, 0.0f, 0.0f});
		CHECK(Projection[2] == vec4{0.0f, 0.0f, -1.001f, -1.0f});
		CHECK(Projection[3] == vec4{0.0f, 0.0f, -0.1001f, 0.0f});

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
		CHECK(View[0] == vec4{-0.416147f, 0.544189f, -0.728478f, 0.0f});
		CHECK(View[1] == vec4{0.0f, -0.80114f, -0.59847f, 0.0f});
		CHECK(View[2] == vec4{-0.909297f, -0.249052f, 0.333393f, 0.0f});
		CHECK(View[3] == vec4{0.0f, 0.0f, -5.0f, 1.0f});


		mat4 Model = scale(mat4(1.0f), vec3(0.5f));
		CHECK(Model[0] == vec4{0.5f, 0.0f, 0.0f, 0.0f});
		CHECK(Model[1] == vec4{0.0f, 0.5f, 0.0f, 0.0f});
		CHECK(Model[2] == vec4{0.0f, 0.0f, 0.5f, 0.0f});
		CHECK(Model[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		mat4 MVP = Projection * View * Model;
		CHECK(MVP[0] == vec4{-0.37675f, 0.656894f, 0.3646f, 0.364239f});
		CHECK(MVP[1] == vec4{0.0f, -0.967066f, 0.299536f, 0.299236f});
		CHECK(MVP[2] == vec4{-0.823214f, -0.300633f, -0.166864f, -0.166697f});
		CHECK(MVP[3] == vec4{0.0f, 0.0f, 4.9049f, 5.0f});


		auto inv = inverse(MVP);
		CHECK(inv[0] == vec4{-0.45966f, 0.0f, -1.00438f, 0.0f});
		CHECK(inv[1] == vec4{0.450821f, -0.663689f, -0.206322f, 0.0f});
		CHECK(inv[2] == vec4{72.77483f, 59.78728f, -33.306f, -9.98998f});
		CHECK(inv[3] == vec4{-71.3907f, -58.6501f, 32.6725f, 9.99998f});
	}

	SECTION("project/unproject")
	{
		vec3 point(1.0f, -2.0f, 3.0f);

		mat4          model = translate(mat4::identity(), vec3(0.0f, 0.0f, -10.0f));
		viewport<f32> vp(0.0f, 0.0f, 640.0f, 360.0f);
		mat4          projection = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);


		vec3 projected = project(point, model, projection, vp);
		CHECK(projected == vec3(365.714f, 231.429f, 0.8658f));

		vec3 unprojected = unproject(projected, model, projection, vp);
		CHECK(unprojected == point);
	}


	SECTION("project plane")
	{
		vec2 mouse{100.0f, 124.0f};
		f32  width      = 1920.0f;
		f32  height     = 1080.0f;
		mat4 Projection = perspective(to_radians(85.0f), width / height, 0.1f, 100.0f);

		mat4 ViewTranslate = translate(mat4::identity(), vec3(0.0f, 0.0f, -5.0f));
		mat4 ViewRotateX   = rotate(ViewTranslate, 2.5f, vec3(-1.0f, 0.0f, 0.0f));
		mat4 View          = rotate(ViewRotateX, -2.0f, vec3(0.0f, 1.0f, 0.0f));

		viewport<f32> vp(0.0f, 0.0f, width, height);
		vec3          mouse_world_nearplane = unproject(vec3(mouse.x, mouse.y, 0.0f), View, Projection, vp);
		vec3          mouse_world_farplane  = unproject(vec3(mouse.x, mouse.y, 1.0f), View, Projection, vp);

		vec3 projected_back_np = project(mouse_world_nearplane, View, Projection, vp);
		CHECK(projected_back_np == vec3(mouse.x, mouse.y, 0.0f));

		vec3 projected_back_fp = project(mouse_world_farplane, View, Projection, vp);
		CHECK(projected_back_fp == vec3(mouse.x, mouse.y, 1.0f));
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


	SECTION("matrix4 to matrix3")
	{
		const mat4 m4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
		mat3       m3 = mat3(m4);
		CHECK(m3[0] == vec3{1.0f, 2.0f, 3.0f});
		CHECK(m3[1] == vec3{5.0f, 6.0f, 7.0f});
		CHECK(m3[2] == vec3{9.0f, 10.0f, 11.0f});


		m3 = m4.to_mat3();
		CHECK(m3[0] == vec3{1.0f, 2.0f, 3.0f});
		CHECK(m3[1] == vec3{5.0f, 6.0f, 7.0f});
		CHECK(m3[2] == vec3{9.0f, 10.0f, 11.0f});
	}

	SECTION("matrix3 to matrix4")
	{
		const mat3 m3(1, 2, 3, 4, 5, 6, 7, 8, 9);
		mat4       m4 = mat4(m3);
		CHECK(m4[0] == vec4{1.0f, 2.0f, 3.0f, 0.0f});
		CHECK(m4[1] == vec4{4.0f, 5.0f, 6.0f, 0.0f});
		CHECK(m4[2] == vec4{7.0f, 8.0f, 9.0f, 0.0f});
		CHECK(m4[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});

		m4 = m3.to_mat4();
		CHECK(m4[0] == vec4{1.0f, 2.0f, 3.0f, 0.0f});
		CHECK(m4[1] == vec4{4.0f, 5.0f, 6.0f, 0.0f});
		CHECK(m4[2] == vec4{7.0f, 8.0f, 9.0f, 0.0f});
		CHECK(m4[3] == vec4{0.0f, 0.0f, 0.0f, 1.0f});
	}

	SECTION("matrix hashing")
	{
		const mat4 a{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		const mat4 b{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
		const mat4 c{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0};

		const std::hash<mat4> hasher{};

		CHECK(hasher(a) == hasher(b));
		CHECK(hasher(a) != hasher(c));
	}
}
