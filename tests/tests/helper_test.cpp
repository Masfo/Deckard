#include <catch2/catch_test_macros.hpp>


import std;
import deckard.helpers;
import deckard.types;

using namespace deckard;

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
		std::string input("\t hello \t");
		auto        trimmed = trim_front(input);

		REQUIRE(trimmed == "hello \t");

		trimmed = trim_back(trimmed);
		REQUIRE(trimmed == "hello");

		trimmed = trim("\t\n hello \n\t");
		REQUIRE(trimmed == "hello");
	}


	SECTION("index_of")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		REQUIRE(index_of(input, 40) == 3);
	}

	SECTION("take")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30};

		auto first = take(input, 3);
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
