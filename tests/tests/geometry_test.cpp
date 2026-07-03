#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>


import std;
import deckard.types;
import deckard.math.utils;
import deckard.geometry;
import deckard.vec;

TEST_CASE("geometry", "[geometry]") 
{
	using namespace deckard;
	using namespace deckard::geometry;
	using namespace deckard::math;

	SECTION("rect") 
	{
		SECTION("c-tor") 
		{
			rect r;

			CHECK(r.position == vec2{0.0f, 0.0f});
			CHECK(r.size == vec2{1.0f, 1.0f});
			CHECK(r.rotation == Catch::Approx(0.0f));

			auto verts = r.vertices();
			CHECK(verts[0] == vec2{-0.5f, -0.5f});
			CHECK(verts[1] == vec2{+0.5f, -0.5f});
			CHECK(verts[2] == vec2{+0.5f, +0.5f});
			CHECK(verts[3] == vec2{-0.5f, +0.5f});

		}

		SECTION("position") 
		{ 
			rect r{vec2{15.0f, -15.0f}};

			CHECK(r.position == vec2{15.0f, -15.0f});
			CHECK(r.size == vec2{1.0f, 1.0f});
		}

		SECTION("size")
		{
			rect r{vec2{15.0f, -15.0f}, vec2{7.5f, 2.5f}};

			CHECK(r.position == vec2{15.0f, -15.0f});
			CHECK(r.size == vec2{7.5f, 2.5f});

		}

		SECTION("rotate")
		{
			rect r;

			r.rotate(to_radians(45.0f));

			CHECK(r.position == vec2{0.0f, 0.0f});
			CHECK(r.size == vec2{1.0f, 1.0f});
			CHECK(r.rotation == Catch::Approx(to_radians(45.0f)));

			constexpr f32 h = std::numbers::sqrt2_v<f32> / 2.0f; // ~0.70711f

			auto verts = r.vertices();
			CHECK(verts[0] == vec2{0.0f, -h});
			CHECK(verts[1] == vec2{h, 0.0f});
			CHECK(verts[2] == vec2{0.0f, h});
			CHECK(verts[3] == vec2{-h, 0.0f});
		}

	}
}

TEST_CASE("geometry intersection", "[geometry][intersection]")
{
	using namespace deckard::geometry;

	SECTION("ray3d intersects...")
	{
		SECTION("sphere")
		{
			//
		}

		SECTION("plane")
		{
			ray3d r{
			  {0, 0, 0},                   // origin
			  {0, 0, 1}                    // direction
			};

			plane p({0, 0, 1}, {0, 0, 5}); // plane z = 5

			auto result = intersect(r, p);

			CHECK(result.hit);
			CHECK(result.distance == Catch::Approx(5.0f));

			CHECK_THAT(result.point.x, Catch::Matchers::WithinAbs(0.0f, 0.0001f));
			CHECK_THAT(result.point.y, Catch::Matchers::WithinAbs(0.0f, 0.0001f));
			CHECK_THAT(result.point.z, Catch::Matchers::WithinAbs(5.0f, 0.0001f));
		}
	}


	SECTION("sphere intersects...")
	{
		SECTION("sphere (intersect)")
		{
			sphere s1({0, 0, 0}, 1.0f);
			sphere s2({0, 0, 1.5f}, 1.0f);

			CHECK(intersect(s1, s2) == true);
		}

		SECTION("sphere (no intersect)")
		{
			sphere s1({0, 0, 0}, 1.0f);
			sphere s2({0, 0, 3.0f}, 1.0f);
			CHECK_FALSE(intersect(s1, s2));
		}

		SECTION("sphere (tanget)")
		{
			sphere s1({0, 0, 0}, 1.0f);
			sphere s2({0, 0, 2.0f}, 1.0f);
			CHECK(intersect(s1, s2));
		}
	}
}
