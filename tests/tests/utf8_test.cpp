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
		utf8::string str("hello world");
		CHECK(str.size() == 11);
		CHECK(str.length() == 11);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 11);

		CHECK(std::string(str.c_str()) == "hello world"sv);
	}


	SECTION("unicode c-tor")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.is_valid() == true);
		CHECK(std::string(str.c_str()) == "hello 🌍"sv);
	}

	SECTION("copy assignment")
	{

		utf8::string a("hello 🌍");
		auto         b = a;

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

	SECTION("assign c-string")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.is_valid() == true);
		CHECK(std::string(str.c_str()) == "hello 🌍"sv);
		//


		str = "hello";
		CHECK(str.size() == 5);
		CHECK(str.length() == 5);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 5);
		CHECK(str.is_valid() == true);
		CHECK(std::string(str.c_str()) == "hello"sv);
		//
	}

	SECTION("insert/append/+/+=")
	{
		utf8::string a("hello ");


		CHECK(a.size() == 6);
		CHECK(a.front() == 'h');
		CHECK(a.back() == ' ');


		utf8::string b("🌍");
		a.insert(a.end(), b);


		CHECK(a.size() == 7);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 0x1'f30d);

		a.append('b');
		CHECK(a.size() == 8);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 'b');

		char32 w = 0x1'f30d;
		a.append(w);
		CHECK(a.size() == 9);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 0x1'f30d);

		auto c = a + b;
		CHECK(c.size() == 10);
		CHECK(c.front() == 'h');
		CHECK(c.back() == 0x1'f30d);
		CHECK(c == "hello 🌍b🌍🌍");

		c += "♥";
		CHECK(c.size() == 11);
		CHECK(c.front() == 'h');
		CHECK(c.back() == 0x2665);
		CHECK(c == "hello 🌍b🌍🌍♥");


		b = "♥";
		c = b + "♦";
		CHECK(c.size() == 2);
		CHECK(c.front() == 0x2665);
		CHECK(c.back() == 0x2666);
		CHECK(c == "♥♦");

		c += "♧";
		CHECK(c.size() == 3);
		CHECK(c.front() == 0x2665);
		CHECK(c.back() == 0x2667);
		CHECK(c == "♥♦♧");
	}

	SECTION("substr")
	{
		utf8::string a("hello world");

		CHECK(a.size() == 11);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 'd');

		auto sub = a.substr(6, 10);

		CHECK(sub.size() == 5);
		CHECK(sub.front() == 'w');
		CHECK(sub.back() == 'd');


		utf8::string b("hello 🌍");
		auto         c = b.substr(6, 7);
		CHECK(c.size() == 1);
		CHECK(c.front() == 0x1'f30d);
		CHECK(c.back() == 0x1'f30d);
	}

	SECTION("widths")
	{
		//             UTF32		UTF8
		// 1 byte: A   0x41			0x41
		// 2 byte: Ä   0xC4			0xC3 0x84
		// 3 byte: ↥   0x21A5		0xE2 0x86 0xA5
		// 4 byte: 🌍  0x1F30D		0xF0 0x9F 0x8C 0x8D

		utf8::string a("AÄ↥🌍");

		std::array<u32, 4> correct{0x41, 0xC4, 0x21A5, 0x1'F30D};

		auto it = a.begin();

		CHECK(*it++ == correct[0]);
		CHECK(*it++ == correct[1]);
		CHECK(*it++ == correct[2]);
		CHECK(*it++ == correct[3]);
	}
}

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
