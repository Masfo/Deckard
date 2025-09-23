#include <catch2/catch_all.hpp>
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

TEST_CASE("as", "[as]")
{
	SECTION("string to number")
	{

		CHECK(123 == as<i32>("123"));
		CHECK(123.456f == as<f32>("123.456"));
		CHECK(-123.456f == as<f32>("-123.456"));
		CHECK(456.654 == as<f64>("456.654"));
		CHECK(128 == as<u8>("128"));
		CHECK(666 == as<i16>("#29A", 16));
	}
	SECTION("number to string")
	{
		CHECK("ffff"sv == as<std::string>(0xFFFF, 16));
		CHECK(0xFFFF == as<u32>("0xFFFF", 16));
	}

	SECTION("float to integer")
	{
		CHECK(123 == as<i32>(123.456f));
		CHECK(-456 == as<i32>(-456.456f));
	}

	SECTION("float to float") { }

} // namespace deckard::helpers

TEST_CASE("helpers", "[helpers]")
{
	SECTION("replace")
	{
		std::string input("hello|world");
		input = replace(input, "|", "=");

		CHECK(input == "hello=world");
	}

	SECTION("split")
	{
		std::string input("hello|world");
		auto        splitted = split(input, "|");

		CHECK(splitted.size() == 2);
		CHECK(splitted[0] == "hello");
		CHECK(splitted[1] == "world");


		std::string_view input_view = input;
		auto             split_view = split<std::string_view>(input_view, "|");
		CHECK(split_view.size() == 2);
		CHECK(split_view[0] == "hello"sv);
		CHECK(split_view[1] == "world"sv);


		input    = "aa,bb.cc";
		splitted = split(input, ",.");
		CHECK(splitted.size() == 3);
		CHECK(splitted[0] == "aa");
		CHECK(splitted[1] == "bb");
		CHECK(splitted[2] == "cc");
	}


	SECTION("split_exact")
	{
		std::string input("hello||world|dog||cat");
		auto        splitted = split_exact(input, "||");

		CHECK(splitted.size() == 3);
		CHECK(splitted[0] == "hello");
		CHECK(splitted[1] == "world|dog");
		CHECK(splitted[2] == "cat");
	}

	SECTION("split_exact")
	{
		std::string input("hello||||world|dog||cat");
		auto        splitted = split_exact(input, "||", true);

		CHECK(splitted.size() == 4);
		CHECK(splitted[0] == "hello");
		CHECK(splitted[1] == "");
		CHECK(splitted[2] == "world|dog");
		CHECK(splitted[3] == "cat");
	}

	SECTION("split_exact")
	{
		std::string input("helloWORLDmagic");
		auto        splitted = split_exact(input, 5);

		CHECK(splitted.size() == 2);
		CHECK(splitted[0] == "hello");
		CHECK(splitted[1] == "WORLDmagic");
	}

	SECTION("split_stride")
	{
		std::string input("helloWORLDmagic");
		auto        splitted = split_stride(input, 5);

		CHECK(splitted.size() == 3);
		CHECK(splitted[0] == "hello");
		CHECK(splitted[1] == "WORLD");
		CHECK(splitted[2] == "magic");
	}

	SECTION("split_once")
	{
		std::string input("hello|world|123");
		auto        splitted = split_once(input, "|");

		CHECK(splitted.size() == 2);
		CHECK(splitted[0] == "hello");
		CHECK(splitted[1] == "world|123");
	}

	SECTION("trim")
	{
		CHECK(trim_front("\t\n hello \n\t") == "hello \n\t");
		CHECK(trim_back("\t\n hello \n\t") == "\t\n hello");
		CHECK(trim("\t\n hello \n\t") == "hello");
	}

	SECTION("match")
	{
		CHECK(true == match("*X*", "helloXworld"));
		CHECK(false == match("*Y*", "helloXworld"));

		CHECK(true == match("?X*world", "XXworld"));
		CHECK(false == match("?X*world", "XYworld"));
	}

	SECTION("ints/static")
	{
		auto [p1, p2, p3, p4] = ints<4>("p=-66,34 v=45,56");

		CHECK(p1 == -66);
		CHECK(p2 == 34);
		CHECK(p3 == 45);
		CHECK(p4 == 56);


		i64 t1 = ints<1>("hello 123");
		CHECK(t1 == 123);
	}

	SECTION("ints/dynamic")
	{
		auto ps = ints("0: p=12,34 v=-45,56");
		CHECK(ps.size() == 5);
		CHECK(ps[0] == 0);
		CHECK(ps[1] == 12);
		CHECK(ps[2] == 34);
		CHECK(ps[3] == -45);
		CHECK(ps[4] == 56);
	}


	SECTION("try_index_of")
	{
		// array
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};

		auto idx = try_index_of(input, 40);
		CHECK(idx.has_value());
		CHECK(*idx == 3);

		// string
		idx = try_index_of("hello world lazy dog"sv, "world");
		CHECK(idx.has_value());
		CHECK(6 == *idx);

		idx = try_index_of("hello world lazy dog"sv, "l");
		CHECK(idx.has_value());
		CHECK(2 == *idx);

		idx = try_index_of("hello world lazy dog"sv, "Q");
		CHECK(not idx.has_value());
	}

	SECTION("take")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30};

		auto first = take(input, 3);
		CHECK(first.size() == 3);
		CHECK(first == real);
	}

	SECTION("take array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 3> real{10, 20, 30};


		auto first = take<3>(input);
		CHECK(first.size() == 3);
		CHECK(first == real);
	}

	SECTION("head")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30};

		auto first = head(input, 3);
		CHECK(first.size() == real.size());
		CHECK(first == real);
	}

	SECTION("head-array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 3> real{10, 20, 30};

		auto first = head<3>(input);
		CHECK(first.size() == real.size());
		CHECK(first == real);
	}

	SECTION("tail")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{40, 50, 60, 70, 80};

		auto first = tail(input, 3);
		CHECK(first.size() == real.size());
		CHECK(first == real);
	}

	SECTION("tail-array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 5> real{40, 50, 60, 70, 80};

		auto first = tail<3>(input);
		CHECK(first.size() == real.size());
		CHECK(first == real);
	}

	SECTION("last")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{60, 70, 80};

		auto lastof = last(input, 3);
		CHECK(lastof.size() == 3);
		CHECK(lastof == real);
	}

	SECTION("last array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 3> real{60, 70, 80};


		auto first = last<3>(input);
		CHECK(first.size() == 3);
		CHECK(first == real);
	}

	SECTION("strip-range")
	{
		std::string input("+ABC123DEF-");

		input = strip(input, 'A', 'Z');
		CHECK(input == "+123-");
	}

	SECTION("strip")
	{
		std::string input("+ABC123DEF-");

		input = strip(input, "123+-BDF");
		CHECK(input == "ACE");
	}

	SECTION("strip-option")
	{
		std::string input("+ABC x 123 y DEF-");

		using enum string::option;
		input = strip(input, w | u | d);
		CHECK(input == "+xy-");

		CHECK("123" == strip("123abcABC\t#", a | w | s));
	}

	SECTION("include only")
	{
		std::string input("ABC123DEF");

		using enum string::option;
		input = include_only(input, d);
		CHECK(input == "123");
	}

	SECTION("try_to_string")
	{
		auto a = try_to_string(0xffff, 16);
		CHECK(a.has_value() == true);
		CHECK("ffff" == *a);

		a = try_to_string(0xffff);
		CHECK(a.has_value() == true);
		CHECK("65535" == *a);

		a = try_to_string(-256, 16);
		CHECK(a.has_value() == true);
		CHECK("-100" == *a);
	}

	SECTION("concat/integer")
	{
		CHECK(1080 == concat(10, 80));

		CHECK(1234 == concat(1, 2, 3, 4));
	}

	SECTION("count digits")
	{
		CHECK(1 == count_digits(0));

		CHECK(2 == count_digits(10));
		CHECK(20 == count_digits(0xFFFF'FFFF'FFFF'FFFF));

		CHECK(4 == count_digits(-2024));
	}

	SECTION("even/odd")
	{
		CHECK(is_even(0) != is_odd(0));

		CHECK(is_odd(1) == true);
		CHECK(is_odd(2) == false);

		CHECK(is_even(1) == false);
		CHECK(is_even(2) == true);


		CHECK(is_odd(-2) == false);
		CHECK(is_even(-2) == true);

		CHECK(true == (5 | is_odd));
		CHECK(true == (4 | is_even));
	}

	SECTION("split digit")
	{
		CHECK(split_digit(1080) == std::make_pair(10, 80));
		CHECK(split_digit(-1080) == std::make_pair(-10, 80));

		CHECK(split_digit(12) == std::make_pair(1, 2));
		CHECK(split_digit(-12) == std::make_pair(-1, 2));


		CHECK(split_digit(2048) == std::pair<u64, u64>{20, 48});

		CHECK(split_digit(123'456_u64) == std::make_pair(123, 456));
		CHECK(split_digit(123'456_i64) == std::make_pair(123, 456));

		CHECK(split_digit(9'876'543) == std::make_pair(9876, 543));
	}

	SECTION("concat/vector")
	{
		auto v1 = make_vector(1, 2);
		auto v2 = make_vector(3, 4);
		auto v3 = make_vector(5, 6);
		auto v4 = concat(v1, v2, v3);

		CHECK(v4.size() == 6);
		CHECK(v4 == make_vector(1, 2, 3, 4, 5, 6));
	}

	SECTION("concat/array")
	{
		auto v1 = make_array(1, 2);
		auto v2 = make_array(3, 4);
		auto v3 = make_array(5, 6);
		auto v4 = concat(v1, v2, v3);

		CHECK(v4.size() == 6);
		CHECK(v4 == make_array(1, 2, 3, 4, 5, 6));
	}

	
	SECTION("kperm/dynamic")
	{



		//const auto dyna = kperm<2>("ABC"sv);
		//
		//CHECK(dyna.size() == 6);
		//CHECK(dyna[0] == make_vector('A', 'B'));
		//CHECK(dyna[1] == make_vector('A', 'C'));
		//CHECK(dyna[2] == make_vector('B', 'A'));
		//CHECK(dyna[3] == make_vector('B', 'C'));
		//CHECK(dyna[4] == make_vector('C', 'A'));
		//CHECK(dyna[5] == make_vector('C', 'B'));

	}

	SECTION("unique_permutation/dynamic")
	{
		const auto dyna = unique_permutation("ABCD"sv, 2);

		CHECK(dyna.size() == 6);
		CHECK(dyna[0] == make_vector('A', 'B'));
		CHECK(dyna[1] == make_vector('A', 'C'));
		CHECK(dyna[2] == make_vector('A', 'D'));
		CHECK(dyna[3] == make_vector('B', 'C'));
		CHECK(dyna[4] == make_vector('B', 'D'));
		CHECK(dyna[5] == make_vector('C', 'D'));

		for (const auto& [i, v] : std::views::enumerate(unique_permutation("ABCD"sv, 2)))
		{
			CHECK(v == dyna[i]);
		}


		const auto dyna3 = unique_permutation("ABCD"sv, 3);
		CHECK(dyna3.size() == 4);
		CHECK(dyna3[0] == make_vector('A', 'B', 'C'));
		CHECK(dyna3[1] == make_vector('A', 'B', 'D'));
		CHECK(dyna3[2] == make_vector('A', 'C', 'D'));
		CHECK(dyna3[3] == make_vector('B', 'C', 'D'));
	}

	SECTION("unique_permutation/static")
	{
		const auto dyna = unique_permutation<2>("ABCD"sv);

		CHECK(dyna.size() == 6);
		CHECK(dyna[0] == make_array('A', 'B'));
		CHECK(dyna[1] == make_array('A', 'C'));
		CHECK(dyna[2] == make_array('A', 'D'));
		CHECK(dyna[3] == make_array('B', 'C'));
		CHECK(dyna[4] == make_array('B', 'D'));
		CHECK(dyna[5] == make_array('C', 'D'));

		i32 i = 0;
		for (const auto [a, b] : unique_permutation<2>("ABCD"sv))
		{
			CHECK(make_array(a, b) == dyna[i++]);
		}


		for (const auto [index, v] : std::views::enumerate(unique_permutation<2>("ABCD"sv)))
		{
			const auto [a, b] = v;
			CHECK(make_array(a, b) == dyna[index]);
		}
	}

	SECTION("try_to_number")
	{
		std::string input("123456");

		auto a = try_to_number(input);

		CHECK(a.has_value() == true);
		CHECK(*a == 123'456);

		input  = "123.456";
		auto b = try_to_number<f32>(input);
		CHECK(b.has_value() == true);
		CHECK(*b == 123.456f);

		input  = "-123.456";
		auto c = try_to_number<f32>(input);
		CHECK(c.has_value() == true);
		CHECK(*c == -123.456f);

		input   = "456.654";
		auto c2 = try_to_number<f64>(input);
		CHECK(c2.has_value() == true);
		CHECK(*c2 == 456.654);

		input  = "+128";
		auto d = try_to_number<u8>(input);
		CHECK(d.has_value() == true);
		CHECK(*d == 128);

		input  = "#29A";
		auto e = try_to_number<i16>(input);
		CHECK(e.has_value() == true);
		CHECK(*e == 666);


		input  = "hello";
		auto f = try_to_number<i16>(input);
		CHECK(f.has_value() == false);
	}

	SECTION("to_number")
	{
		CHECK(123 == to_number("123"));
		CHECK(false == try_to_number("xyz").has_value());
	}

	SECTION("as")
	{
		//
		CHECK("ffff"sv == as<std::string>(0xFFFF, 16));
		CHECK(0xFFFF == as<u32>("0xFFFF", 16));
	}

	SECTION("prettytime")
	{

		CHECK("2min 8s 678ms"sv == pretty_time(std::chrono::duration<f64>(std::chrono::seconds{123} + 5678ms)));

		CHECK("1d 23min 16s 213ms 741us"sv == pretty_time(std::chrono::duration(std::chrono::days{1} + 23min + 16s + 213ms + 741us)));
	}

	SECTION("human_readable_bytes")
	{
		CHECK("0 bytes"sv == human_readable_bytes(0));
		CHECK("1 MiB"sv == human_readable_bytes(1_MiB));
		CHECK("1.02 MiB"sv == human_readable_bytes(1_MiB + 20_KiB));
		CHECK("3.14 GiB"sv == human_readable_bytes(3_GiB + 140_MiB + 100_KiB));
	}

	SECTION("pretty_bytes")
	{
		CHECK("128 bytes"sv == pretty_bytes(128));
		CHECK("1 KiB, 1 byte"sv == pretty_bytes(1024 + 1));


		CHECK("1 MiB, 256 KiB, 128 bytes"sv == pretty_bytes(1_MiB + 256_KiB + 128));
	}

	SECTION("delta encode/decode") 
	{ 
		std::vector<u8> data{0x01, 0x02, 0x02, 0x02, 0x03, 0x00, 0x01, 0x01};

		delta_encode(data);

		CHECK(data[0] == 0x01);
		CHECK(data[1] == 0x01);
		CHECK(data[2] == 0x00);
		CHECK(data[3] == 0x00);
		CHECK(data[4] == 0x01);
		CHECK(data[5] == 0xFD);
		CHECK(data[6] == 0x01);
		CHECK(data[7] == 0x00);

		delta_decode(data);

		CHECK(data[0] == 0x01);
		CHECK(data[1] == 0x02);
		CHECK(data[2] == 0x02);
		CHECK(data[3] == 0x02);
		CHECK(data[4] == 0x03);
		CHECK(data[5] == 0x00);
		CHECK(data[6] == 0x01);
		CHECK(data[7] == 0x01);
	}
}
