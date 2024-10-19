#include <catch2/catch_test_macros.hpp>


import std;
import deckard.grid;
import deckard.arrays;

using namespace deckard;

TEST_CASE("array", "[array]")
{
	SECTION("init")
	{
		array2d<int> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8 * 8 * 4);

		array2d<bool> g2(8, 8);
		REQUIRE(g2.size_in_bytes() == 8 * 8 / 8);
	}

	SECTION("get/set int")
	{
		array2d<int> g(8, 8);
		g.fill(0);

		REQUIRE(g.get(1, 1) == 0);

		g.set(1, 1, 8);
		REQUIRE(g.get(1, 1) == 8);
	}

	SECTION("get/set bool")
	{
		array2d<bool> g(8, 8);
		g.fill(0);

		REQUIRE(g.get(1, 1) == false);

		g.set(1, 1, true);
		REQUIRE(g.get(1, 1) == true);
	}
}

TEST_CASE("grid", "[grid]")
{
	SECTION("init")
	{
		grid<int> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8 * 8 * 4);

		grid<int> g2(9, 9);
		REQUIRE(g2.size_in_bytes() == 9 * 9 * 4);
	}

	SECTION("get/set")
	{
		grid<int> g(8, 8);

		REQUIRE(0 == g.get(1, 1));
		g.set(1, 1, 1);
		REQUIRE(1 == g.get(1, 1));

		g.set(1, 1, 123);
		REQUIRE(123 == g.get(1, 1));
	}

	SECTION("valid")
	{
		grid<int> g(8, 8);
		REQUIRE(true == g.valid(0, 0));
		REQUIRE(true == g.valid(7, 7));
		REQUIRE(false == g.valid(8, 8));
	}

	SECTION("hash")
	{
		grid<int> g(8, 8);
		REQUIRE(0x3d0a'02d5'a2e3'8a7d == g.hash());
	}

	SECTION("reverse col")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		grid.set(1, 1, '1');
		grid.set(1, 2, '2');

		REQUIRE('1' == grid.at(1, 1));
		REQUIRE('2' == grid.at(1, 2));

		grid.reverse_col(1);

		REQUIRE('1' == grid.at(1, 6));
		REQUIRE('2' == grid.at(1, 5));
	}

	SECTION("reverse row")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		grid.set(1, 1, '1');
		grid.set(2, 1, '2');


		REQUIRE('1' == grid.at(1, 1));
		REQUIRE('2' == grid.at(2, 1));
		REQUIRE('.' == grid.at(6, 1));

		grid.reverse_row(1);

		REQUIRE('1' == grid.at(6, 1));
		REQUIRE('2' == grid.at(5, 1));
		REQUIRE('.' == grid.at(1, 1));
	}

	SECTION("transpose")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		grid.set(1, 1, '1');
		grid.set(2, 1, '2');

		REQUIRE('1' == grid.at(1, 1));
		REQUIRE('2' == grid.at(2, 1));

		grid.transpose();

		REQUIRE('1' == grid.at(1, 1));
		REQUIRE('2' == grid.at(1, 2));
	}
}

//
TEST_CASE("grid<bool>", "[grid]")
{
	SECTION("init")
	{
		grid<bool> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8);

		grid<bool> g2(9, 9);
		REQUIRE(g2.size_in_bytes() == 11);
	}

	SECTION("hash")
	{
		grid<bool> g(8, 8);
		REQUIRE(0x8a23'e09e'6054'a0ea == g.hash());
	}


	SECTION("transpose")
	{
		grid<bool> grid(8, 8);
		grid.fill(false);

		grid.set(1, 1, true);
		grid.set(2, 1, true);

		REQUIRE(true == grid.at(1, 1));
		REQUIRE(true == grid.at(2, 1));


		grid.transpose();

		REQUIRE(true == grid.at(1, 1));
		REQUIRE(true == grid.at(1, 2));

		grid.dump();

		grid.rotate_cw();

		grid.dump();
		grid.rotate_ccw();
		grid.dump();

		int k = 0;
	}
}
