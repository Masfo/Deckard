#include <catch2/catch_test_macros.hpp>


import std;
import deckard.utf8;
using namespace deckard;

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

		REQUIRE(utf8::codepoint_width(str.next()) == 1);
		REQUIRE(utf8::codepoint_width(str.next()) == 2);
		REQUIRE(utf8::codepoint_width(str.next()) == 3);
		REQUIRE(utf8::codepoint_width(str.next()) == 4);

		str = "🌍1🍋Ä";

		REQUIRE(utf8::codepoint_width(str.next()) == 4);
		REQUIRE(utf8::codepoint_width(str.next()) == 1);
		REQUIRE(utf8::codepoint_width(str.next()) == 4);
		REQUIRE(utf8::codepoint_width(str.next()) == 2);
	}

	SECTION("valid codepoints")
	{
		// abc
		utf8::string str  = "ABC";
		auto         test = str.codepoints();
		REQUIRE(test.size() == 3);
		REQUIRE(test[0] == 0x41);
		REQUIRE(test[1] == 0x42);
		REQUIRE(test[2] == 0x43);

		//
		str  = "🌍1🍋Ä";
		test = str.codepoints();
		REQUIRE(test.size() == 4);
		REQUIRE(test[0] == 0x1f30d);
		REQUIRE(test[1] == 0x31);
		REQUIRE(test[2] == 0x1f34b);
		REQUIRE(test[3] == 0xC4);

		//             UTF32		UTF8
		// 1 byte: A   0x41			0x41
		// 2 byte: Ä   0xC4			0xC3 0x84
		// 3 byte: ↥   0x21A8		0xE2 0x86 0xA8
		// 4 byte: 🌍  0x1F30D		0xF0 0x9F 0x8C 0x8D
		str  = "\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D";
		test = str.codepoints();
		REQUIRE(test.size() == 4);
		REQUIRE(test[0] == 0x41);
		REQUIRE(test[1] == 0xC4);
		REQUIRE(test[2] == 0x21A5);
		REQUIRE(test[3] == 0x1F30D);
	}

	SECTION("valid codepoints")
	{
		// u+FFFF
		utf8::string str("\xEF\xBF\xBF");
		auto         test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == 0xFFFF);

		// UTF8 BOM
		str  = "\xEF\xBB\xBF";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == 0xFEFF);

		// UTF16 BOM
		str  = "\uFFFE";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == 0xFFFE);

		// Replacement character 0xFFFD (0xEF 0xBF 0xBD)
		str  = "\xEF\xBF\xDB";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
	}

	SECTION("invalid codepoints")
	{
		utf8::string str("\xC3");

		// C3 (single byte starting with a multi-byte prefix)
		auto test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// E0 80 (incomplete sequence of trailing bytes)
		str  = "\xE0\x80";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// FF (invalid byte value)
		str  = "\xFF";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 1. Lone surrogate halves:
		// D8 00 (high surrogate half)
		str  = "\xD8\x00";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// DC 00 (low surrogate half)
		str  = "\xDC\x00";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 2. Overlong encodings:
		// C0 80 (overlong encoding for character 'A' - U+0041)
		str  = "\xC0\x80";
		test = str.codepoints();
		REQUIRE(test.size() == 2);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);

		// 0xF0 0x80 0x80 0x80
		str  = "\xF0\x80\x80\x80";
		test = str.codepoints();
		REQUIRE(test.size() == 3);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[2] == utf8::REPLACEMENT_CHARACTER);

		//  Start byte followed by non-continuation byte:
		str  = "\xC2\xFF";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// F8 88 88 88 88 (sequence exceeding maximum length)
		str  = "\xF8\x88\x88\x88\x88";
		test = str.codepoints();
		REQUIRE(test.size() == 5);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[2] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[3] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[4] == utf8::REPLACEMENT_CHARACTER);

		// C2 FF (start byte for a 2-byte sequence followed by an invalid byte)
		str  = "\xC2\xFF";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 80 (isolated continuation byte)
		str  = "\x80";
		test = str.codepoints();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// A <invalid> A
		str  = "\x41\x88\xC2\xFF\x41";
		test = str.codepoints();
		REQUIRE(test.size() == 4);
		REQUIRE(test[0] == 0x41);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[2] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[3] == 0x41);
	}
}
