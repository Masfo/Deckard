#include <catch2/catch_test_macros.hpp>


import std;
import deckard.grid;
import deckard.arrays;
import deckard.helpers;
import deckard.as;

using namespace deckard;

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
}

TEST_CASE("grid", "[grid]")
{
	SECTION("init")
	{
		grid<int> g(8, 8);
		CHECK(g.size_in_bytes() == 8 * 8 * 4);

		grid<int> g2(9, 9);
		CHECK(g2.size_in_bytes() == 9 * 9 * 4);
	}

	SECTION("get/set")
	{
		grid<int> g(8, 8);

		CHECK(0 == g.get(1, 1));
		g.set(1, 1, 1);
		CHECK(1 == g.get(1, 1));

		g.set(1, 1, 123);
		CHECK(123 == g.get(1, 1));
	}

	SECTION("valid")
	{
		grid<int> g(8, 8);
		CHECK(true == g.valid(0, 0));
		CHECK(true == g.valid(7, 7));
		CHECK(false == g.valid(8, 8));
	}

	SECTION("hash")
	{
		grid<int> g(8, 8);
		CHECK(0x3d0a'02d5'a2e3'8a7d == g.hash());
	}

	SECTION("reverse col")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		grid.set(1, 1, '1');
		grid.set(1, 2, '2');

		CHECK('1' == grid.at(1, 1));
		CHECK('2' == grid.at(1, 2));

		grid.reverse_col(1);

		CHECK('1' == grid.at(1, 6));
		CHECK('2' == grid.at(1, 5));
	}

	SECTION("reverse row")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		grid.set(1, 1, '1');
		grid.set(2, 1, '2');


		CHECK('1' == grid.at(1, 1));
		CHECK('2' == grid.at(2, 1));
		CHECK('.' == grid.at(6, 1));

		grid.reverse_row(1);

		CHECK('1' == grid.at(6, 1));
		CHECK('2' == grid.at(5, 1));
		CHECK('.' == grid.at(1, 1));
	}

	SECTION("transpose")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		grid.set(1, 1, '1');
		grid.set(2, 1, '2');

		CHECK('1' == grid.at(1, 1));
		CHECK('2' == grid.at(2, 1));

		grid.transpose();

		CHECK('1' == grid.at(1, 1));
		CHECK('2' == grid.at(1, 2));
	}

	SECTION("read_from_line")
	{
		grid<u8> grid(8, 8);

		for (i32 x : upto(grid.width()))
			grid.vline(x, 0,  grid.height() - 1, as<u8>(x+1+'0'));

		grid.rectangle(1, 1, 3, 3, '#');

		auto s = grid.read_from_line(0, 0, grid.width() - 2, grid.height() - 2);

		auto real = make_vector<u8>('1','#','#','#','5','6','7');
		CHECK(s.size() == 7);
		CHECK(real == s);

	}

	SECTION("line")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		CHECK('.' == grid.get(1, 1));
		CHECK('.' == grid.get(2, 2));
		CHECK('.' == grid.get(3, 3));
		CHECK('.' == grid.get(4, 4));
		CHECK('.' == grid.get(5, 5));

		grid.line(1, 1, 5, 5, '#');

		CHECK('#' == grid.get(1, 1));
		CHECK('#' == grid.get(2, 2));
		CHECK('#' == grid.get(3, 3));
		CHECK('#' == grid.get(4, 4));
		CHECK('#' == grid.get(5, 5));
	}

	SECTION("hline")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		CHECK('.' == grid.get(1, 1));
		CHECK('.' == grid.get(2, 2));
		CHECK('.' == grid.get(3, 3));
		CHECK('.' == grid.get(4, 4));
		CHECK('.' == grid.get(5, 5));

		grid.line(1, 1, 5, 1, '#');

		CHECK('#' == grid.get(1, 1));
		CHECK('#' == grid.get(2, 1));
		CHECK('#' == grid.get(3, 1));
		CHECK('#' == grid.get(4, 1));
		CHECK('#' == grid.get(5, 1));
	}

	SECTION("vline")
	{
		grid<u8> grid(8, 8);
		grid.fill('.');

		CHECK('.' == grid.get(1, 1));
		CHECK('.' == grid.get(2, 2));
		CHECK('.' == grid.get(3, 3));
		CHECK('.' == grid.get(4, 4));
		CHECK('.' == grid.get(5, 5));

		grid.line(1, 1, 1, 5, '#');

		CHECK('#' == grid.get(1, 1));
		CHECK('#' == grid.get(1, 2));
		CHECK('#' == grid.get(1, 3));
		CHECK('#' == grid.get(1, 4));
		CHECK('#' == grid.get(1, 5));
	}

	SECTION("rectangle full")
	{
		grid<u8> grid(5, 5);
		grid.fill('.');

		CHECK('.' == grid.get(1, 1));
		CHECK('.' == grid.get(2, 1));
		CHECK('.' == grid.get(3, 1));

		CHECK('.' == grid.get(1, 2));
		CHECK('.' == grid.get(2, 2));
		CHECK('.' == grid.get(3, 2));

		CHECK('.' == grid.get(1, 3));
		CHECK('.' == grid.get(2, 3));
		CHECK('.' == grid.get(3, 3));

		grid.rectangle(1, 1, 3, 3, '#');

		CHECK('#' == grid.get(1, 1));
		CHECK('#' == grid.get(2, 1));
		CHECK('#' == grid.get(3, 1));

		CHECK('#' == grid.get(1, 2));
		CHECK('#' == grid.get(2, 2));
		CHECK('#' == grid.get(3, 2));

		CHECK('#' == grid.get(1, 3));
		CHECK('#' == grid.get(2, 3));
		CHECK('#' == grid.get(3, 3));
	}

	SECTION("rectangle hollow")
	{
		grid<u8> grid(5, 5);
		grid.fill('.');

		CHECK('.' == grid.get(1, 1));
		CHECK('.' == grid.get(2, 1));
		CHECK('.' == grid.get(3, 1));

		CHECK('.' == grid.get(1, 2));
		CHECK('.' == grid.get(2, 2));
		CHECK('.' == grid.get(3, 2));

		CHECK('.' == grid.get(1, 3));
		CHECK('.' == grid.get(2, 3));
		CHECK('.' == grid.get(3, 3));

		grid.rectangle(1, 1, 3, 3, '#', filled::no);

		CHECK('#' == grid.get(1, 1));
		CHECK('#' == grid.get(2, 1));
		CHECK('#' == grid.get(3, 1));

		CHECK('#' == grid.get(1, 2));
		CHECK('.' == grid.get(2, 2));
		CHECK('#' == grid.get(3, 2));

		CHECK('#' == grid.get(1, 3));
		CHECK('#' == grid.get(2, 3));
		CHECK('#' == grid.get(3, 3));
	}
}

//
TEST_CASE("grid<bool>", "[grid]")
{
	SECTION("init")
	{
		grid<bool> g(8, 8);
		CHECK(g.size_in_bytes() == 8);

		grid<bool> g2(9, 9);
		CHECK(g2.size_in_bytes() == 11);
	}

	SECTION("hash")
	{
		grid<bool> g(8, 8);
		CHECK(0x8a23'e09e'6054'a0ea == g.hash());
	}


	SECTION("transpose")
	{
		grid<bool> grid(8, 8);
		grid.fill(false);

		grid.set(1, 1, true);
		grid.set(2, 1, true);

		CHECK(true == grid.at(1, 1));
		CHECK(true == grid.at(2, 1));


		grid.transpose();

		CHECK(true == grid.at(1, 1));
		CHECK(true == grid.at(1, 2));
	}
}
