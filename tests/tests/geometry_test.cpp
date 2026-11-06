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
	using namespace deckard::vec;


	SECTION("irect") 
	{
		SECTION("c-tor")
		{
			irect r;
			CHECK(r.min == ivec2{0, 0});
			CHECK(r.max == ivec2{1, 1});
		}

		SECTION("empty")
		{
			CHECK(irect::none().empty());
			CHECK_FALSE(irect{}.empty());
			CHECK((irect{ivec2{1, 2}, ivec2{1, 3}}.empty()));
		}

		SECTION("from size") 
		{ 
			irect r = irect::from_size(ivec2{15, -15}, ivec2{7, 3});
			CHECK(r.min == ivec2{15, -15});
			CHECK(r.max == ivec2{22, -12});
			CHECK(r.size() == ivec2{7, 3});
		}

		SECTION("min")
		{
			irect r{ivec2{15, -15}};
			CHECK(r.min == ivec2{15, -15});
			CHECK(r.max == ivec2{16, -14});
		}

		SECTION("min/max")
		{
			irect r{ivec2{15, -15}, ivec2{22, -12}};
			CHECK(r.min == ivec2{15, -15});
			CHECK(r.max == ivec2{22, -12});
		}

		SECTION("contains ivec2")
		{
			irect r{ivec2{15, -15}, ivec2{22, -12}};
			CHECK(r.contains(ivec2{15, -15}));
			CHECK(r.contains(ivec2{21, -13}));
			CHECK_FALSE(r.contains(ivec2{22, -12}));
			CHECK_FALSE(r.contains(ivec2{14, -16}));
		}

		SECTION("contains irect")
		{
			irect r1{ivec2{0, 0}, ivec2{8, 8}};
			irect r2{ivec2{2, 2}, ivec2{6, 6}};
			CHECK(r1.contains(r2));
			CHECK_FALSE(r2.contains(r1));
		}

		SECTION("intersects")
		{
			irect r1{ivec2{15, -15}, ivec2{22, -12}};
			irect r2{ivec2{20, -14}, ivec2{25, -9}};
			CHECK(r1.intersects(r2));
			CHECK(r2.intersects(r1));
			irect r3{ivec2{22, -12}, ivec2{27, -7}};
			CHECK_FALSE(r1.intersects(r3));
			CHECK_FALSE(r3.intersects(r1));
		}

		SECTION("intersection")
		{
			irect r1{ivec2{15, -15}, ivec2{22, -12}};
			irect r2{ivec2{20, -14}, ivec2{25, -9}};
			auto  result = r1.intersection(r2);
			REQUIRE(result.has_value());
			CHECK(result->min == ivec2{20, -14});
			CHECK(result->max == ivec2{22, -12});
			CHECK(result->size() == ivec2{2, 2});
			irect r3{ivec2{22, -12}, ivec2{27, -7}};
			auto  result2 = r1.intersection(r3);
			CHECK_FALSE(result2.has_value());
		}

		SECTION("intersection") 
		{
			irect r1{ivec2{1, 1}, ivec2{9, 8}};
			irect r2{ivec2{7, 6}, ivec2{15, 13}};
			auto  result = r1.intersection(r2);
			REQUIRE(result.has_value());
			CHECK(result->min == ivec2{7, 6});
			CHECK(result->max == ivec2{9, 8});
			CHECK(result->size() == ivec2{2, 2});
		}

		SECTION("union")
		{
			irect r1{ivec2{15, -15}, ivec2{22, -12}};
			irect r2{ivec2{20, -14}, ivec2{25, -9}};
			auto  result = r1.union_with(r2);
			CHECK(result.min == ivec2{15, -15});
			CHECK(result.max == ivec2{25, -9});
		}
	}

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
	using namespace deckard;
	using namespace deckard::geometry;
	using namespace deckard::vec;

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

	SECTION("rect intersects...")
	{
		SECTION("vec2 (inside)")
		{
			rect r({0.0f, 0.0f}, {10.0f, 10.0f});
			CHECK(intersect(r, vec2{0.0f, 0.0f}));
			CHECK(intersect(r, vec2{5.0f, 5.0f}));
			CHECK(intersect(r, vec2{-5.0f, -5.0f}));
			CHECK(r.intersect(vec2{2.0f, 3.0f}));
		}

		SECTION("vec2 (outside)")
		{
			rect r({0.0f, 0.0f}, {10.0f, 10.0f});
			CHECK_FALSE(intersect(r, vec2{5.1f, 0.0f}));
			CHECK_FALSE(intersect(r, vec2{0.0f, -5.1f}));
			CHECK_FALSE(r.intersect(vec2{10.0f, 10.0f}));
		}

		SECTION("rotated")
		{
			rect r({0.0f, 0.0f}, {4.0f, 2.0f});
			r.rotate(std::numbers::pi_v<f32> / 2.0f);

			CHECK(intersect(r, vec2{0.0f, 1.5f}));
			CHECK_FALSE(intersect(r, vec2{1.5f, 0.0f}));
		}
	}
}
