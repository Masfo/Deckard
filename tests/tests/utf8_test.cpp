#include <catch2/catch_test_macros.hpp>


import std;
import deckard.utf8;
import deckard.as;
using namespace deckard;
using namespace deckard::utf8;
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

		utf8::string heart("♥");

		c += heart;
		CHECK(c.size() == 11);
		CHECK(c.front() == 'h');
		CHECK(c.back() == 0x2665);
		CHECK(c == "hello 🌍b🌍🌍♥");


		b = "♥"_utf8;
		c = b + "♦"_utf8;
		CHECK(c.size() == 2);
		CHECK(c.front() == 0x2665);
		CHECK(c.back() == 0x2666);
		CHECK(c == "♥♦");

		c += "♧"_utf8;
		CHECK(c.size() == 3);
		CHECK(c.front() == 0x2665);
		CHECK(c.back() == 0x2667);
		CHECK(c == "♥♦♧");

		//
		utf8::string d("a");
		CHECK(d.size() == 1);
		CHECK(d[0] == 'a');

		d.insert(d.begin(), "b");

		CHECK(d.size() == 2);
		CHECK(d[0] == 'b');
		CHECK(d[1] == 'a');

		d.insert(d.end(), "c");

		CHECK(d.size() == 3);
		CHECK(d[0] == 'b');
		CHECK(d[1] == 'a');
		CHECK(d[2] == 'c');


		d.insert(d.begin() + 1, "d");

		CHECK(d.size() == 4);
		CHECK(d[0] == 'b');
		CHECK(d[1] == 'd');
		CHECK(d[2] == 'a');
		CHECK(d[3] == 'c');

		d.insert(d.begin() + 4, heart);

		CHECK(d.size() == 5);
		CHECK(d[0] == 'b');
		CHECK(d[1] == 'd');
		CHECK(d[2] == 'a');
		CHECK(d[3] == 'c');
		CHECK(d[4] == heart[0]);

		utf8::string world("🌍");

		d.insert(d.begin() + 1, world);
		CHECK(d.size() == 6);
		CHECK(d[0] == 'b');
		CHECK(d[1] == world[0]);
		CHECK(d[2] == 'd');
		CHECK(d[3] == 'a');
		CHECK(d[4] == 'c');
		CHECK(d[5] == heart[0]);

		d.insert(d.end(), 'X');
		CHECK(d.size() == 7);
		CHECK(d[0] == 'b');
		CHECK(d[1] == world[0]);
		CHECK(d[2] == 'd');
		CHECK(d[3] == 'a');
		CHECK(d[4] == 'c');
		CHECK(d[5] == heart[0]);
		CHECK(d[6] == 'X');

		char32 q = 0x274c; // ❌
		d.insert(d.begin(), q);
		CHECK(d.size() == 8);
		CHECK(d[0] == q);
		CHECK(d[1] == 'b');
		CHECK(d[2] == world[0]);
		CHECK(d[3] == 'd');
		CHECK(d[4] == 'a');
		CHECK(d[5] == 'c');
		CHECK(d[6] == heart[0]);
		CHECK(d[7] == 'X');
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

	SECTION("replace")
	{
		utf8::string str("hello world");
		str.replace(6, 5, "🌍"_utf8);
		CHECK(str == "hello 🌍");
		CHECK(str.size() == 7);

		str = "hello 🌍";
		str.replace(6, 1, "world");
		CHECK(str == "hello world");
		CHECK(str.size() == 11);

		str = "hello 🌍";
		str.replace(0, 5, "hi");
		CHECK(str == "hi 🌍");
		CHECK(str.size() == 4);

		str = "hello 🌍";
		str.replace(0, 7, "hi");
		CHECK(str == "hi");
		CHECK(str.size() == 2);

		str = "hello 🌍";
		str.replace(6, 1, "🌍🌍");
		CHECK(str == "hello 🌍🌍");
		CHECK(str.size() == 8);

		str = "🌍 hello 🌍";
		utf8::string repl("🌍");
		str.replace(str.begin() + 2, str.end() - 2, repl);
		CHECK(str == "🌍 🌍 🌍");
		CHECK(str.size() == 5);
		CHECK(str[0] == repl[0]);
		CHECK(str[1] == ' ');
		CHECK(str[2] == repl[0]);
		CHECK(str[3] == ' ');
		CHECK(str[4] == repl[0]);
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

		utf8::string str("hello world");
		str.erase(5, 1);
		CHECK(str == "helloworld");
		CHECK(str.size() == 10);

		str = "hello world";
		str.erase(0, 6);
		CHECK(str == "world");
		CHECK(str.size() == 5);

		str = "hello world";
		str.erase(5, 6);

		CHECK(str == "hello"sv);
		CHECK(str.size() == 5);

		str = "hello 🌍 world";
		str.erase(6, 1);
		CHECK(str == "hello  world");
		CHECK(str.size() == 12);

		str = "AÄ↥🌍";
		str.erase(1, 2);
		CHECK(str == "A🌍");
		CHECK(str.size() == 2);

		str      = "hello world";
		auto it1 = str.begin() + 5;
		auto it2 = str.begin() + 6;
		str.erase(it1, it2);
		CHECK(str == "helloworld");
		CHECK(str.size() == 10);

		str = "hello🌍world";
		str.erase(str.begin() + 5);
		CHECK(str == "helloworld");
		CHECK(str.size() == 10);

		str = "🌍test🌍";
		str.erase(0, 1);
		CHECK(str == "test🌍");
		CHECK(str.size() == 5);

		str.erase(4, 1);
		CHECK(str == "test");
		CHECK(str.size() == 4);

		str = "test";
		str.erase(0, str.size());
		CHECK(str.empty());
		CHECK(str.size() == 0);
	}

	SECTION("iterator distance")
	{
		utf8::string str;
		CHECK(str.end() - str.begin() == 0);
		CHECK(std::distance(str.begin(), str.end()) == 0);

		str = "hello world";
		CHECK(str.end() - str.begin() == 11);
		CHECK(std::distance(str.begin(), str.end()) == 11);

		str = "hello 🌍 world";
		CHECK(str.end() - str.begin() == 13);
		CHECK(std::distance(str.begin(), str.end()) == 13);

		str = "hello 🌍 world";
		CHECK(str.end() - str.begin() == 13);

		str = "AÄ↥🌍";
		CHECK(str.end() - str.begin() == 4);
		CHECK(std::distance(str.rbegin(), str.rend()) == 4);
	}

	SECTION("index operator")
	{

		utf8::string str("hello world");
		CHECK(str.size() == 11);

		CHECK(str[0] == (u32)'h');
		CHECK(str[1] == (u32)'e');
		CHECK(str[2] == (u32)'l');
		CHECK(str[3] == (u32)'l');
		CHECK(str[4] == (u32)'o');

		CHECK(str[5] == (u32)' ');

		CHECK(str[6] == (u32)'w');
		CHECK(str[7] == (u32)'o');
		CHECK(str[8] == (u32)'r');
		CHECK(str[9] == (u32)'l');
		CHECK(str[10] == (u32)'d');
	}

	SECTION("pre/post iterator ascii")
	{
		utf8::string str("hello world");
		CHECK(str.size() == 11);

		CHECK(std::distance(str.begin(), str.end()) == 11);


		auto it = str.begin();
		CHECK(*it == (u32)'h');

		auto pre = ++it;
		CHECK(*pre == (u32)'e');
		CHECK(*it == (u32)'e');

		it        = str.begin();
		auto post = it++;
		CHECK(*post == (u32)'h');
		CHECK(*it == (u32)'e');

		post = --it;
		CHECK(*post == (u32)'h');
		CHECK(*it == (u32)'h');

		it++;

		post = it--;
		CHECK(*post == (u32)'e');
		CHECK(*it == (u32)'h');


		it = str.end();
		it--;
		CHECK(*it == (u32)'d');
		auto pre2 = --it;
		CHECK(*pre2 == (u32)'l');
		CHECK(*it == (u32)'l');
	}

	SECTION("pre/post iterator ascii/utf8")
	{
		utf8::string str("🌍hello🌍 world🌍");
		CHECK(str.size() == 14);

		CHECK(std::distance(str.begin(), str.end()) == 14);

		auto it = str.begin();
		CHECK(*it == 0x1'f30d);

		auto pre = ++it;
		CHECK(*pre == (u32)'h');
		CHECK(*it == (u32)'h');

		it        = str.begin();
		auto post = it++;
		CHECK(*post == 0x1'f30d);
		CHECK(*it == (u32)'h');

		post = --it;
		CHECK(*post == 0x1'f30d);
		CHECK(*it == 0x1'f30d);

		it++;

		post = it--;
		CHECK(*post == (u32)'h');
		CHECK(*it == 0x1'f30d);


		it = str.end();
		it--;
		CHECK(*it == 0x1'f30d);
		auto pre2 = --it;
		CHECK(*pre2 == (u32)'d');
		CHECK(*it == (u32)'d');
	}

	SECTION("find_first_of")
	{
#if 0
		utf8::string str("🌍hello🌍 world🌍");
		utf8::string w("🌍"_utf8);
		auto         it = str.find_first_of(w);
		CHECK(std::distance(str.begin(), it) == 0);
		
		//it = str.find_first_of("w"_utf8);
		CHECK(std::distance(str.begin(), it) == 8);
#endif
	}
}

TEST_CASE("utf8::view", "[utf8][utf8view]")
{
	SECTION("c-tors")
	{
		std::array<u8, 4> buffer{'A', 'B', 'C', 'D'};

		utf8::view        v(buffer);
		CHECK(v.size() == 4);

		utf8::string str("🌍hello🌍");
		utf8::view   w(str);
		CHECK(w.size() == 7);

		//
	}
}
