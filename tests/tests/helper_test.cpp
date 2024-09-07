#include <catch2/catch_test_macros.hpp>


import std;
import deckard.helpers;
import deckard.types;

using namespace deckard;
using namespace std::string_view_literals;

TEST_CASE("helpers", "[helpers]")
{
	SECTION("replace")
	{
		std::string input("hello|world");
		input = replace(input, "|", "=");

		REQUIRE(input == "hello=world");
	}

	SECTION("split")
	{
		std::string input("hello|world");
		auto        splitted = split(input, "|");

		REQUIRE(splitted.size() == 2);
		REQUIRE(splitted[0] == "hello");
		REQUIRE(splitted[1] == "world");
	}

	SECTION("split_exact")
	{
		std::string input("hello||world|dog||cat");
		auto        splitted = split_exact(input, "||");

		REQUIRE(splitted.size() == 3);
		REQUIRE(splitted[0] == "hello");
		REQUIRE(splitted[1] == "world|dog");
		REQUIRE(splitted[2] == "cat");
	}

	SECTION("split_exact")
	{
		std::string input("hello||||world|dog||cat");
		auto        splitted = split_exact(input, "||", true);

		REQUIRE(splitted.size() == 4);
		REQUIRE(splitted[0] == "hello");
		REQUIRE(splitted[1] == "");
		REQUIRE(splitted[2] == "world|dog");
		REQUIRE(splitted[3] == "cat");
	}

	SECTION("split_exact")
	{
		std::string input("helloWORLDmagic");
		auto        splitted = split_exact(input, 5);

		REQUIRE(splitted.size() == 2);
		REQUIRE(splitted[0] == "hello");
		REQUIRE(splitted[1] == "WORLDmagic");
	}

	SECTION("split_stride")
	{
		std::string input("helloWORLDmagic");
		auto        splitted = split_stride(input, 5);

		REQUIRE(splitted.size() == 3);
		REQUIRE(splitted[0] == "hello");
		REQUIRE(splitted[1] == "WORLD");
		REQUIRE(splitted[2] == "magic");
	}

	SECTION("split_once")
	{
		std::string input("hello|world|123");
		auto        splitted = split_once(input, "|");

		REQUIRE(splitted.size() == 2);
		REQUIRE(splitted[0] == "hello");
		REQUIRE(splitted[1] == "world|123");
	}

	SECTION("trim")
	{
		REQUIRE(trim_front("\t\n hello \n\t") == "hello \n\t");
		REQUIRE(trim_back("\t\n hello \n\t") == "\t\n hello");
		REQUIRE(trim("\t\n hello \n\t") == "hello");
	}


	SECTION("index_of")
	{
		// array
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		REQUIRE(index_of(input, 40) == 3);

		// string
		REQUIRE(6 == index_of("hello world lazy dog"sv, "world"));
		REQUIRE(2 == index_of("hello world lazy dog"sv, "l"));

		REQUIRE(max_value<u64> == index_of("hello world lazy dog"sv, "Q"));
	}

	SECTION("take")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30};

		auto first = take(input, 3);
		REQUIRE(first.size() == 3);
		REQUIRE(first == real);
	}

	SECTION("take array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 3> real{10, 20, 30};


		auto first = take<3>(input);
		REQUIRE(first.size() == 3);
		REQUIRE(first == real);
	}

	SECTION("last")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{60, 70, 80};

		auto lastof = last(input, 3);
		REQUIRE(lastof.size() == 3);
		REQUIRE(lastof == real);
	}

	SECTION("last array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 3> real{60, 70, 80};


		auto first = last<3>(input);
		REQUIRE(first.size() == 3);
		REQUIRE(first == real);
	}

	SECTION("strip-range")
	{
		std::string input("+ABC123DEF-");

		input = strip(input, 'A', 'Z');
		REQUIRE(input == "+123-");
	}

	SECTION("strip")
	{
		std::string input("+ABC123DEF-");

		input = strip(input, "123+-BDF");
		REQUIRE(input == "ACE");
	}
}
