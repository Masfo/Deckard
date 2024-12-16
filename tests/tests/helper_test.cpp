#include <catch2/catch_test_macros.hpp>


import std;
import deckard.helpers;
import deckard.stringhelper;
import deckard.types;
import deckard.as;
import deckard.enums;

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


		input    = "aa,bb.cc";
		splitted = split(input, ",.");
		REQUIRE(splitted.size() == 3);
		REQUIRE(splitted[0] == "aa");
		REQUIRE(splitted[1] == "bb");
		REQUIRE(splitted[2] == "cc");
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

	SECTION("match")
	{
		REQUIRE(true == match("*X*", "helloXworld"));
		REQUIRE(false == match("*Y*", "helloXworld"));

		REQUIRE(true == match("?X*world", "XXworld"));
		REQUIRE(false == match("?X*world", "XYworld"));
	}

	SECTION("ints/static")
	{
		auto [p1, p2, p3, p4] = ints<4>("p=12,34 v=45,56");

		REQUIRE(p1 == 12);
		REQUIRE(p2 == 34);
		REQUIRE(p3 == 45);
		REQUIRE(p4 == 56);
	}

	SECTION("ints/dynamic")
	{
		auto ps = ints("0: p=12,34 v=45,56");
		REQUIRE(ps.size() == 5);
		REQUIRE(ps[0] == 0);
		REQUIRE(ps[1] == 12);
		REQUIRE(ps[2] == 34);
		REQUIRE(ps[3] == 45);
		REQUIRE(ps[4] == 56);
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

	SECTION("head")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30};

		auto first = head(input, 3);
		REQUIRE(first.size() == real.size());
		REQUIRE(first == real);
	}

	SECTION("head-array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 3> real{10, 20, 30};

		auto first = head<3>(input);
		REQUIRE(first.size() == real.size());
		REQUIRE(first == real);
	}

	SECTION("tail")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{40, 50, 60, 70, 80};

		auto first = tail(input, 3);
		REQUIRE(first.size() == real.size());
		REQUIRE(first == real);
	}

	SECTION("tail-array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 5> real{40, 50, 60, 70, 80};

		auto first = tail<3>(input);
		REQUIRE(first.size() == real.size());
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

	SECTION("strip-option")
	{
		std::string input("+ABC x 123 y DEF-");

		using enum string::option;
		input = strip(input, w | u | d);
		REQUIRE(input == "+xy-");

		REQUIRE("123" == strip("123abcABC\t#", a | w | s));
	}

	SECTION("include only")
	{
		std::string input("ABC123DEF");

		using enum string::option;
		input = include_only(input, d);
		REQUIRE(input == "123");
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

	SECTION("concat/integer")
	{
		REQUIRE(1080 == concat(10, 80));

		REQUIRE(1234 == concat(1, 2, 3, 4));
	}

	SECTION("count digits")
	{
		REQUIRE(1 == count_digits(0));

		REQUIRE(2 == count_digits(10));
		REQUIRE(20 == count_digits(0xFFFF'FFFF'FFFF'FFFF));

		REQUIRE(4 == count_digits(-2024));
	}

	SECTION("even/odd")
	{
		REQUIRE(is_even(0) != is_odd(0));

		REQUIRE(is_odd(1) == true);
		REQUIRE(is_odd(2) == false);

		REQUIRE(is_even(1) == false);
		REQUIRE(is_even(2) == true);


		REQUIRE(is_odd(-2) == false);
		REQUIRE(is_even(-2) == true);

		REQUIRE(true == (5 | is_odd));
		REQUIRE(true == (4 | is_even));
	}

	SECTION("split digit")
	{
		REQUIRE(split_digit(1080) == std::make_pair(10, 80));
		REQUIRE(split_digit(-1080) == std::make_pair(-10, 80));

		REQUIRE(split_digit(12) == std::make_pair(1, 2));
		REQUIRE(split_digit(-12) == std::make_pair(-1, 2));


		REQUIRE(split_digit(2048) == std::pair<u64, u64>{20, 48});

		REQUIRE(split_digit(123'456_u64) == std::make_pair(123, 456));
		REQUIRE(split_digit(123'456_i64) == std::make_pair(123, 456));

		REQUIRE(split_digit(9'876'543) == std::make_pair(9876, 543));
	}

	SECTION("concat/vector")
	{
		auto v1 = make_vector(1, 2);
		auto v2 = make_vector(3, 4);
		auto v3 = make_vector(5, 6);
		auto v4 = concat(v1, v2, v3);

		REQUIRE(v4.size() == 6);
		REQUIRE(v4 == make_vector(1, 2, 3, 4, 5, 6));
	}

	SECTION("concat/array")
	{
		auto v1 = make_array(1, 2);
		auto v2 = make_array(3, 4);
		auto v3 = make_array(5, 6);
		auto v4 = concat(v1, v2, v3);

		REQUIRE(v4.size() == 6);
		REQUIRE(v4 == make_array(1, 2, 3, 4, 5, 6));
	}

	SECTION("kcombo/dynamic")
	{
		const auto dyna = kcombo("ABCD"sv, 2);

		REQUIRE(dyna.size() == 6);
		REQUIRE(dyna[0] == make_vector('A', 'B'));
		REQUIRE(dyna[1] == make_vector('A', 'C'));
		REQUIRE(dyna[2] == make_vector('A', 'D'));
		REQUIRE(dyna[3] == make_vector('B', 'C'));
		REQUIRE(dyna[4] == make_vector('B', 'D'));
		REQUIRE(dyna[5] == make_vector('C', 'D'));

		for (const auto& [i, v] : std::views::enumerate(kcombo("ABCD"sv, 2)))
		{
			REQUIRE(v == dyna[i]);
		}


		const auto dyna3 = kcombo("ABCD"sv, 3);
		REQUIRE(dyna3.size() == 4);
		REQUIRE(dyna3[0] == make_vector('A', 'B', 'C'));
		REQUIRE(dyna3[1] == make_vector('A', 'B', 'D'));
		REQUIRE(dyna3[2] == make_vector('A', 'C', 'D'));
		REQUIRE(dyna3[3] == make_vector('B', 'C', 'D'));
	}

	SECTION("kcombo/static")
	{
		const auto dyna = kcombo<2>("ABCD"sv);

		REQUIRE(dyna.size() == 6);
		REQUIRE(dyna[0] == make_array('A', 'B'));
		REQUIRE(dyna[1] == make_array('A', 'C'));
		REQUIRE(dyna[2] == make_array('A', 'D'));
		REQUIRE(dyna[3] == make_array('B', 'C'));
		REQUIRE(dyna[4] == make_array('B', 'D'));
		REQUIRE(dyna[5] == make_array('C', 'D'));

		i32 i = 0;
		for (const auto [a, b] : kcombo<2>("ABCD"sv))
		{
			REQUIRE(make_array(a, b) == dyna[i++]);
		}


		for (const auto [index, v] : std::views::enumerate(kcombo<2>("ABCD"sv)))
		{
			const auto [a, b] = v;
			REQUIRE(make_array(a, b) == dyna[index]);
		}
	}

	SECTION("try_to_number")
	{
		std::string input("123456");

		auto a = try_to_number(input);

		REQUIRE(a.has_value() == true);
		REQUIRE(*a == 123'456);

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

	SECTION("to_number")
	{
		REQUIRE(123 == to_number("123"));
		REQUIRE(0 == to_number("xyz"));
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
