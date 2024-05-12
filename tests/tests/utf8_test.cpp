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
		auto        mixed_points = utf8::codepoints_to_vector(mixed_utf8_ascii);


		REQUIRE(mixed_points[0] == 0x1f30d);
		REQUIRE(mixed_points[1] == 0x31);
		REQUIRE(mixed_points[2] == 0x1f34b);
		REQUIRE(mixed_points[3] == 0xC4);

		// abc
		REQUIRE(utf8::codepoints_to_vector("abc").size() == 3);

		// 1 byte: A   0x41		 0x41
		// 2 byte: Ä   0xC4	     0xC3 0x84
		// 3 byte: ↥   0x21A5     0xE2 0x86 0xA5
		// 4 byte: 🌍  0x1F30D	 0xF0 0x9F 0x8C 0x8D
		std::string all_bytes{"\x41\xC3\x84\xE2\x86\xA5\xF0\x9F\x8C\x8D"};
		auto        aa_points = utf8::codepoints_to_vector(all_bytes);
		REQUIRE(aa_points.size() == 4);
		REQUIRE(aa_points[0] == 0x41);
		REQUIRE(aa_points[1] == 0xC4);
		REQUIRE(aa_points[2] == 0x21A5);
		REQUIRE(aa_points[3] == 0x1F30D);

		// u+FFFF
		REQUIRE(utf8::codepoints_to_vector("\xEF\xBF\xBF").size() == 1);

		// UTF8 BOM
		REQUIRE(utf8::codepoints_to_vector("\xEF\xBB\xBF").size() == 1);


		// UTF16 BOM
		REQUIRE(utf8::codepoints_to_vector("\uFEFF").size() == 1);
		REQUIRE(utf8::codepoints_to_vector("\uFFFE").size() == 1);
	}

	SECTION("invalid codepoints")
	{
		utf8::codepoints decoder;


		// C3 (single byte starting with a multi-byte prefix)
		decoder.reload("\xC3");
		auto test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);


		// E0 80 (incomplete sequence of trailing bytes)
		decoder.reload("\xE0\x80");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// FF (invalid byte value)
		decoder.reload("\xFF");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);


		// 1. Lone surrogate halves:
		// D8 00 (high surrogate half)
		decoder.reload("\xD8\x00");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// DC 00 (low surrogate half)
		decoder.reload("\xDC\x00");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 2. Overlong encodings:
		// C0 80 (overlong encoding for character 'A' - U+0041)
		decoder.reload("\xC0\x80");
		test = decoder.data();
		REQUIRE(test.size() == 2);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);


		// 0xF0 0x80 0x80 0x80
		decoder.reload("\xF0\x80\x80\x80");
		test = decoder.data();
		REQUIRE(test.size() == 3);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[2] == utf8::REPLACEMENT_CHARACTER);

		//  Start byte followed by non-continuation byte:
		decoder.reload("\xC2\xFF");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// F8 88 88 88 88 (sequence exceeding maximum length)
		decoder.reload("\xF8\x88\x88\x88\x88");
		test = decoder.data();
		REQUIRE(test.size() == 5);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[2] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[3] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[4] == utf8::REPLACEMENT_CHARACTER);

		// C2 FF (start byte for a 2-byte sequence followed by an invalid byte)
		decoder.reload("\xC2\xFF");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// 80 (isolated continuation byte)
		decoder.reload("\x80");
		test = decoder.data();
		REQUIRE(test.size() == 1);
		REQUIRE(test[0] == utf8::REPLACEMENT_CHARACTER);

		// A <invalid> A
		decoder.reload("\x41\x88\xC2\xFF\x41");
		test = decoder.data();
		REQUIRE(test.size() == 4);
		REQUIRE(test[0] == 0x41);
		REQUIRE(test[1] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[2] == utf8::REPLACEMENT_CHARACTER);
		REQUIRE(test[3] == 0x41);
	}
}
