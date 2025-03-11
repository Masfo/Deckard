#include <catch2/catch_test_macros.hpp>


import std;
import deckard.utf8;
import deckard.as;
using namespace deckard;
using namespace std::string_view_literals;

TEST_CASE("utf8::string", "[utf8]")
{
	SECTION("ascii c-tor")
	{
		utf8::string2 str("hello world");
		CHECK(str.size() == 11);
		CHECK(str.length() == 11);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 11);

		CHECK(std::string(str.c_str()) == "hello world"sv);
	}

	SECTION("unicode c-tor")
	{
		utf8::string2 str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.is_valid() == true);
		CHECK(std::string(str.c_str()) == "hello 🌍"sv);
	}

	SECTION("copy assignment")
	{

		utf8::string2 a("hello 🌍");
		auto          b = a;

		CHECK(a.size() == 7);
		CHECK(a.length() == 7);
		CHECK(a.empty() == false);
		CHECK(a.size_in_bytes() == 10);
		CHECK(a.is_valid() == true);
		CHECK(std::string(a.c_str()) == "hello 🌍"sv);

		CHECK(b.size() == 7);
		CHECK(b.length() == 7);
		CHECK(b.empty() == false);
		CHECK(b.size_in_bytes() == 10);
		CHECK(b.is_valid() == true);
		CHECK(std::string(b.c_str()) == "hello 🌍"sv);

		CHECK(a.data() != b.data());
	}

	SECTION("insert/append") 
	{
		utf8::string2 a("hello ");


		CHECK(a.size() == 6);
		CHECK(a.front() == 'h');
		CHECK(a.back() == ' ');


		utf8::string2 b("🌍");
		a.insert(a.end(), b);


		CHECK(a.size() == 7);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 0x1f30d);

		a.append('b');
		CHECK(a.size() == 8);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 'b');

	}
}

TEST_CASE("utf8 v2", "[utf8]")
{

	SECTION("width from leading byte")
	{

		CHECK(utf8::codepoint_width(as<u8>(0x41)) == 1);
		CHECK(utf8::codepoint_width(as<u8>(0xC3)) == 2);
		CHECK(utf8::codepoint_width(as<u8>(0xE2)) == 3);
		CHECK(utf8::codepoint_width(as<u8>(0xF0)) == 4);

		CHECK(utf8::codepoint_width(as<u8>(0x84)) == 0);
		CHECK(utf8::codepoint_width(as<u8>(0x9F)) == 0);
	}
}

TEST_CASE("utf8 decode to codepoints", "[utf8]")
{
	/*
	$examples = array(
	'Valid ASCII' => "a",
	'Valid 2 Octet Sequence' => "\xc3\xb1",
	'Invalid 2 Octet Sequence' => "\xc3\x28",
	'Invalid Sequence Identifier' => "\xa0\xa1",
	'Valid 3 Octet Sequence' => "\xe2\x82\xa1",
	'Invalid 3 Octet Sequence (in 2nd Octet)' => "\xe2\x28\xa1",
	'Invalid 3 Octet Sequence (in 3rd Octet)' => "\xe2\x82\x28",

	'Valid 4 Octet Sequence' => "\xf0\x90\x8c\xbc",
	'Invalid 4 Octet Sequence (in 2nd Octet)' => "\xf0\x28\x8c\xbc",
	'Invalid 4 Octet Sequence (in 3rd Octet)' => "\xf0\x90\x28\xbc",
	'Invalid 4 Octet Sequence (in 4th Octet)' => "\xf0\x28\x8c\x28",
	'Valid 5 Octet Sequence (but not Unicode!)' => "\xf8\xa1\xa1\xa1\xa1",
	'Valid 6 Octet Sequence (but not Unicode!)' => "\xfc\xa1\xa1\xa1\xa1\xa1",
);
	*/


	SECTION("widths")
	{
		utf8::string str("\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D");

		CHECK(utf8::codepoint_width(str.next()) == 1);
		CHECK(utf8::codepoint_width(str.next()) == 2);
		CHECK(utf8::codepoint_width(str.next()) == 3);
		CHECK(utf8::codepoint_width(str.next()) == 4);

		str = "🌍1🍋Ä";

		CHECK(utf8::codepoint_width(str.next()) == 4);
		CHECK(utf8::codepoint_width(str.next()) == 1);
		CHECK(utf8::codepoint_width(str.next()) == 4);
		CHECK(utf8::codepoint_width(str.next()) == 2);
	}

	SECTION("valid codepoints")
	{
		// abc
		utf8::string str  = "ABC";
		auto         test = str.codepoints();
		CHECK(test.size() == 3);
		CHECK(test[0] == 0x41);
		CHECK(test[1] == 0x42);
		CHECK(test[2] == 0x43);

		//
		str  = "🌍1🍋Ä";
		test = str.codepoints();
		CHECK(test.size() == 4);
		CHECK(test[0] == 0x1'f30d);
		CHECK(test[1] == 0x31);
		CHECK(test[2] == 0x1'f34b);
		CHECK(test[3] == 0xC4);

		//             UTF32		UTF8
		// 1 byte: A   0x41			0x41
		// 2 byte: Ä   0xC4			0xC3 0x84
		// 3 byte: ↥   0x21A8		0xE2 0x86 0xA8
		// 4 byte: 🌍  0x1F30D		0xF0 0x9F 0x8C 0x8D
		str  = "\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D";
		test = str.codepoints();
		CHECK(test.size() == 4);
		CHECK(test[0] == 0x41);
		CHECK(test[1] == 0xC4);
		CHECK(test[2] == 0x21A5);
		CHECK(test[3] == 0x1'F30D);
	}

	SECTION("valid codepoints")
	{
		// u+FFFF
		utf8::string str("\xEF\xBF\xBF");
		auto         test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == 0xFFFF);

		// UTF8 BOM
		str  = "\xEF\xBB\xBF";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == 0xFEFF);

		// UTF16 BOM
		str  = "\uFFFE";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == 0xFFFE);

		// Replacement character 0xFFFD (0xEF 0xBF 0xBD)
		str  = "\xEF\xBF\xDB";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);
	}

	SECTION("invalid codepoints")
	{
		utf8::string str("\xC3");

		// C3 (single byte starting with a multi-byte prefix)
		auto test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// E0 80 (incomplete sequence of trailing bytes)
		str  = "\xE0\x80";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// FF (invalid byte value)
		str  = "\xFF";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 1. Lone surrogate halves:
		// D8 00 (high surrogate half)
		str  = "\xD8\x00";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// DC 00 (low surrogate half)
		str  = "\xDC\x00";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 2. Overlong encodings:
		// C0 80 (overlong encoding for character 'A' - U+0041)
		str  = "\xC0\x80";
		test = str.codepoints();
		CHECK(test.size() == 2);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[1] == utf8::REPLACEMENT_CHARACTER);

		// 0xF0 0x80 0x80 0x80
		str  = "\xF0\x80\x80\x80";
		test = str.codepoints();
		CHECK(test.size() == 3);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[1] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[2] == utf8::REPLACEMENT_CHARACTER);

		//  Start byte followed by non-continuation byte:
		str  = "\xC2\xFF";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// F8 88 88 88 88 (sequence exceeding maximum length)
		str  = "\xF8\x88\x88\x88\x88";
		test = str.codepoints();
		CHECK(test.size() == 5);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[1] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[2] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[3] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[4] == utf8::REPLACEMENT_CHARACTER);

		// C2 FF (start byte for a 2-byte sequence followed by an invalid byte)
		str  = "\xC2\xFF";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 80 (isolated continuation byte)
		str  = "\x80";
		test = str.codepoints();
		CHECK(test.size() == 1);
		CHECK(test[0] == utf8::REPLACEMENT_CHARACTER);

		// A <invalid> A
		str  = "\x41\x88\xC2\xFF\x41";
		test = str.codepoints();
		CHECK(test.size() == 4);
		CHECK(test[0] == 0x41);
		CHECK(test[1] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[2] == utf8::REPLACEMENT_CHARACTER);
		CHECK(test[3] == 0x41);
	}
}
