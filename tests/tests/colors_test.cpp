#include <catch2/catch_test_macros.hpp>


import std;
import deckard.colors;
using namespace deckard;

TEST_CASE("colors", "[colors][rgba][rgb]")
{
	SECTION("rgb to hsv")
	{

		// rgb == hsv
		CHECK(rgb(0, 0, 0) == hsv(0.0f, 0.0f, 0.0f));
		CHECK(rgb(0, 0, 0) != hsv(360.0f, 1.0f, 1.0f));


		CHECK(rgb(255, 0, 0) == hsv(0.0f, 100.0f, 100.0f));
		CHECK(rgb(0, 255, 0) == hsv(120.0f, 100.0f, 100.0f));
		CHECK(rgb(0, 0, 255) == hsv(240.0f, 100.0f, 100.0f));

		CHECK(rgb(255, 0, 255) == hsv(300.0f, 100.0f, 100.0f));
		CHECK(rgb(255, 255, 255) == hsv(0.0f, 0.0f, 100.0f));

		CHECK(rgb(0, 128, 192) == hsv(200.0f, 100.0f, 75.3f));


		// hsv == rgb
		CHECK(hsv(0.0f, 0.0f, 0.0f) == rgb(0, 0, 0));
		CHECK(hsv(0.0f, 100.0f, 100.0f) == rgb(255, 0, 0));
		CHECK(hsv(120.0f, 100.0f, 100.0f) == rgb(0, 255, 0));
		CHECK(hsv(240.0f, 100.0f, 100.0f) == rgb(0, 0, 255));
		CHECK(hsv(300.0f, 100.0f, 100.0f) == rgb(255, 0, 255));
		CHECK(hsv(0.0f, 0.0f, 100.0f) == rgb(255, 255, 255));
		CHECK(hsv(200.0f, 100.0f, 75.3f) == rgb(0, 128, 192));
	}
	SECTION("mul")
	{
		rgba a(200, 150, 100, 255);
		rgba b(128, 255, 128, 255);


		CHECK(a * b == rgba(100, 150, 50, 255));
		CHECK(b * a == rgba(100, 150, 50, 255));
	}

	SECTION("div")
	{
		//
	}

	SECTION("add")
	{
		rgba a(64, 0, 0, 0);
		rgba b(64, 128, 0, 0);
		CHECK(a + b == rgba(128, 128, 0, 0));
		CHECK(b + a == rgba(128, 128, 0, 0));
	}

	SECTION("add overflow")
	{
		rgba a(196, 0, 0, 0);
		rgba b(128, 128, 0, 0);
		CHECK(a + b == rgba(255, 128, 0, 0));
		CHECK(b + a == rgba(255, 128, 0, 0));
	}


	SECTION("sub")
	{
		rgba a(64, 0, 0, 0);
		rgba b(64, 0, 0, 0);
		CHECK(a - b == rgba(0, 0, 0, 0));
		CHECK(b - a == rgba(0, 0, 0, 0));
	}

	SECTION("sub underflow")
	{
		rgba a(64, 0, 0, 0);
		rgba b(64, 128, 0, 0);
		CHECK(a - b == rgba(0, 0, 0, 0));
		CHECK(b - a == rgba(0, 128, 0, 0));
	}

	SECTION("sub underflow")
	{
		rgba a(196, 0, 0, 0);
		rgba b(196, 128, 0, 0);
		CHECK(a - b == rgba(0, 0, 0, 0));
		CHECK(b - a == rgba(0, 128, 0, 0));
	}

	SECTION("format")
	{
		//
	}
}
