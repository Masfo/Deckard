#include <catch2/catch_test_macros.hpp>


import std;
import deckard.helpers;
import deckard.stringhelper;
import deckard.types;
import deckard.as;

using namespace deckard;
using namespace deckard::string;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

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


		std::string_view input_view = input;
		auto             split_view = split<std::string_view>(input_view, "|");
		REQUIRE(split_view.size() == 2);
		REQUIRE(split_view[0] == "hello"sv);
		REQUIRE(split_view[1] == "world"sv);


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


	SECTION("try_index_of")
	{
		// array
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};

		auto idx = try_index_of(input, 40);
		REQUIRE(idx.has_value());
		REQUIRE(*idx == 3);

		// string
		idx = try_index_of("hello world lazy dog"sv, "world");
		REQUIRE(idx.has_value());
		REQUIRE(6 == *idx);

		idx = try_index_of("hello world lazy dog"sv, "l");
		REQUIRE(idx.has_value());
		REQUIRE(2 == *idx);

		idx = try_index_of("hello world lazy dog"sv, "Q");
		REQUIRE(not idx.has_value());
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

	SECTION("try_to_string")
	{
		auto a = try_to_string(0xffff, 16);
		REQUIRE(a.has_value() == true);
		REQUIRE("ffff" == *a);

		a = try_to_string(0xffff);
		REQUIRE(a.has_value() == true);
		REQUIRE("65535" == *a);

		a = try_to_string(-256, 16);
		REQUIRE(a.has_value() == true);
		REQUIRE("-100" == *a);
	}

	SECTION("try_to_number")
	{
		std::string input("123456");

		auto a = try_to_number(input);

		REQUIRE(a.has_value() == true);
		REQUIRE(*a == 123456);

		input  = "123.456";
		auto b = try_to_number<f32>(input);
		REQUIRE(b.has_value() == true);
		REQUIRE(*b == 123.456f);

		input  = "-123.456";
		auto c = try_to_number<f32>(input);
		REQUIRE(c.has_value() == true);
		REQUIRE(*c == -123.456f);

		input   = "456.654";
		auto c2 = try_to_number<f64>(input);
		REQUIRE(c2.has_value() == true);
		REQUIRE(*c2 == 456.654);

		input  = "+128";
		auto d = try_to_number<u8>(input);
		REQUIRE(d.has_value() == true);
		REQUIRE(*d == 128);

		input  = "#29A";
		auto e = try_to_number<i16>(input);
		REQUIRE(e.has_value() == true);
		REQUIRE(*e == 666);


		input  = "hello";
		auto f = try_to_number<i16>(input);
		REQUIRE(f.has_value() == false);
	}

	SECTION("as")
	{
		//
		REQUIRE("ffff"sv == as<std::string>(0xFFFF, 16));
		REQUIRE(0xFFFF == as<u32>("0xFFFF", 16));
	}

	SECTION("prettytime")
	{

		REQUIRE("2min 8s 678ms"sv == pretty_time(std::chrono::duration<f64>(std::chrono::seconds{123} + 5678ms)));

		REQUIRE("1d 23min 16s 213ms 741us"sv == pretty_time(std::chrono::duration(std::chrono::days{1} + 23min + 16s + 213ms + 741us)));
	}

	SECTION("human_readable_bytes")
	{
		REQUIRE("0 bytes"sv == human_readable_bytes(0));
		REQUIRE("1 MiB"sv == human_readable_bytes(1_MiB));
		REQUIRE("1.02 MiB"sv == human_readable_bytes(1_MiB + 20_KiB));
		REQUIRE("3.14 GiB"sv == human_readable_bytes(3_GiB + 140_MiB + 100_KiB));
	}

	SECTION("pretty_bytes")
	{
		REQUIRE("128 bytes"sv == pretty_bytes(128));
		REQUIRE("1 KiB, 1 byte"sv == pretty_bytes(1024 + 1));


		REQUIRE("1 MiB, 256 KiB, 128 bytes"sv == pretty_bytes(1_MiB + 256_KiB + 128));
	}
}
