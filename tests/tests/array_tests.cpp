#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.math;
import deckard.vec;
import deckard.arrays;
import deckard.helpers;
import deckard.as;

using namespace deckard;
using namespace deckard::vec;

TEST_CASE("array", "[array]")
{
	SECTION("init")
	{
		array2d<int> g(8, 8);
		CHECK(g.size_in_bytes() == 8 * 8 * 4);

		array2d<bool> g2(8, 8);
		CHECK(g2.size_in_bytes() == 8 * 8 / 8);
	}

	SECTION("get/set int")
	{
		array2d<int> g(8, 8);
		g.fill(0);

		CHECK(g.get(1, 1) == 0);

		g.set(1, 1, 8);
		CHECK(g.get(1, 1) == 8);
	}

	SECTION("get/set bool")
	{
		array2d<bool> g(8, 8);
		g.fill(0);

		CHECK(g.get(1, 1) == false);

		g.set(1, 1, true);
		CHECK(g.get(1, 1) == true);
	}

	SECTION("extent constructor")
	{
		array2d<int> g(extent<u32>{4, 3});
		CHECK(g.width() == 4u);
		CHECK(g.height() == 3u);
		CHECK(g.size() == 12u);
		CHECK(g.get(3, 2) == 0);

		array2d<bool> b(extent<u32>{4, 3});
		CHECK(b.size_in_bytes() == 2u);
		CHECK_FALSE(b.get(3, 2));
	}

	SECTION("default construct")
	{
		array2d<int> g;
		CHECK(g.width() == 1u);
		CHECK(g.height() == 1u);
		CHECK(g.get(0, 0) == 0);
	}

	SECTION("valid")
	{
		array2d<int> g(4, 3);
		CHECK(g.valid(0, 0));
		CHECK(g.valid(3, 2));
		CHECK_FALSE(g.valid(4, 2));
		CHECK_FALSE(g.valid(3, 3));
		CHECK(g.valid(ivec2{3, 2}));
		CHECK_FALSE(g.valid(ivec2{-1, 0}));
	}

	SECTION("set out of bounds is ignored")
	{
		array2d<int> g(4, 3);
		g.fill(1);
		const auto before = g.hash();

		g.set(4, 0, 9);
		g.set(0, 3, 9);
		g.set(ivec2{-1, 0}, 9);

		CHECK(g.hash() == before);
	}

	SECTION("operator[]")
	{
		array2d<int> g(4, 3);
		g[2, 1]        = 5;
		g[ivec2{3, 2}] = 7;

		CHECK(g[2, 1] == 5);
		CHECK(g[ivec2{2, 1}] == 5);
		CHECK(g[ivec2{3, 2}] == 7);
		CHECK(g.at(3, 2) == 7);
		// linear index, row-major
		CHECK(g[1u * 4u + 2u] == 5);
	}

	SECTION("fill and reset")
	{
		array2d<int> g(3, 3);
		g.fill(4);
		CHECK(g.count("\x04") == 9u);

		g.reset();
		CHECK(g.get(0, 0) == 0);
		CHECK(g.get(2, 2) == 0);
	}

	SECTION("resize")
	{
		array2d<int> g(2, 2);
		g.resize(5, 4);
		CHECK(g.width() == 5u);
		CHECK(g.height() == 4u);
		CHECK(g.size() == 20u);
		CHECK(g.valid(4, 3));

		g.resize(extent<u32>{3, 3});
		CHECK(g.size() == 9u);
	}

	SECTION("clear")
	{
		array2d<int> g(4, 4);
		g.clear();
		CHECK(g.size() == 0u);
	}

	SECTION("equality")
	{
		array2d<int> a(3, 3);
		array2d<int> b(3, 3);
		CHECK(a == b);

		b.set(1, 1, 1);
		CHECK_FALSE(a == b);

		array2d<int> c(3, 2);
		CHECK_FALSE(a == c);
	}

	SECTION("copy and assign")
	{
		array2d<int> a(3, 3);
		a.set(2, 2, 42);

		array2d<int> b = a;
		CHECK(a == b);

		b.set(2, 2, 1);
		CHECK(a.get(2, 2) == 42);
		CHECK(b.get(2, 2) == 1);

		array2d<int> c(1, 1);
		c = a;
		CHECK(c == a);
	}

	SECTION("swap")
	{
		array2d<int> a(3, 2);
		array2d<int> b(2, 4);
		a.set(0, 0, 1);
		b.set(0, 0, 2);

		a.swap(b);

		CHECK(a.width() == 2u);
		CHECK(a.height() == 4u);
		CHECK(a.get(0, 0) == 2);
		CHECK(b.width() == 3u);
		CHECK(b.height() == 2u);
		CHECK(b.get(0, 0) == 1);
	}

	SECTION("hash")
	{
		array2d<int> a(3, 3);
		array2d<int> b(3, 3);
		CHECK(a.hash() == b.hash());
		CHECK(std::hash<array2d<int>>{}(a) == a.hash());

		b.set(0, 0, 1);
		CHECK(a.hash() != b.hash());

		// same cell count, different shape
		array2d<int> c(9, 1);
		CHECK(a.hash() != c.hash());

		CHECK(a.hash(1) != a.hash(2));
	}

	SECTION("bool flip")
	{
		array2d<bool> g(8, 8);
		CHECK_FALSE(g.get(3, 3));

		g.flip(3, 3);
		CHECK(g.get(3, 3));

		g.flip(3, 3);
		CHECK_FALSE(g.get(3, 3));
	}

	SECTION("bool set clears bit")
	{
		array2d<bool> g(8, 8);
		g.fill(true);
		g.set(2, 2, false);

		CHECK_FALSE(g.get(2, 2));
		CHECK(g.get(1, 2));
		CHECK(g.get(3, 2));
	}

	SECTION("bool bits do not bleed across cells")
	{
		array2d<bool> g(5, 5);

		for (u32 y = 0; y < 5; ++y)
			for (u32 x = 0; x < 5; ++x)
				g.set(x, y, (x + y) % 2 == 0);

		for (u32 y = 0; y < 5; ++y)
			for (u32 x = 0; x < 5; ++x)
				CHECK(g.get(x, y) == ((x + y) % 2 == 0));
	}

	SECTION("bool fill equals per-cell set")
	{
		// 9x9 = 81 bits, padding bits in the last byte
		array2d<bool> filled_grid(9, 9);
		filled_grid.fill(true);

		array2d<bool> set_grid(9, 9);
		for (u32 y = 0; y < 9; ++y)
			for (u32 x = 0; x < 9; ++x)
				set_grid.set(x, y, true);

		CHECK(filled_grid == set_grid);
		CHECK(filled_grid.hash() == set_grid.hash());
	}

	SECTION("bool equality and hash")
	{
		array2d<bool> a(8, 8);
		array2d<bool> b(8, 8);
		CHECK(a == b);
		CHECK(a.hash() == b.hash());

		b.set(7, 7, true);
		CHECK_FALSE(a == b);
		CHECK(a.hash() != b.hash());
	}
}

