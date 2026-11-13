#include <catch2/catch_all.hpp>


import std;
import deckard.types;
import deckard.math;
import deckard.vec;
import deckard.grid;

TEST_CASE("grid", "[grid]")
{
	using namespace deckard;
	using namespace deckard::vec;
	using namespace std::string_view_literals;

	SECTION("initialize")
	{
		//
		CHECK(1 == 1);
	}

	SECTION("directions")
	{
		CHECK(north.x == 0);
		CHECK(north.y == -1);
		CHECK(east.x == 1);
		CHECK(east.y == 0);
		CHECK(south.x == 0);
		CHECK(south.y == 1);
		CHECK(west.x == -1);
		CHECK(west.y == 0);

		CHECK(north_east.x == 1);
		CHECK(north_east.y == -1);
		CHECK(south_east.x == 1);
		CHECK(south_east.y == 1);
		CHECK(south_west.x == -1);
		CHECK(south_west.y == 1);
		CHECK(north_west.x == -1);
		CHECK(north_west.y == -1);
	}

	SECTION("direction aliases")
	{
		CHECK(deckard::up == north);
		CHECK(deckard::down == south);
		CHECK(deckard::left == west);
		CHECK(deckard::right == east);
		CHECK(vertical == ivec2{0, 1});
		CHECK(horizontal == ivec2{1, 0});
	}

	SECTION("default construct")
	{
		grid<char> g;
		CHECK(g.width() == 1u);
		CHECK(g.height() == 1u);
		CHECK(g.size() == 1u);
		CHECK(g.get(0, 0) == 0);
	}

	SECTION("construct with size")
	{
		grid<char> g(4, 3);
		CHECK(g.width() == 4u);
		CHECK(g.height() == 3u);
		CHECK(g.size() == 12u);
		CHECK(g.size_in_bytes() == 12u);

		grid<u32> g32(4, 3);
		CHECK(g32.size_in_bytes() == 48u);
	}

	SECTION("resize")
	{
		grid<char> g;
		g.resize(5, 2);
		CHECK(g.width() == 5u);
		CHECK(g.height() == 2u);
		CHECK(g.size() == 10u);
	}

	SECTION("clear")
	{
		grid<char> g(4, 3);
		g.clear();
		CHECK(g.size() == 0u);
	}

	SECTION("valid")
	{
		grid<char> g(4, 3);
		CHECK(g.valid(0, 0));
		CHECK(g.valid(3, 2));
		CHECK_FALSE(g.valid(4, 0));
		CHECK_FALSE(g.valid(0, 3));
		CHECK_FALSE(g.valid(100, 100));

		CHECK(g.valid(ivec2{3, 2}));
		CHECK_FALSE(g.valid(ivec2{-1, 0}));
		CHECK_FALSE(g.valid(ivec2{0, -1}));
		CHECK_FALSE(g.valid(ivec2{4, 0}));
	}


	SECTION("hashers")
	{
		grid<char> g1(4, 3);
		grid<char> g2(4, 3);
		CHECK(g1.hash() == g2.hash());

		g1.set(0, 0, 'x');
		CHECK(g1.hash() != g2.hash());

		g2.set(0, 0, 'x');
		CHECK(g1.hash() == g2.hash());


		grid<char>                     g3(4, 3);
		std::unordered_set<grid<char>> set;
		set.insert(g1); //
		set.insert(g2); // same as g1 now
		set.insert(g3);
		CHECK(set.size() == 2u);

		std::map<grid<char>, std::string> map;
		map[g1] = "g1";
		map[g2] = "g2"; // same as g1 now
		map[g3] = "g3";
		CHECK(map.size() == 2u);
	}

	SECTION("set and get")
	{
		grid<char> g(4, 3);
		g.set(1, 2, 'a');
		g.set(ivec2{3, 0}, 'b');

		CHECK(g.get(1, 2) == 'a');
		CHECK(g.get(ivec2{1, 2}) == 'a');
		CHECK(g.get(3, 0) == 'b');
		CHECK(g.at(3, 0) == 'b');
		CHECK(g[3, 0] == 'b');
		CHECK(g[ivec2{3, 0}] == 'b');
		CHECK(g.get(0, 0) == 0);
	}

	SECTION("set out of bounds is ignored")
	{
		grid<char> g(4, 3);
		g.fill('.');
		const auto before = g.hash();

		g.set(100, 100, 'x');
		g.set(4, 0, 'x');
		g.set(0, 3, 'x');

		CHECK(g.hash() == before);
	}

	SECTION("mutable reference access")
	{
		grid<char> g(4, 3);
		g.fill('.');

		g[1, 1]     = 'x';
		g.at(2, 1)  = 'y';
		g.get(3, 1) = 'z';

		CHECK(g.get(1, 1) == 'x');
		CHECK(g.get(2, 1) == 'y');
		CHECK(g.get(3, 1) == 'z');
	}

	SECTION("try_at")
	{
		grid<char> g(4, 3);
		g.set(2, 1, 'q');

		auto hit = g.try_at(ivec2{2, 1});
		CHECK(hit.has_value());
		CHECK(hit.value_or('?') == 'q');

		CHECK_FALSE(g.try_at(ivec2{-1, 0}).has_value());
		CHECK_FALSE(g.try_at(ivec2{0, -1}).has_value());
		CHECK_FALSE(g.try_at(ivec2{4, 0}).has_value());
		CHECK_FALSE(g.try_at(ivec2{0, 3}).has_value());
	}

	SECTION("fill")
	{
		grid<char> g(4, 3);
		g.fill('#');

		CHECK(g.count("#") == 12u);
	}

	SECTION("for_each")
	{
		grid<u8> g(3, 2);
		g.set(0, 0, 1);
		g.set(1, 0, 2);
		g.set(2, 0, 3);
		g.set(0, 1, 4);
		g.set(1, 1, 5);
		g.set(2, 1, 6);

		u32 visited = 0;
		u32 sum     = 0;
		g.for_each(
		  [&](const ivec2 pos, const u8 v)
		  {
			  visited += 1;
			  sum += v;
			  CHECK(g.get(pos) == v);
			  return true;
		  });

		CHECK(visited == 6u);
		CHECK(sum == 21u);
	}

	SECTION("points")
	{
		grid<u8>                 g(3, 2);
		const std::vector<ivec2> expected{{0, 0}, {1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 1}};
		std::vector<ivec2>       points;

		for (const ivec2 point : g.points())
			points.push_back(point);

		CHECK(points == expected);
	}

	SECTION("row")
	{
		grid<u8> g(3, 2);
		g.set(0, 0, 1);
		g.set(1, 0, 2);
		g.set(2, 0, 3);
		g.set(0, 1, 4);
		g.set(1, 1, 5);
		g.set(2, 1, 6);
		// 0: 1 2 3
		// 1: 4 5 6

		const std::array<u8, 3> expected_first_row{1, 2, 3};
		const std::array<u8, 3> expected_second_row{4, 5, 6};
		const auto              first_row  = g.row(0);
		const auto              second_row = g.row(1);
		REQUIRE(first_row.has_value());
		REQUIRE(second_row.has_value());
		CHECK(std::ranges::equal(*first_row, expected_first_row));
		CHECK(std::ranges::equal(*second_row, expected_second_row));
		CHECK_FALSE(g.row(2).has_value());
	}

	SECTION("for_each_row and for_each_column")
	{
		grid<u8> g(3, 2);
		g.set(0, 0, 1);
		g.set(1, 0, 2);
		g.set(2, 0, 3);
		g.set(0, 1, 4);
		g.set(1, 1, 5);
		g.set(2, 1, 6);


		// 0: 1 2 3
		// 1: 4 5 6

		const std::vector<std::vector<u8>> expected_rows{{1, 2, 3}, {4, 5, 6}};
		std::vector<std::vector<u8>>       rows;
		g.for_each_row(
		  [&](const usize y, const std::span<const u8> row)
		  {
			  CHECK(y == rows.size());
			  rows.emplace_back(row.begin(), row.end());
		  });
		CHECK(rows == expected_rows);

		// 1 2 3
		// 4 5 6
		// -----
		// 0 1 2
		const std::vector<std::vector<u8>> expected_columns{{1, 4}, {2, 5}, {3, 6}};
		std::vector<std::vector<u8>>       columns;
		g.for_each_column(
		  [&](const u32 x, const std::vector<u8>& column)
		  {
			  CHECK(x == columns.size());
			  columns.push_back(column);
		  });
		CHECK(columns == expected_columns);
	}

	SECTION("count")
	{
		grid<char> g(3, 2);
		g.fill('.');
		g.set(0, 0, '#');
		g.set(2, 1, '#');
		g.set(1, 1, 'o');

		CHECK(g.count("#") == 2u);
		CHECK(g.count("o") == 1u);
		CHECK(g.count("#o") == 3u);
		CHECK(g.count(".") == 3u);
		CHECK(g.count("z") == 0u);
	}

	SECTION("from_string")
	{
		grid g;
		g.from_string(R"(
########
#......#
#..##..#
#......#
########
	)"sv);
		CHECK(g.width() == 8u);
		CHECK(g.height() == 5u);
		CHECK(g.get(0, 0) == '#');
		CHECK(g.get(1, 1) == '.');
	}

	SECTION("from_lines")
	{
		grid<char> g;
		g.from_lines({"abc", "def"});

		CHECK(g.width() == 3u);
		CHECK(g.height() == 2u);
		CHECK(g.get(0, 0) == 'a');
		CHECK(g.get(2, 0) == 'c');
		CHECK(g.get(0, 1) == 'd');
		CHECK(g.get(2, 1) == 'f');
	}

	SECTION("print")
	{
		grid<char> g;
		g.from_lines({"abc", "def"});

		CHECK(g.print() == "abc\ndef\n");
	}

	SECTION("format")
	{
		grid<char> g;
		g.from_lines({"ab", "cd"});

		CHECK(std::format("{}", g) == "ab\ncd\n");
	}

	SECTION("hash")
	{
		grid<char> a(3, 3);
		grid<char> b(3, 3);
		a.fill('.');
		b.fill('.');

		CHECK(a.hash() == b.hash());

		b.set(1, 1, 'x');
		CHECK(a.hash() != b.hash());

		b.set(1, 1, '.');
		CHECK(a.hash() == b.hash());

		grid<char> c(3, 2);
		c.fill('.');
		CHECK(a.hash() != c.hash());
	}

	SECTION("equality")
	{
		grid<char> a(3, 3);
		grid<char> b(3, 3);
		a.fill('.');
		b.fill('.');
		CHECK(a == b);

		b.set(1, 1, 'x');
		CHECK_FALSE(a == b);

		grid<char> c(3, 2);
		c.fill('.');
		CHECK_FALSE(a == c);
	}

	SECTION("std::hash")
	{
		grid<char> a(3, 3);
		grid<char> b(3, 3);
		a.fill('.');
		b.fill('.');

		CHECK(std::hash<grid<char>>{}(a) == std::hash<grid<char>>{}(b));
		CHECK(std::hash<grid<char>>{}(a) == static_cast<std::size_t>(a.hash()));

		b.set(0, 0, 'x');
		CHECK(std::hash<grid<char>>{}(a) != std::hash<grid<char>>{}(b));
	}

	SECTION("std::hash in unordered_set")
	{
		grid<char> a(2, 2);
		grid<char> b(2, 2);
		a.fill('.');
		b.fill('.');
		b.set(1, 1, 'x');

		std::unordered_set<grid<char>> set;
		set.insert(a);
		set.insert(a);
		set.insert(b);

		CHECK(set.size() == 2u);
		CHECK(set.contains(a));
		CHECK(set.contains(b));
	}

	SECTION("subgrid")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto sub = g.subgrid(1, 1, 2, 2);
		CHECK(sub.width() == 2u);
		CHECK(sub.height() == 2u);
		CHECK(sub.get(0, 0) == 'f');
		CHECK(sub.get(1, 0) == 'g');
		CHECK(sub.get(0, 1) == 'j');
		CHECK(sub.get(1, 1) == 'k');
	}

	SECTION("subgrid ivec2")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto sub = g.subgrid(ivec2{2, 0}, 2, 1);
		CHECK(sub.width() == 2u);
		CHECK(sub.height() == 1u);
		CHECK(sub.get(0, 0) == 'c');
		CHECK(sub.get(1, 0) == 'd');
	}

	SECTION("subgrid clamped to edges")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto sub = g.subgrid(3, 2, 10, 10);
		CHECK(sub.width() == 1u);
		CHECK(sub.height() == 1u);
		CHECK(sub.get(0, 0) == 'l');
	}

	SECTION("subgrid invalid returns default grid")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		const auto empty_hash = grid<char>{}.hash();

		CHECK(g.subgrid(0, 0, 0, 2).hash() == empty_hash);
		CHECK(g.subgrid(0, 0, 2, 0).hash() == empty_hash);
		CHECK(g.subgrid(4, 0, 2, 2).hash() == empty_hash);
		CHECK(g.subgrid(0, 3, 2, 2).hash() == empty_hash);
	}

	SECTION("hline")
	{
		grid<char> g(5, 3);
		g.fill('.');
		g.hline(1, 1, 3, '-');

		CHECK(g.get(0, 1) == '.');
		CHECK(g.get(1, 1) == '-');
		CHECK(g.get(2, 1) == '-');
		CHECK(g.get(3, 1) == '-');
		CHECK(g.get(4, 1) == '.');
		CHECK(g.count("-") == 3u);
	}

	SECTION("hline reversed")
	{
		grid<char> g(5, 3);
		g.fill('.');
		g.hline(3, 1, 1, '-');

		CHECK(g.count("-") == 3u);
		CHECK(g.get(1, 1) == '-');
		CHECK(g.get(3, 1) == '-');
	}

	SECTION("vline")
	{
		grid<char> g(3, 5);
		g.fill('.');
		g.vline(1, 1, 3, '|');

		CHECK(g.get(1, 0) == '.');
		CHECK(g.get(1, 1) == '|');
		CHECK(g.get(1, 2) == '|');
		CHECK(g.get(1, 3) == '|');
		CHECK(g.get(1, 4) == '.');
		CHECK(g.count("|") == 3u);
	}

	SECTION("vline reversed")
	{
		grid<char> g(3, 5);
		g.fill('.');
		g.vline(1, 3, 1, '|');

		CHECK(g.count("|") == 3u);
		CHECK(g.get(1, 1) == '|');
		CHECK(g.get(1, 3) == '|');
	}

	SECTION("line horizontal")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(0, 2, 4, 2, 'x');

		CHECK(g.count("x") == 5u);
		CHECK(g.get(0, 2) == 'x');
		CHECK(g.get(4, 2) == 'x');
	}

	SECTION("line vertical")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(2, 0, 2, 4, 'x');

		CHECK(g.count("x") == 5u);
		CHECK(g.get(2, 0) == 'x');
		CHECK(g.get(2, 4) == 'x');
	}

	SECTION("line diagonal")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(0, 0, 4, 4, 'x');

		CHECK(g.count("x") == 5u);
		for (u32 i = 0; i < 5; ++i)
			CHECK(g.get(i, i) == 'x');
	}

	SECTION("line diagonal reversed")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(4, 0, 0, 4, 'x');

		CHECK(g.count("x") == 5u);
		for (u32 i = 0; i < 5; ++i)
			CHECK(g.get(4 - i, i) == 'x');
	}

	SECTION("line with callback")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(0, 0, 4, 0, []() -> char { return 'c'; });

		CHECK(g.count("c") == 5u);
	}

	SECTION("line single point")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(3, 1, 3, 1, 'x');

		CHECK(g.count("x") == 1u);
		CHECK(g.get(3, 1) == 'x');
	}

	SECTION("rectangle filled")
	{
		grid<char> g(6, 5);
		g.fill('.');
		g.rectangle(1, 1, 3, 2, '#');

		CHECK(g.count("#") == 6u);
		CHECK(g.get(1, 1) == '#');
		CHECK(g.get(3, 2) == '#');
		CHECK(g.get(0, 1) == '.');
		CHECK(g.get(4, 1) == '.');
		CHECK(g.get(1, 3) == '.');
	}

	SECTION("rectangle filled swapped corners")
	{
		grid<char> g(6, 5);
		g.fill('.');
		g.rectangle(3, 2, 1, 1, '#');

		CHECK(g.count("#") == 6u);
		CHECK(g.get(1, 1) == '#');
		CHECK(g.get(3, 2) == '#');
	}

	SECTION("rectangle outline")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.rectangle(0, 0, 4, 4, '#', filled::no);

		CHECK(g.count("#") == 16u);
		CHECK(g.get(0, 0) == '#');
		CHECK(g.get(4, 0) == '#');
		CHECK(g.get(0, 4) == '#');
		CHECK(g.get(4, 4) == '#');
		CHECK(g.get(2, 0) == '#');
		CHECK(g.get(2, 4) == '#');
		CHECK(g.get(0, 2) == '#');
		CHECK(g.get(4, 2) == '#');
		CHECK(g.get(2, 2) == '.');
		CHECK(g.get(1, 1) == '.');
		CHECK(g.get(3, 3) == '.');
	}

	SECTION("from_line horizontal")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto v = g.read_from_line(0, 1, 3, 1);
		CHECK(v.size() == 4u);
		CHECK(v == std::vector<char>{'e', 'f', 'g', 'h'});
	}

	SECTION("from_line vertical")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto v = g.read_from_line(2, 0, 2, 2);
		CHECK(v == std::vector<char>{'c', 'g', 'k'});
	}

	SECTION("read_from_line diagonal")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto v = g.read_from_line(0, 0, 2, 2);
		CHECK(v == std::vector<char>{'a', 'f', 'k'});
	}

	SECTION("read_from_line reversed")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto v = g.read_from_line(3, 0, 0, 0);
		CHECK(v == std::vector<char>{'d', 'c', 'b', 'a'});
	}

	SECTION("read_from_line ivec2")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto v = g.read_from_line(ivec2{0, 0}, ivec2{2, 2});
		CHECK(v == std::vector<char>{'a', 'f', 'k'});
	}

	SECTION("read_from_line single point")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});

		auto v = g.read_from_line(1, 2, 1, 2);
		CHECK(v == std::vector<char>{'j'});
	}

	SECTION("find_all")
	{
		grid<char> g;
		g.from_lines({"a.a", ".a.", "..a"});

		auto points = g.find_all('a');
		CHECK(points.size() == 4u);

		// sorted by row, then column
		CHECK(points[0] == ivec2{0, 0});
		CHECK(points[1] == ivec2{2, 0});
		CHECK(points[2] == ivec2{1, 1});
		CHECK(points[3] == ivec2{2, 2});
	}

	SECTION("find_all none")
	{
		grid<char> g;
		g.from_lines({"...", "..."});

		CHECK(g.find_all('x').empty());
	}

	SECTION("floodfill")
	{
		grid<char> g;
		g.from_lines({"..#.", "..#.", "..#.", "..#."});

		const auto filled_count = g.floodfill({0, 0}, 'o');

		CHECK(filled_count == 8u);
		CHECK(g.count("o") == 8u);
		CHECK(g.count("#") == 4u);
		CHECK(g.count(".") == 4u);
		CHECK(g.get(0, 0) == 'o');
		CHECK(g.get(1, 3) == 'o');
		CHECK(g.get(2, 0) == '#');
		CHECK(g.get(3, 0) == '.');
	}

	SECTION("floodfill whole grid")
	{
		grid<char> g(4, 3);
		g.fill('.');

		CHECK(g.floodfill({1, 1}, 'o') == 12u);
		CHECK(g.count("o") == 12u);
	}

	SECTION("floodfill with callback")
	{
		grid<char> g(3, 3);
		g.fill('.');

		u32  calls = 0;
		auto n     = g.floodfill(
		  {0, 0},
		  [&](ivec2) -> char
		  {
			  calls += 1;
			  return 'c';
		  });

		CHECK(n == 9u);
		CHECK(calls == 9u);
		CHECK(g.count("c") == 9u);
	}

	SECTION("floodfill same value does not loop")
	{
		grid<char> g(3, 3);
		g.fill('.');

		CHECK(g.floodfill({0, 0}, '.') == 9u);
	}

	SECTION("transpose")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.transpose();

		// abc
		// def
		// ghi


		CHECK(g.print() == "adg\nbeh\ncfi\n");
	}

	SECTION("reverse_row")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.reverse_row(1);

		CHECK(g.print() == "abc\nfed\nghi\n");
	}

	SECTION("reverse_col")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.reverse_col(0);

		CHECK(g.print() == "gbc\ndef\nahi\n");
	}

	SECTION("reverse_rows")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.reverse_rows();

		CHECK(g.print() == "cba\nfed\nihg\n");
	}

	SECTION("reverse_columns")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.reverse_columns();

		CHECK(g.print() == "ghi\ndef\nabc\n");
	}

	SECTION("rotate_cw")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.rotate_cw();

		CHECK(g.print() == "gda\nheb\nifc\n");
	}

	SECTION("rotate_ccw")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.rotate_ccw();

		CHECK(g.print() == "cfi\nbeh\nadg\n");
	}

	SECTION("rotate four times is identity")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		const auto original = g.hash();

		g.rotate_cw();
		g.rotate_cw();
		g.rotate_cw();
		g.rotate_cw();
		CHECK(g.hash() == original);

		g.rotate_ccw();
		g.rotate_ccw();
		g.rotate_ccw();
		g.rotate_ccw();
		CHECK(g.hash() == original);
	}

	SECTION("rotate cw then ccw is identity")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		const auto original = g.hash();

		g.rotate_cw();
		g.rotate_ccw();
		CHECK(g.hash() == original);
	}

	SECTION("transpose non-square")
	{
		grid<char> g;
		g.from_lines({"abc", "def"});
		g.transpose();

		CHECK(g.width() == 2u);
		CHECK(g.height() == 3u);
		CHECK(g.print() == "ad\nbe\ncf\n");
	}

	SECTION("rotate_cw non-square")
	{
		grid<char> g;
		g.from_lines({"abc", "def"});
		g.rotate_cw();

		CHECK(g.print() == "da\neb\nfc\n");
	}

	SECTION("rotate_ccw non-square")
	{
		grid<char> g;
		g.from_lines({"abc", "def"});
		g.rotate_ccw();

		CHECK(g.print() == "cf\nbe\nad\n");
	}

	SECTION("reverse_row non-square")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh"});
		g.reverse_row(1);

		CHECK(g.print() == "abcd\nhgfe\n");
	}

	SECTION("reverse_col non-square")
	{
		grid<char> g;
		g.from_lines({"abcd", "efgh", "ijkl"});
		g.reverse_col(3);

		CHECK(g.print() == "abcl\nefgh\nijkd\n");
	}

	SECTION("circle")
	{
		grid<char> g(7, 7);
		g.fill('.');
		g.circle(3, 3, 2, '#');


		CHECK(g.get(5, 3) == '#');
		CHECK(g.get(1, 3) == '#');
		CHECK(g.get(3, 5) == '#');
		CHECK(g.get(3, 1) == '#');
		CHECK(g.get(3, 3) == '.');
		CHECK(g.get(0, 0) == '.');
		CHECK(g.get(6, 6) == '.');
	}

	SECTION("circle clipped at edge")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.circle(0, 0, 2, '#');

		CHECK(g.get(2, 0) == '#');
		CHECK(g.get(0, 2) == '#');
		CHECK(g.get(0, 0) == '.');
	}

	SECTION("neighbours4")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});

		// abc
		// def
		// ghi

		auto center = g.neighbours4(ivec2{1, 1});
		CHECK(center[0] == 'b');
		CHECK(center[1] == 'f');
		CHECK(center[2] == 'h');
		CHECK(center[3] == 'd');

		auto corner = g.neighbours4(ivec2{0, 0});
		CHECK_FALSE(corner[0].has_value());
		CHECK(corner[1] == 'b');
		CHECK(corner[2] == 'd');
		CHECK_FALSE(corner[3].has_value());
	}

	SECTION("neighbours8")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});

		// abc
		// def
		// ghi

		const auto                center = g.neighbours8(ivec2{1, 1});
		const std::array<char, 8> expected_center{'b', 'c', 'f', 'i', 'h', 'g', 'd', 'a'};
		for (std::size_t i = 0; i < center.size(); ++i)
			CHECK(center[i] == expected_center[i]);

		const auto corner = g.neighbours8(ivec2{0, 0});
		CHECK_FALSE(corner[0].has_value());
		CHECK_FALSE(corner[1].has_value());
		CHECK(corner[2] == 'b');
		CHECK(corner[3] == 'e');
		CHECK(corner[4] == 'd');
		CHECK_FALSE(corner[5].has_value());
		CHECK_FALSE(corner[6].has_value());
		CHECK_FALSE(corner[7].has_value());
	}


	SECTION("for_each_neighbour4")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});

		// abc
		// def
		// ghi
		g.for_each_neighbour4(
		  [](const ivec2 pos, const char v, auto neighbours)
		  {
			  if (pos == ivec2{0, 0})
			  {
				  CHECK(v == char{'a'});
				  CHECK_FALSE(neighbours[0].has_value());
				  CHECK(neighbours[1].value() == char{'b'});
				  CHECK(neighbours[2].value() == char{'d'});
				  CHECK_FALSE(neighbours[3].has_value());
			  }


			  if (pos == ivec2{1, 1})
			  {
				  CHECK(v == char{'e'});
				  CHECK(neighbours[0].value() == char{'b'});
				  CHECK(neighbours[1].value() == char{'f'});
				  CHECK(neighbours[2].value() == char{'h'});
				  CHECK(neighbours[3].value() == char{'d'});
			  }

			  if (pos == ivec2{2, 1})
				  return false;

			  return true;
		  });
	}

	SECTION("for_each_neighbour8")
	{
		grid<char> g;
		g.from_lines({"abc", "def", "ghi"});
		g.for_each_neighbour8(
		  [&](const ivec2 pos, const char v, auto neighbours)
		  {
			  if (pos == ivec2{0, 0})
			  {
				  CHECK(v == char{'a'});
				  CHECK_FALSE(neighbours[0].has_value());
				  CHECK_FALSE(neighbours[1].has_value());
				  CHECK(neighbours[2].value() == char{'b'});
				  CHECK(neighbours[3].value() == char{'e'});
				  CHECK(neighbours[4].value() == char{'d'});
				  CHECK_FALSE(neighbours[5].has_value());
				  CHECK_FALSE(neighbours[6].has_value());
				  CHECK_FALSE(neighbours[7].has_value());
			  }
			  if (pos == ivec2{1, 1})
			  {
				  CHECK(v == char{'e'});
				  CHECK(neighbours[0].value() == char{'b'});
				  CHECK(neighbours[1].value() == char{'c'});
				  CHECK(neighbours[2].value() == char{'f'});
				  CHECK(neighbours[3].value() == char{'i'});
				  CHECK(neighbours[4].value() == char{'h'});
				  CHECK(neighbours[5].value() == char{'g'});
				  CHECK(neighbours[6].value() == char{'d'});
				  CHECK(neighbours[7].value() == char{'a'});
			  }
			  if (pos == ivec2{2, 1})
				  return false;

			  return true;
		  });
	}


	SECTION("ivec2 element access")
	{
		grid<char> g(4, 3);
		g.fill('.');

		g.at(ivec2{1, 1})  = 'a';
		g[ivec2{2, 1}]     = 'b';
		g.get(ivec2{3, 1}) = 'c';

		CHECK(g.get(1, 1) == 'a');
		CHECK(g.get(2, 1) == 'b');
		CHECK(g.get(3, 1) == 'c');
		CHECK(std::as_const(g).at(ivec2{1, 1}) == 'a');
	}

	SECTION("ivec2 line")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(ivec2{0, 0}, ivec2{4, 4}, 'x');

		CHECK(g.count("x") == 5u);
		CHECK(g.get(2, 2) == 'x');
	}

	SECTION("ivec2 line with callback")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(ivec2{0, 1}, ivec2{4, 1}, []() -> char { return 'c'; });

		CHECK(g.count("c") == 5u);
		CHECK(g.get(3, 1) == 'c');
	}

	SECTION("ivec2 hline")
	{
		grid<char> g(5, 3);
		g.fill('.');
		g.hline(ivec2{1, 1}, 3, '-');

		CHECK(g.count("-") == 3u);
		CHECK(g.get(1, 1) == '-');
		CHECK(g.get(3, 1) == '-');
	}

	SECTION("ivec2 vline")
	{
		grid<char> g(3, 5);
		g.fill('.');
		g.vline(ivec2{1, 1}, 3, '|');

		CHECK(g.count("|") == 3u);
		CHECK(g.get(1, 1) == '|');
		CHECK(g.get(1, 3) == '|');
	}

	SECTION("ivec2 rectangle")
	{
		grid<char> g(6, 5);
		g.fill('.');
		g.rectangle(ivec2{1, 1}, ivec2{3, 2}, '#');
		CHECK(g.count("#") == 6u);

		g.fill('.');
		g.rectangle(ivec2{0, 0}, ivec2{4, 4}, '#', filled::no);
		CHECK(g.count("#") == 16u);
		CHECK(g.get(2, 2) == '.');
	}

	SECTION("ivec2 circle")
	{
		grid<char> g(7, 7);
		g.fill('.');
		g.circle(ivec2{3, 3}, 2, '#');

		CHECK(g.get(5, 3) == '#');
		CHECK(g.get(1, 3) == '#');
		CHECK(g.get(3, 5) == '#');
		CHECK(g.get(3, 1) == '#');
		CHECK(g.get(3, 3) == '.');
	}

	SECTION("ivec2 floodfill")
	{
		grid<char> g;
		g.from_lines({"..#.", "..#.", "..#.", "..#."});

		CHECK(g.floodfill(ivec2{0, 0}, 'o') == 8u);
		CHECK(g.count("o") == 8u);
	}

	SECTION("ivec2 floodfill with callback")
	{
		grid<char> g(3, 3);
		g.fill('.');

		u32  calls = 0;
		auto n     = g.floodfill(
		  ivec2{1, 1},
		  [&](ivec2) -> char
		  {
			  calls += 1;
			  return 'c';
		  });

		CHECK(n == 9u);
		CHECK(calls == 9u);
	}

	SECTION("ivec2 const element access")
	{
		grid<char> g(4, 3);
		g.fill('.');
		g.set(ivec2{2, 2}, 'z');

		const grid<char>& cg = g;
		CHECK(cg.at(ivec2{2, 2}) == 'z');
		CHECK(cg.get(ivec2{2, 2}) == 'z');
		CHECK(cg[ivec2{2, 2}] == 'z');
		CHECK(cg.at(ivec2{0, 0}) == '.');
	}

	SECTION("ivec2 element access writes through")
	{
		grid<char> g(3, 3);
		g.fill('.');

		g.at(ivec2{1, 1}) = 'x';
		CHECK(g.get(1, 1) == 'x');
		CHECK(g.get(ivec2{1, 1}) == 'x');

		g[ivec2{0, 2}] = 'y';
		CHECK(g.get(0, 2) == 'y');
	}

	SECTION("ivec2 line matches coordinate overload")
	{
		grid<char> a(6, 6);
		grid<char> b(6, 6);
		a.fill('.');
		b.fill('.');

		a.line(1, 0, 5, 3, 'x');
		b.line(ivec2{1, 0}, ivec2{5, 3}, 'x');

		CHECK(a.hash() == b.hash());
	}

	SECTION("ivec2 line horizontal")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(ivec2{0, 2}, ivec2{4, 2}, 'x');

		CHECK(g.count("x") == 5u);
		CHECK(g.get(0, 2) == 'x');
		CHECK(g.get(4, 2) == 'x');
	}

	SECTION("ivec2 line vertical")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(ivec2{2, 0}, ivec2{2, 4}, 'x');

		CHECK(g.count("x") == 5u);
		CHECK(g.get(2, 0) == 'x');
		CHECK(g.get(2, 4) == 'x');
	}

	SECTION("ivec2 line reversed")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(ivec2{4, 4}, ivec2{0, 0}, 'x');

		CHECK(g.count("x") == 5u);
		for (u32 i = 0; i < 5; ++i)
			CHECK(g.get(i, i) == 'x');
	}

	SECTION("ivec2 line single point")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.line(ivec2{3, 1}, ivec2{3, 1}, 'x');

		CHECK(g.count("x") == 1u);
		CHECK(g.get(3, 1) == 'x');
	}

	SECTION("ivec2 line callback called per cell")
	{
		grid<char> g(5, 5);
		g.fill('.');

		u32 calls = 0;
		g.line(ivec2{0, 0},
			   ivec2{4, 4},
			   [&]() -> char
			   {
				   calls += 1;
				   return 'c';
			   });

		CHECK(calls == 5u);
		CHECK(g.count("c") == 5u);
	}

	SECTION("ivec2 hline reversed end")
	{
		grid<char> g(5, 3);
		g.fill('.');
		g.hline(ivec2{3, 1}, 1, '-');

		CHECK(g.count("-") == 3u);
		CHECK(g.get(1, 1) == '-');
		CHECK(g.get(2, 1) == '-');
		CHECK(g.get(3, 1) == '-');
	}

	SECTION("ivec2 hline single cell")
	{
		grid<char> g(5, 3);
		g.fill('.');
		g.hline(ivec2{2, 0}, 2, '-');

		CHECK(g.count("-") == 1u);
		CHECK(g.get(2, 0) == '-');
	}

	SECTION("ivec2 hline clipped at edge")
	{
		grid<char> g(5, 3);
		g.fill('.');
		g.hline(ivec2{3, 2}, 8, '-');

		CHECK(g.count("-") == 2u);
		CHECK(g.get(3, 2) == '-');
		CHECK(g.get(4, 2) == '-');
	}

	SECTION("ivec2 vline reversed end")
	{
		grid<char> g(3, 5);
		g.fill('.');
		g.vline(ivec2{1, 3}, 1, '|');

		CHECK(g.count("|") == 3u);
		CHECK(g.get(1, 1) == '|');
		CHECK(g.get(1, 2) == '|');
		CHECK(g.get(1, 3) == '|');
	}

	SECTION("ivec2 vline single cell")
	{
		grid<char> g(3, 5);
		g.fill('.');
		g.vline(ivec2{0, 4}, 4, '|');

		CHECK(g.count("|") == 1u);
		CHECK(g.get(0, 4) == '|');
	}

	SECTION("ivec2 vline clipped at edge")
	{
		grid<char> g(3, 5);
		g.fill('.');
		g.vline(ivec2{2, 3}, 9, '|');

		CHECK(g.count("|") == 2u);
		CHECK(g.get(2, 3) == '|');
		CHECK(g.get(2, 4) == '|');
	}

	SECTION("ivec2 rectangle swapped corners")
	{
		grid<char> g(6, 5);
		g.fill('.');
		g.rectangle(ivec2{3, 2}, ivec2{1, 1}, '#');

		CHECK(g.count("#") == 6u);
		CHECK(g.get(1, 1) == '#');
		CHECK(g.get(3, 2) == '#');
	}

	SECTION("ivec2 rectangle outline matches coordinate overload")
	{
		grid<char> a(6, 6);
		grid<char> b(6, 6);
		a.fill('.');
		b.fill('.');

		a.rectangle(1, 1, 4, 4, '#', filled::no);
		b.rectangle(ivec2{1, 1}, ivec2{4, 4}, '#', filled::no);

		CHECK(a.hash() == b.hash());
		CHECK(b.count("#") == 12u);
		CHECK(b.get(2, 2) == '.');
	}

	SECTION("ivec2 rectangle single cell")
	{
		grid<char> g(4, 4);
		g.fill('.');
		g.rectangle(ivec2{2, 2}, ivec2{2, 2}, '#');

		CHECK(g.count("#") == 1u);
		CHECK(g.get(2, 2) == '#');
	}

	SECTION("ivec2 circle matches coordinate overload")
	{
		grid<char> a(9, 9);
		grid<char> b(9, 9);
		a.fill('.');
		b.fill('.');

		a.circle(4, 4, 3, '#');
		b.circle(ivec2{4, 4}, 3, '#');

		CHECK(a.hash() == b.hash());
	}

	SECTION("ivec2 circle clipped at edge")
	{
		grid<char> g(5, 5);
		g.fill('.');
		g.circle(ivec2{0, 0}, 2, '#');

		CHECK(g.get(2, 0) == '#');
		CHECK(g.get(0, 2) == '#');
		CHECK(g.get(0, 0) == '.');
	}

	SECTION("ivec2 floodfill stops at wall")
	{
		grid<char> g;
		g.from_lines({"..#.", "..#.", "..#.", "..#."});

		CHECK(g.floodfill(ivec2{3, 0}, 'o') == 4u);
		CHECK(g.count("o") == 4u);
		CHECK(g.get(0, 0) == '.');
		CHECK(g.get(3, 3) == 'o');
	}

	SECTION("ivec2 floodfill whole grid")
	{
		grid<char> g(4, 3);
		g.fill('.');

		CHECK(g.floodfill(ivec2{2, 1}, 'o') == 12u);
		CHECK(g.count("o") == 12u);
	}

	SECTION("ivec2 floodfill single cell")
	{
		grid<char> g(3, 3);
		g.fill('.');
		g.set(1, 1, '#');

		CHECK(g.floodfill(ivec2{1, 1}, 'o') == 1u);
		CHECK(g.get(1, 1) == 'o');
		CHECK(g.count("o") == 1u);
	}

	SECTION("ivec2 floodfill callback receives position")
	{
		grid<char> g(2, 2);
		g.fill('.');

		std::vector<ivec2> seen;
		g.floodfill(ivec2{0, 0},
					[&](ivec2 pos) -> char
					{
						seen.push_back(pos);
						return 'c';
					});

		CHECK(seen.size() == 4u);
		CHECK(std::ranges::contains(seen, ivec2{0, 0}));
		CHECK(std::ranges::contains(seen, ivec2{1, 0}));
		CHECK(std::ranges::contains(seen, ivec2{0, 1}));
		CHECK(std::ranges::contains(seen, ivec2{1, 1}));
	}

	SECTION("islands")
	{
		grid<u8> g;
		g.from_lines(
		  {"@.........",
		   "..##......",
		   ".###......",
		   "..........",
		   "......@...",
		   "......@...",
		   "......@...",
		   "......@...",
		   "..@@@@@...",
		   ".........."});
		//  0123456789
		auto islands = g.find_islands("#@");
		CHECK(islands.size() == 3u);

		CHECK(islands[0].size() == 1u);
		CHECK(std::ranges::contains(islands[0], ivec2{0, 0}));


		CHECK(islands[1].size() == 5u);
		CHECK(std::ranges::contains(islands[1], ivec2{2, 1}));
		CHECK(std::ranges::contains(islands[1], ivec2{3, 2}));

		CHECK(islands[2].size() == 9u);
		CHECK(std::ranges::contains(islands[2], ivec2{6, 4}));
		CHECK(std::ranges::contains(islands[2], ivec2{2, 8}));
		CHECK_FALSE(std::ranges::contains(islands[2], ivec2{1, 8}));


	}

	SECTION("visual a_star")
	{
		grid<u8> g;
		g.from_lines(
		  {"##########",
		   "#....#...#",
		   "#.##.#.#.#",
		   "#.#..#.#.#",
		   "#.#.##.#.#",
		   "#.#....#.#",
		   "#.####.#.#",
		   "#......#.#",
		   "#.######.#",
		   "##########"});


		CHECK(g.width() == 10u);
		CHECK(g.height() == 10u);

		ivec2 start{1, 1};
		ivec2 goal{8, 8};

		auto path = a_star(g, start, goal, [](const u8 c) { return c == '.'; }); // explicit walkable callback
		REQUIRE(path.has_value());
		const auto& p = path.value();
		CHECK(p.size() == 25u);
		CHECK(p.front() == start);
		CHECK(p.back() == goal);

		// draw path on grid
		for (std::size_t i = 1; i < p.size() - 1; ++i) // don't overwrite start and goal
			g.set(p[i], 'o');


		grid<u8> g_solved;
		g_solved.from_lines(
		  {"##########",
		   "#.ooo#ooo#",
		   "#.##o#o#o#",
		   "#.#oo#o#o#",
		   "#.#o##o#o#",
		   "#.#oooo#o#",
		   "#.####.#o#",
		   "#......#o#",
		   "#.######.#",
		   "##########"});

		REQUIRE(g.hash() == g_solved.hash());


		// check that each step is adjacent to the previous one
		for (std::size_t i = 1; i < p.size(); ++i)
			CHECK(std::abs(p[i].x - p[i - 1].x) + std::abs(p[i].y - p[i - 1].y) == 1);
	}

	SECTION("a_star straight line")
	{
		grid<u8> g(5, 1);

		auto path = a_star(g, ivec2{0, 0}, ivec2{4, 0});
		CHECK(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 5u);
		CHECK(p.front() == ivec2{0, 0});
		CHECK(p.back() == ivec2{4, 0});
	}

	SECTION("a_star start equals goal")
	{
		grid<u8> g(3, 3);

		auto path = a_star(g, ivec2{1, 1}, ivec2{1, 1});
		CHECK(path.has_value());
		CHECK(path.value().size() == 1u);
		CHECK(path.value().front() == ivec2{1, 1});
	}

	SECTION("a_star shortest path on open grid")
	{
		grid<u8> g(5, 5);

		auto path = a_star(g, ivec2{0, 0}, ivec2{4, 4});
		CHECK(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 9u);
		CHECK(p.front() == ivec2{0, 0});
		CHECK(p.back() == ivec2{4, 4});

		for (std::size_t i = 1; i < p.size(); ++i)
			CHECK(std::abs(p[i].x - p[i - 1].x) + std::abs(p[i].y - p[i - 1].y) == 1);
	}

	SECTION("a_star goes around wall")
	{
		grid<u8> g(3, 3);
		g.set(1, 0, 1);
		g.set(1, 1, 1);

		auto path = a_star(g, ivec2{0, 0}, ivec2{2, 0});
		CHECK(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 7u);
		CHECK(p[0] == ivec2{0, 0});
		CHECK(p[1] == ivec2{0, 1});
		CHECK(p[2] == ivec2{0, 2});
		CHECK(p[3] == ivec2{1, 2});
		CHECK(p[4] == ivec2{2, 2});
		CHECK(p[5] == ivec2{2, 1});
		CHECK(p[6] == ivec2{2, 0});
	}

	SECTION("a_star path avoids walls")
	{
		grid<u8> g(5, 5);
		g.vline(2, 0, 3, 1);

		auto path = a_star(g, ivec2{0, 0}, ivec2{4, 0});
		CHECK(path.has_value());

		for (const auto& p : path.value())
			CHECK(g.get(p) == 0);
	}

	SECTION("a_star no path")
	{
		grid<u8> g(3, 3);
		g.vline(1, 0, 2, 1);

		CHECK_FALSE(a_star(g, ivec2{0, 0}, ivec2{2, 0}).has_value());
	}

	SECTION("a_star blocked start or goal")
	{
		grid<u8> g(3, 3);
		g.set(0, 0, 1);
		g.set(2, 2, 1);

		CHECK_FALSE(a_star(g, ivec2{0, 0}, ivec2{1, 1}).has_value());
		CHECK_FALSE(a_star(g, ivec2{1, 1}, ivec2{2, 2}).has_value());
	}

	SECTION("a_star out of bounds")
	{
		grid<u8> g(3, 3);

		CHECK_FALSE(a_star(g, ivec2{-1, 0}, ivec2{2, 2}).has_value());
		CHECK_FALSE(a_star(g, ivec2{0, 0}, ivec2{3, 0}).has_value());
		CHECK_FALSE(a_star(g, ivec2{0, 0}, ivec2{0, 3}).has_value());
	}

	SECTION("a_star with predicate")
	{
		grid<char> g;
		g.from_lines({"S.#G", "..#.", "...."});

		auto path = a_star(g, ivec2{0, 0}, ivec2{3, 0}, [](const char c) { return c != '#'; });
		CHECK(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 8u);
		CHECK(p.front() == ivec2{0, 0});
		CHECK(p.back() == ivec2{3, 0});

		for (const auto& pos : p)
			CHECK(g.get(pos) != '#');
	}

	SECTION("a_star predicate blocks everything")
	{
		grid<char> g(3, 3);
		g.fill('.');

		CHECK_FALSE(a_star(g, ivec2{0, 0}, ivec2{2, 2}, [](const char) { return false; }).has_value());
	}

	SECTION("bfs open grid")
	{
		grid<u8> g(5, 5);

		auto path = bfs(g, ivec2{0, 0}, ivec2{4, 4});
		REQUIRE(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 9u);
		CHECK(p.front() == ivec2{0, 0});
		CHECK(p.back() == ivec2{4, 4});

		for (std::size_t i = 1; i < p.size(); ++i)
			CHECK(std::abs(p[i].x - p[i - 1].x) + std::abs(p[i].y - p[i - 1].y) == 1);
	}

	SECTION("bfs start equals goal")
	{
		grid<u8> g(3, 3);

		auto path = bfs(g, ivec2{1, 1}, ivec2{1, 1});
		REQUIRE(path.has_value());
		CHECK(path.value().size() == 1u);
		CHECK(path.value().front() == ivec2{1, 1});
	}

	SECTION("bfs goes around wall")
	{
		grid<u8> g(3, 3);
		g.set(1, 0, 1);
		g.set(1, 1, 1);

		auto path = bfs(g, ivec2{0, 0}, ivec2{2, 0});
		REQUIRE(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 7u);
		CHECK(p.front() == ivec2{0, 0});
		CHECK(p.back() == ivec2{2, 0});

		for (const auto& pos : p)
			CHECK(g.get(pos) == 0);
	}

	SECTION("bfs no path")
	{
		grid<u8> g(3, 3);
		g.vline(1, 0, 2, 1);

		CHECK_FALSE(bfs(g, ivec2{0, 0}, ivec2{2, 0}).has_value());
	}

	SECTION("bfs blocked or out of bounds")
	{
		grid<u8> g(3, 3);
		g.set(2, 2, 1);

		CHECK_FALSE(bfs(g, ivec2{0, 0}, ivec2{2, 2}).has_value());
		CHECK_FALSE(bfs(g, ivec2{-1, 0}, ivec2{1, 1}).has_value());
		CHECK_FALSE(bfs(g, ivec2{0, 0}, ivec2{3, 0}).has_value());
	}

	SECTION("bfs with predicate")
	{
		grid<char> g;
		g.from_lines({"S.#G", "..#.", "...."});

		auto path = bfs(g, ivec2{0, 0}, ivec2{3, 0}, [](const char c) { return c != '#'; });
		REQUIRE(path.has_value());

		const auto& p = path.value();
		CHECK(p.size() == 8u);

		for (const auto& pos : p)
			CHECK(g.get(pos) != '#');
	}

	SECTION("bfs matches a_star length")
	{
		grid<u8> g;
		g.from_lines(
		  {"##########",
		   "#....#...#",
		   "#.##.#.#.#",
		   "#.#..#.#.#",
		   "#.#.##.#.#",
		   "#.#....#.#",
		   "#.####.#.#",
		   "#......#.#",
		   "#.######.#",
		   "##########"});

		const auto open = [](const u8 c) { return c != '#'; };

		auto b = bfs(g, ivec2{1, 1}, ivec2{8, 8}, open);
		auto a = a_star(g, ivec2{1, 1}, ivec2{8, 8}, open);
		auto d = dfs(g, ivec2{1, 1}, ivec2{8, 8}, open);
		REQUIRE(b.has_value());
		REQUIRE(a.has_value());
		CHECK(b.value().size() == a.value().size());
		CHECK(d.value().size() >= b.value().size()); // DFS may not find the shortest path
	}

	SECTION("DFS")
	{
		grid<u8> g;
		g.from_lines(
		  {"##########",
		   "#....#...#",
		   "#.##.#.#.#",
		   "#.#..#.#.#",
		   "#.#.##.#.#",
		   "#.#....#.#",
		   "#.####.#.#",
		   "#......#.#",
		   "#.######.#",
		   "##########"});

		const auto open = [](const u8 c) { return c != '#'; };

		auto path = dfs(g, ivec2{1, 1}, ivec2{8, 8}, open);
		REQUIRE(path.has_value());
		CHECK(path.value().front() == ivec2{1, 1});
		CHECK(path.value().back() == ivec2{8, 8});
		CHECK(path.value().size() >= 25u); // DFS may not find the shortest path
	}
}

TEST_CASE("grid3d rotate", "[grid][grid3d]")
{
	using namespace deckard;

	SECTION("quarter turns rotate coordinates and extents")
	{
		grid3d<char> around_x(2, 3, 4);
		around_x.set({1, 2, 3}, 'x');
		around_x.set_face({1, 0, 3}, grid3d_directions::top, 't');
		around_x.rotate({1, 0, 0}, 90);
		CHECK(around_x.get({1, 0, 2}) == 'x');
		CHECK(around_x.get_face({1, 0, 0}, grid3d_directions::front) == 't');
		CHECK_FALSE(around_x.get({1, 4, 0}).has_value());
		around_x.rotate({1, 0, 0}, -90);
		CHECK(around_x.get({1, 2, 3}) == 'x');

		grid3d<char> around_y(2, 3, 4);
		around_y.set({1, 2, 3}, 'y');
		around_y.rotate({0, 1, 0}, 90);
		CHECK(around_y.get({0, 2, 1}) == 'y');
		CHECK_FALSE(around_y.get({4, 0, 0}).has_value());

		grid3d<char> around_z(2, 3, 4);
		around_z.set({1, 2, 3}, 'z');
		around_z.rotate({0, 0, 1}, 90);
		CHECK(around_z.get({0, 1, 3}) == 'z');
		CHECK_FALSE(around_z.get({3, 0, 0}).has_value());
	}

	SECTION("signed axes and full turns")
	{
		grid3d<char> g(2, 3, 4);
		g.set({1, 2, 3}, 'v');
		g.rotate({-1, 0, 0}, 90);
		CHECK(g.get({1, 3, 0}) == 'v');
		g.rotate({0, 0, 1}, 360);
		CHECK(g.get({1, 3, 0}) == 'v');
	}
}

TEST_CASE("rubiks cube test", "[grid][grid3d]")
{
	using namespace deckard;
	using namespace deckard::vec;

	std::array<grid<char>, 6> faces{
	  grid<char>{3, 3}, grid<char>{3, 3}, grid<char>{3, 3}, grid<char>{3, 3}, grid<char>{3, 3}, grid<char>{3, 3}};
	faces[0].fill('F');
	faces[1].fill('B');
	faces[2].fill('L');
	faces[3].fill('R');
	faces[4].fill('U');
	faces[5].fill('D');

	grid3d<char> cube(3, 3, 3);
	cube.load_from_grids(faces);

	// This corner cubie retains all three stickers rather than one overwriting the others.
	CHECK(cube.get_face({0, 0, 0}, grid3d_directions::front) == 'F');
	CHECK(cube.get_face({0, 0, 0}, grid3d_directions::left) == 'L');
	CHECK(cube.get_face({0, 0, 0}, grid3d_directions::top) == 'U');

	CHECK(cube.getplane(grid3d_directions::front).get(0, 0) == 'F');
	CHECK(cube.getplane(grid3d_directions::left).get(0, 0) == 'L');
	CHECK(cube.getplane(grid3d_directions::top).get(0, 0) == 'U');
	CHECK(
	  cube.print()
	  == "    UUU\n"
		 "    UUU\n"
		 "    UUU\n"
		 "LLL FFF RRR BBB\n"
		 "LLL FFF RRR BBB\n"
		 "LLL FFF RRR BBB\n"
		 "    DDD\n"
		 "    DDD\n"
		 "    DDD\n");


	SECTION("rotate cube around y axis")
	{

		cube.rotate({0, 1, 0}, 90);
		CHECK(cube.getplane(grid3d_directions::front).get(0, 0) == 'L');
		CHECK(cube.getplane(grid3d_directions::left).get(0, 0) == 'B');
		CHECK(cube.getplane(grid3d_directions::back).get(0, 0) == 'R');
		CHECK(cube.getplane(grid3d_directions::right).get(0, 0) == 'F');

		CHECK(
		  cube.print()
		  == "    UUU\n"
			 "    UUU\n"
			 "    UUU\n"
			 "BBB LLL FFF RRR\n"
			 "BBB LLL FFF RRR\n"
			 "BBB LLL FFF RRR\n"
			 "    DDD\n"
			 "    DDD\n"
			 "    DDD\n");
	}
}

TEST_CASE("grid3d load from six faces", "[grid][grid3d]")
{
	using namespace deckard;
	using namespace deckard::vec;
	CHECK(grid3d_directions::top == ivec3{0, -1, 0});
	CHECK(grid3d_directions::bottom == ivec3{0, 1, 0});
	CHECK(grid3d_directions::left == ivec3{-1, 0, 0});
	CHECK(grid3d_directions::right == ivec3{1, 0, 0});
	CHECK(grid3d_directions::front == ivec3{0, 0, -1});
	CHECK(grid3d_directions::back == ivec3{0, 0, 1});

	std::array<grid<char>, 6> faces{
	  grid<char>{4, 4}, grid<char>{4, 4}, grid<char>{4, 4}, grid<char>{4, 4}, grid<char>{4, 4}, grid<char>{4, 4}};
	faces[0].fill('f'); // front
	faces[1].fill('b'); // back
	faces[2].fill('l'); // left
	faces[3].fill('r'); // right
	faces[4].fill('t'); // top
	faces[5].fill('d'); // bottom

	grid3d<char> cube(4, 4, 4);
	cube.load_from_grids(faces);

	grid3d<char> partial_cube(4, 4, 4);
	partial_cube.set_plane(faces[0], grid3d_directions::front);
	CHECK(partial_cube.get_face({0, 0, 0}, grid3d_directions::front) == 'f');
	CHECK_FALSE(partial_cube.get_face({0, 0, 0}, grid3d_directions::left).has_value());
	partial_cube.set_plane(faces[2], grid3d_directions::left);
	CHECK(partial_cube.get_face({0, 0, 0}, grid3d_directions::front) == 'f');
	CHECK(partial_cube.get_face({0, 0, 0}, grid3d_directions::left) == 'l');

	CHECK(cube.get_face({1, 1, 0}, grid3d_directions::front) == 'f');
	CHECK(cube.get_face({1, 1, 3}, grid3d_directions::back) == 'b');
	CHECK(cube.get_face({0, 1, 1}, grid3d_directions::left) == 'l');
	CHECK(cube.get_face({3, 1, 1}, grid3d_directions::right) == 'r');
	CHECK(cube.get_face({1, 0, 1}, grid3d_directions::top) == 't');
	CHECK(cube.get_face({1, 3, 1}, grid3d_directions::bottom) == 'd');
	CHECK(cube.get_face({0, 0, 0}, grid3d_directions::front) == 'f');
	CHECK(cube.get_face({0, 0, 0}, grid3d_directions::left) == 'l');
	CHECK(cube.get_face({0, 0, 0}, grid3d_directions::top) == 't');

	const auto front = cube.getplane(grid3d_directions::front);
	CHECK(front.count("f") == 16u);
	CHECK(front.get(1, 1) == 'f');

	const auto grid_back   = cube.getplane(grid3d_directions::back);
	const auto grid_left   = cube.getplane(grid3d_directions::left);
	const auto grid_right  = cube.getplane(grid3d_directions::right);
	const auto grid_top    = cube.getplane(grid3d_directions::top);
	const auto grid_bottom = cube.getplane(grid3d_directions::bottom);
	CHECK(grid_back.get(1, 1) == 'b');
	CHECK(grid_left.get(1, 1) == 'l');
	CHECK(grid_right.get(1, 1) == 'r');
	CHECK(grid_top.get(1, 1) == 't');
	CHECK(grid_bottom.get(1, 1) == 'd');
}

TEST_CASE("grid3d rectangular cuboid planes", "[grid][grid3d]")
{
	using namespace deckard;

	std::array<grid<char>, 6> faces{
	  grid<char>{2, 3}, grid<char>{2, 3}, grid<char>{4, 3}, grid<char>{4, 3}, grid<char>{2, 4}, grid<char>{2, 4}};
	faces[0].fill('F');
	faces[1].fill('B');
	faces[2].fill('L');
	faces[3].fill('R');
	faces[4].fill('T');
	faces[5].fill('D');

	grid3d<char> cuboid(2, 3, 4);
	cuboid.load_from_grids(faces);

	const auto grid_front  = cuboid.getplane(grid3d_directions::front);
	const auto grid_back   = cuboid.getplane(grid3d_directions::back);
	const auto grid_left   = cuboid.getplane(grid3d_directions::left);
	const auto grid_right  = cuboid.getplane(grid3d_directions::right);
	const auto grid_top    = cuboid.getplane(grid3d_directions::top);
	const auto grid_bottom = cuboid.getplane(grid3d_directions::bottom);
	CHECK(grid_front.width() == 2);
	CHECK(grid_front.height() == 3);
	CHECK(grid_back.width() == 2);
	CHECK(grid_back.height() == 3);
	CHECK(grid_left.width() == 4);
	CHECK(grid_left.height() == 3);
	CHECK(grid_right.width() == 4);
	CHECK(grid_right.height() == 3);
	CHECK(grid_top.width() == 2);
	CHECK(grid_top.height() == 4);
	CHECK(grid_bottom.width() == 2);
	CHECK(grid_bottom.height() == 4);
	CHECK(cuboid.get_face({0, 0, 0}, grid3d_directions::front) == 'F');
	CHECK(cuboid.get_face({0, 0, 0}, grid3d_directions::left) == 'L');
	CHECK(cuboid.get_face({0, 0, 0}, grid3d_directions::top) == 'T');
	CHECK(
	  cuboid.print()
	  == "     TT\n"
		 "     TT\n"
		 "     TT\n"
		 "     TT\n"
		 "LLLL FF RRRR BB\n"
		 "LLLL FF RRRR BB\n"
		 "LLLL FF RRRR BB\n"
		 "     DD\n"
		 "     DD\n"
		 "     DD\n"
		 "     DD\n");
}
