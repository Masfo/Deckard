#include <catch2/catch_test_macros.hpp>


import std;
import deckard.utf8;
using namespace deckard;

TEST_CASE("utf8 decode to codepoints", "[utf8]")
{
	SECTION("valid codepoints")
	{
		//

		std::string mixed_utf8_ascii{"🌍1🍋Ä"};
		auto        codepoints = utf8::codepoints_from_utf8_string(mixed_utf8_ascii);

		REQUIRE(codepoints[0] == 0x1f30d);
		REQUIRE(codepoints[1] == 0x31);
		REQUIRE(codepoints[2] == 0x1f34b);
		REQUIRE(codepoints[3] == 0xC4);

		// abc
		REQUIRE(utf8::codepoints_from_utf8_string("abc").size() == 3);

		// 1 byte: A  0x41		 0x41
		// 2 byte: Ä  0xC4	     0xC3 0x84
		// 3 byte: ↥  0x21A5     0xE2 0x86 0xA5
		// 4 byte: 🌍 0x1F30D	 0xF0 0x9F 0x8C 0x8D
		std::string all_bytes{"\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D"};
		auto        aa_points = utf8::codepoints_from_utf8_string(all_bytes);
		REQUIRE(aa_points.size() == 4);
		REQUIRE(aa_points[0] == 0x41);
		REQUIRE(aa_points[1] == 0xC4);
		REQUIRE(aa_points[2] == 0x21A5);
		REQUIRE(aa_points[3] == 0x1F30D);
	}

	SECTION("invalid codepoints")
	{
		// u+FFFF
		REQUIRE(utf8::codepoints_from_utf8_string("\xEF\xBF\xBF").size() == 1);

		// UTF BOM
		REQUIRE(utf8::codepoints_from_utf8_string("\uFEFF").empty() == true);
		REQUIRE(utf8::codepoints_from_utf8_string("\uFFFE").empty() == true);


		// C3 (single byte starting with a multi-byte prefix)
		REQUIRE(utf8::codepoints_from_utf8_string("\xC3").empty() == true);

		// E0 80 (incomplete sequence of trailing bytes)
		REQUIRE(utf8::codepoints_from_utf8_string("\xE0\x80").empty() == true);

		// FF (invalid byte value)
		REQUIRE(utf8::codepoints_from_utf8_string("\xFF").empty() == true);


		// 1. Lone surrogate halves:
		// D8 00 (high surrogate half)
		REQUIRE(utf8::codepoints_from_utf8_string("\xD8\00").empty() == true);

		// DC 00 (low surrogate half)
		REQUIRE(utf8::codepoints_from_utf8_string("\xDC\x00").empty() == true);

		// 2. Overlong encodings:
		// C0 80 (overlong encoding for character 'A' - U+0041)
		REQUIRE(utf8::codepoints_from_utf8_string("\xC0\x80").empty() == true);

		// 0xF0 0x80 0x80 0x80
		REQUIRE(utf8::codepoints_from_utf8_string("\xF0\x80\x80\x80").empty() == true);

		//  Start byte followed by non-continuation byte:
		REQUIRE(utf8::codepoints_from_utf8_string("\xC2\xFF").empty() == true);

		// F8 88 88 88 88 (sequence exceeding maximum length)
		REQUIRE(utf8::codepoints_from_utf8_string("\xF8\x88\x88\x88\x88").empty() == true);

		// C2 FF (start byte for a 2-byte sequence followed by an invalid byte)
		REQUIRE(utf8::codepoints_from_utf8_string("\xC2\xFF").empty() == true);

		// 80 (isolated continuation byte)
		REQUIRE(utf8::codepoints_from_utf8_string("\x80").empty() == true);
	}
}
