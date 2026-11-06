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
			CHECK_FALSE(irect{ivec2{0, 0}, ivec2{1, 1}}.empty());
		}

		SECTION("from size") 
		{ 
			irect r = irect::from_size(ivec2{15, -15}, ivec2{7, 3});
			CHECK(r.min == ivec2{15, -15});
			CHECK(r.max == ivec2{22, -12});
			CHECK(r.size() == ivec2{7, 3});
		}

		SECTION("area")
		{
			irect r{ivec2{15, -15}, ivec2{22, -12}};
			CHECK(r.area() == 21);
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
			CHECK(contains(r, ivec2{15, -15}));
			CHECK_FALSE(contains(r, ivec2{22, -12}));
		}

		SECTION("contains irect")
		{
			irect r1{ivec2{0, 0}, ivec2{8, 8}};
			irect r2{ivec2{2, 2}, ivec2{6, 6}};
			CHECK(r1.contains(r2));
			CHECK_FALSE(r2.contains(r1));
			CHECK(contains(r1, r2));
			CHECK_FALSE(contains(r2, r1));
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

			CHECK(intersects(r1, r2));
			CHECK_FALSE(intersects(r1, r3));
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

			CHECK(intersection(r1, r2) == result);
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


	SECTION("ibox") 
	{
		//
		SECTION("c-tor")
		{
			ibox b;
			CHECK(b.min == ivec3{0, 0, 0});
			CHECK(b.max == ivec3{1, 1, 1});
		}

		SECTION("from size")
		{
			ibox b = ibox::from_size(ivec3{15, -15, 5}, ivec3{7, 3, 2});
			CHECK(b.min == ivec3{15, -15, 5});
			CHECK(b.max == ivec3{22, -12, 7});
			CHECK(b.size() == ivec3{7, 3, 2});
		}



		SECTION("empty")
		{
			CHECK(ibox{ivec3{0, 0, 0}, ivec3{1, 1, 1}}.empty() == false);
			CHECK(ibox{ivec3{0, 0, 0}, ivec3{0, 1, 1}}.empty());
			CHECK(ibox{ivec3{0, 0, 0}, ivec3{1, 0, 1}}.empty());
			CHECK(ibox{ivec3{0, 0, 0}, ivec3{1, 1, 0}}.empty());
		}

		SECTION("size")
		{
			ibox b{ivec3{0, 0, 0}, ivec3{4, 5, 6}};
			CHECK(b.size() == ivec3{4, 5, 6});
		}

		SECTION("contains ivec3")
		{
			ibox b{ivec3{0, 0, 0}, ivec3{4, 5, 6}};
			CHECK(b.contains(ivec3{0, 0, 0}));
			CHECK(b.contains(ivec3{3, 4, 5}));
			CHECK_FALSE(b.contains(ivec3{4, 5, 6}));
			CHECK_FALSE(b.contains(ivec3{-1, -1, -1}));
		}

		SECTION("contains ibox")
		{ 
			ibox a{ivec3{0, 0, 0}, ivec3{4, 5, 6}};
			ibox b{ivec3{0, 0, 0}, ivec3{1, 1, 1}};
			CHECK(a.contains(b));
			CHECK_FALSE(b.contains(a));
		}

		SECTION("intersects") 
		{
			ibox a{ivec3{0, 0, 0}, ivec3{4, 5, 6}};
			ibox b{ivec3{2, 2, 2}, ivec3{6, 6, 6}};
			CHECK(a.intersects(b));
			CHECK(b.intersects(a));
			ibox c{ivec3{4, 5, 6}, ivec3{8, 9, 10}};
			CHECK_FALSE(a.intersects(c));
			CHECK_FALSE(c.intersects(a));
		}

		SECTION("intersection") 
		{
			ibox a{ivec3{0, 0, 0}, ivec3{4, 5, 6}};
			ibox b{ivec3{2, 2, 2}, ivec3{6, 6, 6}};
			auto result = a.intersection(b);
			REQUIRE(result.has_value());
			CHECK(result->min == ivec3{2, 2, 2});
			CHECK(result->max == ivec3{4, 5, 6});
			CHECK(result->size() == ivec3{2, 3, 4});
			CHECK(result->volume() == 24);
			ibox c{ivec3{4, 5, 6}, ivec3{8, 9, 10}};
			auto result2 = a.intersection(c);
			CHECK_FALSE(result2.has_value());
		}

		SECTION("volume")
		{
			ibox a{ivec3{0, 0, 0}, ivec3{4, 5, 6}};
			CHECK(a.volume() == 120);

			ibox b{{-8, -6, -4}, {-2, -1, 0}};
			CHECK(b.volume() == 120);
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
