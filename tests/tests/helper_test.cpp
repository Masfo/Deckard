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
using namespace std::string_literals;
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

	SECTION("read_le/read_be")
	{
		std::array<u8, 8> buf{0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};

		CHECK(read_le<u8>(buf) == 0x77_u8);
		CHECK(read_le<u16>(buf) == 0x6677_u16);
		CHECK(read_le<u32>(buf) == 0x4455'6677_u32);
		CHECK(read_le<u64>(buf) == 0x0011'2233'4455'6677_u64);

		CHECK(read_be<u8>(buf) == 0x77_u8);
		CHECK(read_be<u16>(buf) == 0x7766_u16);
		CHECK(read_be<u32>(buf) == 0x7766'5544_u32);
		CHECK(read_be<u64>(buf) == 0x7766'5544'3322'1100_u64);

		// Signed
		{
			std::array<u8, 1> neg{0xFE_u8};
			CHECK(read_le<i8>(neg) == static_cast<i8>(-2)); // 0xFE
			CHECK(read_be<i8>(neg) == static_cast<i8>(-2)); // 0xFE
		}
		{
			std::array<u8, 2> neg{0xFE_u8, 0xFF_u8};
			CHECK(read_le<i16>(neg) == static_cast<i16>(-2));
			CHECK(read_be<i16>(neg) == static_cast<i16>(0xFEFF)); // -257
		}
		{
			std::array<u8, 2> neg_be{0xFF_u8, 0xFE_u8};
			CHECK(read_be<i16>(neg_be) == static_cast<i16>(-2));
			CHECK(read_le<i16>(neg_be) == static_cast<i16>(0xFEFF)); // -257
		}
		{
			std::array<u8, 4> neg{0xFE_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8};
			CHECK(read_le<i32>(neg) == static_cast<i32>(-2));
			CHECK(read_be<i32>(neg) == static_cast<i32>(0xFEFF'FFFF)); // -16777217
		}
		{
			std::array<u8, 4> neg_be{0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFE_u8};
			CHECK(read_be<i32>(neg_be) == static_cast<i32>(-2));
			CHECK(read_le<i32>(neg_be) == static_cast<i32>(0xFEFF'FFFF)); // -16777217
		}
		{
			std::array<u8, 8> neg{0xFE_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8};
			CHECK(read_le<i64>(neg) == static_cast<i64>(-2));
			CHECK(read_be<i64>(neg) == static_cast<i64>(0xFEFF'FFFF'FFFF'FFFF)); // -72057594037927937
		}
		{
			std::array<u8, 8> neg_be{0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFF_u8, 0xFE_u8};
			CHECK(read_be<i64>(neg_be) == static_cast<i64>(-2));
			CHECK(read_le<i64>(neg_be) == static_cast<i64>(0xFEFF'FFFF'FFFF'FFFF)); // -72057594037927937
		}
	}

	SECTION("read_le/read_be offset")
	{
		std::array<u8, 10> buf{0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
		CHECK(read_le<u8>(buf, 1) == 0x88_u8);
		CHECK(read_le<u16>(buf, 2) == 0x6677_u16);
		CHECK(read_le<u32>(buf, 3) == 0x3344'5566_u32);
		CHECK(read_le<u64>(buf, 2) == 0x0011'2233'4455'6677_u64);

		CHECK(read_be<u8>(buf, 1) == 0x88_u8);
		CHECK(read_be<u16>(buf, 2) == 0x7766_u16);
		CHECK(read_be<u32>(buf, 3) == 0x6655'4433_u32);
		CHECK(read_be<u64>(buf, 2) == 0x7766'5544'3322'1100_u64);

		// signed with offset
		std::array<u8, 10> s{0xAA_u8, 0xFE_u8, 0xBB_u8, 0xCC_u8, 0xDD_u8, 0xEE_u8, 0xFF_u8, 0x11_u8, 0x22_u8, 0x33_u8};
		CHECK(read_le<i8>(s, 1) == static_cast<i8>(0xFE));                    // -2
		CHECK(read_le<i16>(s, 1) == static_cast<i16>(0xBBFE));                // -17410
		CHECK(read_le<i32>(s, 1) == static_cast<i32>(0xDDCC'BBFE));           // -573785090
		CHECK(read_le<i64>(s, 1) == static_cast<i64>(0x2211'FFEE'DDCC'BBFE)); // 2450205651927358462

		CHECK(read_be<i8>(s, 1) == static_cast<i8>(0xFE));                    // -2
		CHECK(read_be<i16>(s, 1) == static_cast<i16>(0xFEBB));                // -325
		CHECK(read_be<i32>(s, 1) == static_cast<i32>(0xFEBB'CCDD));           // -21246947
		CHECK(read_be<i64>(s, 1) == static_cast<i64>(0xFEBB'CCDD'EEFF'1122)); // -36242258794888862
	}

	SECTION("write_le/write_be")
	{
		std::array<u8, 32> buf{};
		std::fill(buf.begin(), buf.end(), 0_u8);

		write_le<u8>(buf, 0, 0x12_u8);
		CHECK(buf[0] == 0x12_u8);

		write_be<u8>(buf, 1, 0x34_u8);
		CHECK(buf[1] == 0x34_u8);

		write_le<u16>(buf, 0, 0x1234_u16);
		CHECK(buf[0] == 0x34_u8);
		CHECK(buf[1] == 0x12_u8);

		write_be<u16>(buf, 2, 0x1234_u16);
		CHECK(buf[2] == 0x12_u8);
		CHECK(buf[3] == 0x34_u8);

		write_le<u32>(buf, 4, 0x1122'3344_u32);
		CHECK(buf[4] == 0x44_u8);
		CHECK(buf[5] == 0x33_u8);
		CHECK(buf[6] == 0x22_u8);
		CHECK(buf[7] == 0x11_u8);

		write_be<u32>(buf, 8, 0x1122'3344_u32);
		CHECK(buf[8] == 0x11_u8);
		CHECK(buf[9] == 0x22_u8);
		CHECK(buf[10] == 0x33_u8);
		CHECK(buf[11] == 0x44_u8);

		write_le<u64>(buf, 16, 0x1122'3344'5566'7788_u64);
		CHECK(buf[16] == 0x88_u8);
		CHECK(buf[17] == 0x77_u8);
		CHECK(buf[18] == 0x66_u8);
		CHECK(buf[19] == 0x55_u8);
		CHECK(buf[20] == 0x44_u8);
		CHECK(buf[21] == 0x33_u8);
		CHECK(buf[22] == 0x22_u8);
		CHECK(buf[23] == 0x11_u8);

		write_be<u64>(buf, 24, 0x1122'3344'5566'7788_u64);
		CHECK(buf[24] == 0x11_u8);
		CHECK(buf[25] == 0x22_u8);
		CHECK(buf[26] == 0x33_u8);
		CHECK(buf[27] == 0x44_u8);
		CHECK(buf[28] == 0x55_u8);
		CHECK(buf[29] == 0x66_u8);
		CHECK(buf[30] == 0x77_u8);
		CHECK(buf[31] == 0x88_u8);

		write_le<i16>(buf, 12, static_cast<i16>(-2));
		CHECK(buf[12] == 0xFE_u8);
		CHECK(buf[13] == 0xFF_u8);

		write_be<i16>(buf, 14, static_cast<i16>(-2));
		CHECK(buf[14] == 0xFF_u8);
		CHECK(buf[15] == 0xFE_u8);
	}

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


		input    = "rrgwru, gwubwg, wgbbw, bwb, ugrgw, uwgguuw, wuggb, bgwb, ubrr, grruuub, rwbwgbub, wwwggg, uuw";
		splitted = split(input, ", ");
		CHECK(splitted.size() == 13);
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


		std::string csv("1,5,9,7,5,6,7,8,9");
		auto        pcsv = ints(csv);
		CHECK(pcsv.size() == 9);
		CHECK(pcsv[0] == 1);
		CHECK(pcsv[1] == 5);
		CHECK(pcsv[2] == 9);
		CHECK(pcsv[3] == 7);
		CHECK(pcsv[4] == 5);
		CHECK(pcsv[5] == 6);
		CHECK(pcsv[6] == 7);
		CHECK(pcsv[7] == 8);
		CHECK(pcsv[8] == 9);
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

		CHECK(take(input, 10).size() == 8);
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

	SECTION("head")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30, 40, 50, 60, 70, 80};

		auto first = head(input, 10);
		CHECK(first.size() == real.size());
		CHECK(first == real);

		CHECK(head(input, 0).empty());
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

		auto first = tail(input, 5);
		CHECK(first.size() == real.size());
		CHECK(first == real);

		CHECK(tail(input, 0).empty());
	}


	SECTION("tail")
	{
		std::vector<u32> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::vector<u32> real{10, 20, 30, 40, 50, 60, 70, 80};

		auto first = tail(input, 10);
		CHECK(first.size() == real.size());
		CHECK(first == real);
	}

	SECTION("tail-array")
	{
		std::array<u32, 8> input{10, 20, 30, 40, 50, 60, 70, 80};
		std::array<u32, 5> real{40, 50, 60, 70, 80};

		auto first = tail<5>(input);
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

		input = include_only(input, string::option::d);
		CHECK(input == "123");
	}

	SECTION("slice")
	{
		std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
		std::span<int>   sp(v);


		auto s1 = slice(sp, 2, 5);   // {2, 3, 4}
		auto s2 = slice(sp, 3);      // {3, 4, 5, 6, 7, 8, 9}
		auto s3 = slice(sp, -3, -1); // {7, 8}
		auto s4 = slice(sp, 0, -2);  // {0, 1, 2, 3, 4, 5, 6, 7}
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

	SECTION("nth_digit")
	{
		CHECK(5 == nth_digit(12345, 1));
		CHECK(4 == nth_digit(12345, 2));
		CHECK(3 == nth_digit(12345, 3));
		CHECK(2 == nth_digit(12345, 4));
		CHECK(1 == nth_digit(12345, 5));
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


	SECTION("permutations/dynamic")
	{
		{
			const auto dyna = permutations("ABC"sv, 2);

			CHECK(dyna.size() == 6);
			CHECK(dyna[0] == "AB"s);
			CHECK(dyna[1] == "AC"s);
			CHECK(dyna[2] == "BA"s);
			CHECK(dyna[3] == "BC"s);
			CHECK(dyna[4] == "CA"s);
			CHECK(dyna[5] == "CB"s);
		}
		{
			const auto dyna = permutations("ABCD"sv, 3);
			CHECK(dyna.size() == 24);
		}

		{
			const auto dyna = permutations("ABCDA"sv, 3);
			CHECK(dyna.size() == 60);
		}

		{
			const std::array<std::string, 3> names{"Alice", "Bob", "Eve"};
			const auto                       dyna = permutations(names, 2);
			CHECK(dyna.size() == 6);
			CHECK(dyna[0] == make_vector("Alice"s, "Bob"s));
			CHECK(dyna[1] == make_vector("Alice"s, "Eve"s));
			CHECK(dyna[2] == make_vector("Bob"s, "Alice"s));
			CHECK(dyna[3] == make_vector("Bob"s, "Eve"s));
			CHECK(dyna[4] == make_vector("Eve"s, "Alice"s));
			CHECK(dyna[5] == make_vector("Eve"s, "Bob"s));
		}
	}


	SECTION("unique_permutations/dynamic")
	{
		{
			const auto dyna = unique_permutations("AAB"sv, 3);

			CHECK(dyna.size() == 3);
			CHECK(dyna[0] == "AAB"s);
			CHECK(dyna[1] == "ABA"s);
			CHECK(dyna[2] == "BAA"s);
		}

		{
			const auto dyna = unique_permutations("ABCDA"sv, 3);
			CHECK(dyna.size() == 33);
		}

		{
			const std::array<std::string, 3> names{"A", "A", "B"};
			const auto                       dyna = unique_permutations(names, 3);
			CHECK(dyna.size() == 3);
			CHECK(dyna[0] == make_vector("A"s, "A"s, "B"s));
			CHECK(dyna[1] == make_vector("A"s, "B"s, "A"s));
			CHECK(dyna[2] == make_vector("B"s, "A"s, "A"s));
		}

		{
			const std::array<std::string, 3> names{"Alice", "Bob", "Eve"};
			const auto                       dyna = unique_permutations(names, 3);
			CHECK(dyna.size() == 6);
			CHECK(dyna[0] == make_vector("Alice"s, "Bob"s, "Eve"s));
			CHECK(dyna[1] == make_vector("Alice"s, "Eve"s, "Bob"s));
			CHECK(dyna[2] == make_vector("Bob"s, "Alice"s, "Eve"s));
			CHECK(dyna[3] == make_vector("Bob"s, "Eve"s, "Alice"s));
			CHECK(dyna[4] == make_vector("Eve"s, "Alice"s, "Bob"s));
			CHECK(dyna[5] == make_vector("Eve"s, "Bob"s, "Alice"s));
		}
	}

	SECTION("combinations/dynamic")
	{
		{
			const auto dyna = combinations("ABCD"sv, 2);
			CHECK(dyna.size() == 6);
		}
		{
			const std::array<std::string, 3> names{"Alice", "Bob", "Eve"};
			const auto                       dyna = combinations(names, 3);
			CHECK(dyna.size() == 1);
		}

		{
			const std::array<std::string, 4> names{"Alice", "Bob", "Eve", "David"};
			const auto                       dyna = combinations(names, 3);

			CHECK(dyna.size() == 4);
			CHECK(dyna[0] == make_vector("Alice"s, "Bob"s, "David"s));
			CHECK(dyna[1] == make_vector("Alice"s, "Bob"s, "Eve"s));
			CHECK(dyna[2] == make_vector("Alice"s, "David"s, "Eve"s));
			CHECK(dyna[3] == make_vector("Bob"s, "David"s, "Eve"s));
		}
	}

	SECTION("5 elements on combinations/permutations/unique_permutations")
	{
		const std::array<std::string, 5> names{"Alice", "Bob", "Eve", "David", "Carl"};

		u32 count = 1;
		CHECK(permutations(names, count).size() == 5);
		CHECK(unique_permutations(names, count).size() == 5);
		CHECK(combinations(names, count).size() == 5);

		count = 2;
		CHECK(permutations(names, count).size() == 20);
		CHECK(unique_permutations(names, count).size() == 20);
		CHECK(combinations(names, count).size() == 10);

		count = 3;
		CHECK(permutations(names, count).size() == 60);
		CHECK(unique_permutations(names, count).size() == 60);
		CHECK(combinations(names, count).size() == 10);

		count = 4;
		CHECK(permutations(names, count).size() == 120);
		CHECK(unique_permutations(names, count).size() == 120);
		CHECK(combinations(names, count).size() == 5);

		count = 5;
		CHECK(permutations(names, count).size() == 120);
		CHECK(unique_permutations(names, count).size() == 120);
		CHECK(combinations(names, count).size() == 1);
	}


	SECTION("4 elements on combinations/permutations/unique_permutations")
	{
		const std::array<std::string, 4> names{"Alice", "Bob", "Alice", "Eve"};

		u32 count = 1;
		CHECK(permutations(names, count).size() == 4);
		CHECK(unique_permutations(names, count).size() == 3);
		CHECK(combinations(names, count).size() == 4);

		count = 2;
		CHECK(permutations(names, count).size() == 12);
		CHECK(unique_permutations(names, count).size() == 7);
		CHECK(combinations(names, count).size() == 6);

		count = 3;
		CHECK(permutations(names, count).size() == 24);
		CHECK(unique_permutations(names, count).size() == 12);
		CHECK(combinations(names, count).size() == 4);

		count = 4;
		CHECK(permutations(names, count).size() == 24);
		CHECK(unique_permutations(names, count).size() == 12);
		CHECK(combinations(names, count).size() == 1);

		count = 5;
		CHECK(permutations(names, count).size() == 0);
		CHECK(unique_permutations(names, count).size() == 0);
		CHECK(combinations(names, count).size() == 0);
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

	SECTION("variadic math helpers")
	{
		i32 a = 0, b = -5, c = 10, d = 2;

		CHECK(-5 == vmin(a, b, c, d));
		CHECK(10 == vmax(a, b, c, d));
		CHECK(7 == sum(a, b, c, d));
		CHECK(-17 == subtract(b, c, d));
		CHECK(-100 == product(b, c, d));
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

	SECTION("unique")
	{
		{
			std::array<u32, 9> data{1, 2, 2, 3, 3, 3, 4, 5, 5};
			auto               res = unique(data);
			CHECK(res.size() == 5);
			CHECK(res[0] == 1);
			CHECK(res[1] == 2);
			CHECK(res[2] == 3);
			CHECK(res[3] == 4);
			CHECK(res[4] == 5);
		}

		{
			std::array<u32, 9> data{5, 5, 4, 3, 3, 3, 2, 2, 1};
			auto               res = unique(data);
			CHECK(res.size() == 5);
			CHECK(res[0] == 5);
			CHECK(res[1] == 4);
			CHECK(res[2] == 3);
			CHECK(res[3] == 2);
			CHECK(res[4] == 1);
		}

		{
			std::vector<u32> data{1, 2, 2, 3, 3, 3, 4, 5, 5};
			auto             res = unique(data);
			CHECK(res.size() == 5);
			CHECK(res[0] == 1);
			CHECK(res[1] == 2);
			CHECK(res[2] == 3);
			CHECK(res[3] == 4);
			CHECK(res[4] == 5);
		}


		{
			std::vector<std::string> data{"hello", "world", "hello", "dog", "cat", "dog"};
			auto                     res = unique(data);
			CHECK(res.size() == 4);
			CHECK(res[0] == "hello");
			CHECK(res[1] == "world");
			CHECK(res[2] == "dog");
			CHECK(res[3] == "cat");
		}

		{
			std::string input("122333455");
			auto        res = unique(input);
			CHECK(res == "12345"s);
		}

		{
			std::string input("554333221");
			auto        res = unique(input);
			CHECK(res == "54321"s);
		}
		{
			std::string input("554333221");
			auto        res = unique<i32>(input);
			CHECK(res == 5);
		}
	}

	SECTION("match")
	{
		std::string input("turn left and rotate 180");
		std::string expr("tur? left and rot?te *");

		CHECK(match(expr, input) == true);
	}

	SECTION("simple-pattern-match")
	{
		{
			std::string input("turn off 123,456 through 789,012 XXX");
			std::string expr("turn off ?,? through ?,?");

			auto result = string::simple_pattern_match(expr, input);

			CHECK(result.size() == 4);
			CHECK(result[0] == "123");
			CHECK(result[1] == "456");
			CHECK(result[2] == "789");
			CHECK(result[3] == "012");
		}

		{
			std::string input("turn off 123,456 through 789,012");
			std::string expr("turn off ?,? through ?,?");

			auto result = string::simple_pattern_match(expr, input);

			CHECK(result.size() == 4);
			CHECK(result[0] == "123");
			CHECK(result[1] == "456");
			CHECK(result[2] == "789");
			CHECK(result[3] == "012");
		}

		{
			std::string input("turn off 123 with String XXX");
			std::string expr("turn off ? with ?");

			auto [what, with] = string::simple_pattern_match<i32, std::string>(expr, input);

			CHECK(what == 123);
			CHECK(with == "String"s);
		}

		{
			std::string input("turn off 123 with String");
			std::string expr("turn off ? with ?");

			auto [what, with] = string::simple_pattern_match<i32, std::string>(expr, input);

			CHECK(what == 123);
			CHECK(with == "String"s);
		}
	}
}
