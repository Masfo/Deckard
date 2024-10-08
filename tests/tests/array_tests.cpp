#include <catch2/catch_test_macros.hpp>


import std;
import deckard.arrays;

using namespace deckard;

TEST_CASE("array2d", "[array]")
{
	SECTION("init")
	{
		array2d<int> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8 * 8 * 4);

		array2d<int> g2(9, 9);
		REQUIRE(g2.size_in_bytes() == 9 * 9 * 4);
	}

	SECTION("get/set")
	{
		array2d<int> g(8, 8);

		REQUIRE(0 == g.get(1, 1));
		g.set(1, 1, 1);
		REQUIRE(1 == g.get(1, 1));

		g.set(1, 1, 123);
		REQUIRE(123 == g.get(1, 1));
	}

	SECTION("valid")
	{
		array2d<int> g(8, 8);
		REQUIRE(true == g.valid(0, 0));
		REQUIRE(true == g.valid(7, 7));
		REQUIRE(false == g.valid(8, 8));
	}

	SECTION("hash")
	{
		array2d<int> g(8, 8);
		REQUIRE(0x3d0a'02d5'a2e3'8a7d == g.hash());
	}

	SECTION("reverse col")
	{
		array2d<u8> grid(8, 8);
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
		array2d<u8> grid(8, 8);
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
}

//
TEST_CASE("array2d<bool>", "[array]")
{
	SECTION("init")
	{
		array2d<bool> g(8, 8);
		REQUIRE(g.size_in_bytes() == 8);

		array2d<bool> g2(9, 9);
		REQUIRE(g2.size_in_bytes() == 11);
	}

	SECTION("hash")
	{
		array2d<bool> g(8, 8);
		REQUIRE(0x8a23'e09e'6054'a0ea == g.hash());
	}
}
