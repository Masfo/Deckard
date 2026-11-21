#include <catch2/catch_test_macros.hpp>


import std;
import deckard.colors;
using namespace deckard;
using namespace std::literals::string_literals;
;

TEST_CASE("colors", "[colors][rgba][rgb]")
{
	SECTION("rgb/rgba constructor")
	{ 
		rgb<u8> a(196, 25, 9);
		rgba<u8> b(a, 255);

		CHECK(b.r == 196);
		CHECK(b.g == 25);
		CHECK(b.b == 9);
		CHECK(b.a == 255);

		rgba<u8> c(196,25,9,128);
		rgb<u8> d(c);
		CHECK(d.r == 196);
		CHECK(d.g == 25);
		CHECK(d.b == 9);



	}
	SECTION("rgb to hsv")
	{

		// rgb == hsv
		CHECK(rgb<u8>(0, 0, 0) == hsv(0.0f, 0.0f, 0.0f));
		CHECK(rgb<u8>(0, 0, 0) != hsv(360.0f, 1.0f, 1.0f));


		CHECK(rgb<u8>(255, 0, 0) == hsv(0.0f, 100.0f, 100.0f));
		CHECK(rgb<u8>(0, 255, 0) == hsv(120.0f, 100.0f, 100.0f));
		CHECK(rgb<u8>(0, 0, 255) == hsv(240.0f, 100.0f, 100.0f));

		CHECK(rgb<u8>(255, 0, 255) == hsv(300.0f, 100.0f, 100.0f));
		CHECK(rgb<u8>(255, 255, 255) == hsv(0.0f, 0.0f, 100.0f));

		CHECK(rgb<u8>(0, 128, 192) == hsv(200.0f, 100.0f, 75.3f));
		CHECK(hsv{rgb<u8>{0, 128, 192}} == rgb<u8>{0, 128, 192});
		CHECK(hsv{rgba<u8>{0, 128, 192, 64}} == rgba<u8>{0, 128, 192, 64});


		// hsv == rgb
		CHECK(hsv(0.0f, 0.0f, 0.0f) == rgb<u8>(0, 0, 0));
		CHECK(hsv(0.0f, 100.0f, 100.0f) == rgb<u8>(255, 0, 0));
		CHECK(hsv(120.0f, 100.0f, 100.0f) == rgb<u8>(0, 255, 0));
		CHECK(hsv(240.0f, 100.0f, 100.0f) == rgb<u8>(0, 0, 255));
		CHECK(hsv(300.0f, 100.0f, 100.0f) == rgb<u8>(255, 0, 255));
		CHECK(hsv(0.0f, 0.0f, 100.0f) == rgb<u8>(255, 255, 255));
		CHECK(hsv(200.0f, 100.0f, 75.3f) == rgb<u8>(0, 128, 192));
	}
	SECTION("rgb mul")
	{
		rgb<u8> a(200, 150, 100);
		rgb<u8> b(128, 255, 128);

		CHECK(a * b == rgb<u8>(100, 150, 50));
		CHECK(b * a == rgb<u8>(100, 150, 50));
	}

	SECTION("rgb add")
	{
		rgb<u8> a(196, 0, 0);
		rgb<u8> b(128, 128, 0);

		CHECK(a + b == rgb<u8>(255, 128, 0));
		CHECK(b + a == rgb<u8>(255, 128, 0));
	}

	SECTION("rgb sub")
	{
		rgb<u8> a(64, 0, 0);
		rgb<u8> b(64, 128, 0);

		CHECK(a - b == rgb<u8>(0, 0, 0));
		CHECK(b - a == rgb<u8>(0, 128, 0));
	}
	SECTION("generic color")
	{
		static_assert(std::same_as<gray<u8>, generic_color<u8, 1>>);
		static_assert(std::same_as<rgb<u8>, generic_color<u8, 3>>);
		static_assert(std::same_as<rgba<u8>, generic_color<u8, 4>>);
		static_assert(std::same_as<rgb<f32>, generic_color<f32, 3>>);
		static_assert(std::same_as<rgba<f32>, generic_color<f32, 4>>);
		static_assert(std::same_as<rgb_f32, rgb<f32>>);
		static_assert(std::same_as<rgba_f32, rgba<f32>>);

		const gray<u8> gray_color{128};
		CHECK(gray_color.r == 128);
		CHECK(generic_color<u8, 2>{10, 20}[1] == 20);

		const rgb<u8> named_rgb{1, 2, 3};
		CHECK(named_rgb.r == 1);
		CHECK(named_rgb.g == 2);
		CHECK(named_rgb.b == 3);
		rgb<u8> editable_rgb{1, 2, 3};
		editable_rgb.r = 4;
		CHECK(editable_rgb.r == 4);
		const rgba<u8> named_rgba{1, 2, 3, 4};
		CHECK(named_rgba.r == 1);
		CHECK(named_rgba.g == 2);
		CHECK(named_rgba.b == 3);
		CHECK(named_rgba.a == 4);

		const rgb<f32> float_color{0.25f, 0.5f, 0.75f};
		const rgb<f32> float_rhs{0.5f, 0.25f, 0.5f};
		CHECK(float_color + float_rhs == rgb<f32>{0.75f, 0.75f, 1.0f});
		CHECK(float_color - float_rhs == rgb<f32>{0.0f, 0.25f, 0.25f});
		CHECK(float_color * float_rhs == rgb<f32>{0.125f, 0.125f, 0.375f});
		CHECK(float_color / float_rhs == rgb<f32>{0.5f, 1.0f, 1.0f});

		const rgba<f32> float_rgba{float_color, 0.25f};
		CHECK(float_rgba.a == 0.25f);
		const rgba<f32> default_float_rgba{};
		CHECK(default_float_rgba.a == 1.0f);

		const rgba<u8> byte_rgba{rgb<u8>{1, 2, 3}, 0.5f};
		CHECK(byte_rgba == rgba<u8>{1, 2, 3, 128});
	}

	SECTION("rgba mul")
	{
		rgba<u8> a(200, 150, 100, 255);
		rgba<u8> b(128, 255, 128, 255);


		CHECK(a * b == rgba<u8>(100, 150, 50, 255));
		CHECK(b * a == rgba<u8>(100, 150, 50, 255));
	}

	SECTION("div")
	{
		//
	}

	SECTION("add")
	{
		rgba<u8> a(64, 0, 0, 0);
		rgba<u8> b(64, 128, 0, 0);
		CHECK(a + b == rgba<u8>(128, 128, 0, 0));
		CHECK(b + a == rgba<u8>(128, 128, 0, 0));
	}

	SECTION("add overflow")
	{
		rgba<u8> a(196, 0, 0, 0);
		rgba<u8> b(128, 128, 0, 0);
		CHECK(a + b == rgba<u8>(255, 128, 0, 0));
		CHECK(b + a == rgba<u8>(255, 128, 0, 0));
	}


	SECTION("sub")
	{
		rgba<u8> a(64, 0, 0, 0);
		rgba<u8> b(64, 0, 0, 0);
		CHECK(a - b == rgba<u8>(0, 0, 0, 0));
		CHECK(b - a == rgba<u8>(0, 0, 0, 0));
	}

	SECTION("sub underflow")
	{
		rgba<u8> a(64, 0, 0, 0);
		rgba<u8> b(64, 128, 0, 0);
		CHECK(a - b == rgba<u8>(0, 0, 0, 0));
		CHECK(b - a == rgba<u8>(0, 128, 0, 0));
	}

	SECTION("sub underflow")
	{
		rgba<u8> a(196, 0, 0, 0);
		rgba<u8> b(196, 128, 0, 0);
		CHECK(a - b == rgba<u8>(0, 0, 0, 0));
		CHECK(b - a == rgba<u8>(0, 128, 0, 0));
	}

	SECTION("format")
	{

		rgba<u8> a(196, 0, 0);
		CHECK(std::format("{}", a) == "rgba(196, 0, 0, 255)"s);

		CHECK(std::format("{:x}", a) == "rgba(#c40000ff)"s);
		CHECK(std::format("{:X}", a) == "rgba(#C40000FF)"s);

		CHECK(std::format("{:f}", a) == "rgba(0.769, 0.0, 0.0, 1.0)"s);


		rgb<u8> b(196, 25, 9);
		CHECK(std::format("{}", b) == "rgb(196, 25, 9)"s);


		gray<u8> c(128);
		CHECK(std::format("{}", c) == "gray(128)"s);
	}
}
