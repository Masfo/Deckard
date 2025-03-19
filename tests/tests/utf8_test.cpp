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


		str = "\xF0\x9F\xA4\xA6\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F";
		CHECK(str.length() == 5);
		CHECK(str.size_in_bytes() == 17);
		CHECK(str.count() == 2); // TODO: should be 1

		str = "नमस्ते";
		CHECK(str.length() == 6);
		CHECK(str.size_in_bytes() == 18);
		CHECK(str.count() == 4);
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

	SECTION("starts_with")
	{
		//
		utf8::string a("AÄ↥🌍");

		CHECK(a.starts_with("AÄ") == true);
		CHECK(a.starts_with("AÄ"sv) == true);


		utf8::string b("AÄ");
		CHECK(a.starts_with(b) == true);

		b = "QW";
		CHECK(a.starts_with(b) == false);
	}

	SECTION("ends_with")
	{
		utf8::string a("AÄ↥🌍");

		CHECK(a.ends_with("↥🌍") == true);
		CHECK(a.ends_with("↥🌍"sv) == true);


		utf8::string b("↥🌍");
		CHECK(a.ends_with(b) == true);

		b = "QW";
		CHECK(a.ends_with(b) == false);

		a = "ab🌍cd";

		CHECK(a.ends_with("cd") == true);
		CHECK(a.ends_with("ab") == false);
		CHECK(a.ends_with("b🌍c") == false);
	}


	SECTION("contains")
	{
		utf8::string a("hello 🌍AÄ world ↥🌍");

		CHECK(a.contains("🌍") == true);
		CHECK(a.contains("🌍"sv) == true);
		CHECK(a.contains("world"sv) == true);
		CHECK(a.contains("xyz"sv) == false);
	}

	SECTION("find")
	{
		utf8::string a("hello 🌍AÄ world ↥🌍");

		CHECK(a.find("🌍") == 6);

		utf8::string b("world");
		CHECK(a.find(b) == 10);

		CHECK(a.find("xyz"sv) == -1);

		CHECK(a.find("🌍", 10) == 17);
	}

SECTION("erase")
	{

		// Basic ASCII erase
		utf8::string str("hello world");
		str.erase(5, 1); // erase space
		CHECK(str == "helloworld");
		CHECK(str.size() == 10);
		
		// Erase from start
		str = "hello world";
		str.erase(0, 5);
		CHECK(str == " world");
		CHECK(str.size() == 6);
		
		// Erase from end
		str = "hello world";
		str.erase(6, 5);
		
		CHECK(str == "hello"sv);
		CHECK(str.size() ==5);

		// Erase UTF-8 characters
		str = "hello 🌍 world";
		str.erase(6, 1); // erase emoji
		CHECK(str == "hello  world");
		CHECK(str.size() == 12);

		#if 0
		// Multiple UTF-8 characters
		str = "AÄ↥🌍";
		str.erase(1, 2); // erase Ä↥
		CHECK(str == "A🌍");
		CHECK(str.size() == 2);

		// Iterator range erase
		str      = "hello world";
		auto it1 = str.begin() + 5;
		auto it2 = str.begin() + 6;
		str.erase(it1, it2);
		CHECK(str == "helloworld");
		CHECK(str.size() == 10);

		// Erase single position
		str = "hello🌍world";
		str.erase(str.begin() + 5); // erase emoji
		CHECK(str == "helloworld");
		CHECK(str.size() == 10);

		// Boundary tests
		str = "🌍test🌍";
		str.erase(0, 1); // erase first emoji
		CHECK(str == "test🌍");
		CHECK(str.size() == 5);

		str.erase(4, 1); // erase last emoji
		CHECK(str == "test");
		CHECK(str.size() == 4);

		// Empty after full erase
		str = "test";
		str.erase(0, str.size());
		CHECK(str.empty());
		CHECK(str.size() == 0);
		#endif
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
