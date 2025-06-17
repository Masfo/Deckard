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
		// CHECK(str.count() == 2); // TODO: should be 1

		str = "नमस्ते";
		CHECK(str.length() == 6);
		CHECK(str.size_in_bytes() == 18);
	}


	SECTION("unicode c-tor")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.valid() == true);
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
		CHECK(a.valid() == true);
		CHECK(std::string(a.c_str()) == "hello 🌍"sv);

		CHECK(b.size() == 7);
		CHECK(b.length() == 7);
		CHECK(b.empty() == false);
		CHECK(b.size_in_bytes() == 10);
		CHECK(b.valid() == true);
		CHECK(std::string(b.c_str()) == "hello 🌍"sv);

		CHECK(a == b);
		CHECK(a.c_str() != b.c_str());
	}

	SECTION("move")
	{

		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);


		auto moved = std::move(str);

		CHECK(moved.size() == 7);
		CHECK(moved == "hello 🌍"_utf8);

		CHECK(str.size() == 0);
		CHECK(str.empty());
		CHECK(str != "hello 🌍"_utf8);

		// Test self-move
		moved = std::move(moved);
		CHECK(moved.size() == 7);
		CHECK(moved == "hello 🌍"_utf8);
	}

	SECTION("move operations")
	{
		utf8::string original("hello 🌍");
		CHECK(original.size() == 7);
		CHECK(original.length() == 7);
		CHECK(original.size_in_bytes() == 10);

		// Move constructor
		utf8::string moved(std::move(original));
		CHECK(moved.size() == 7);
		CHECK(moved.length() == 7);
		CHECK(moved.size_in_bytes() == 10);
		CHECK(moved == "hello 🌍"_utf8);

		// Original should be empty after move
		CHECK(original.empty());
		CHECK(original.size() == 0);
		CHECK(original.length() == 0);
		CHECK(original.size_in_bytes() == 0);

		// Move assignment
		utf8::string target;
		target = std::move(moved);
		CHECK(target.size() == 7);
		CHECK(target.length() == 7);
		CHECK(target.size_in_bytes() == 10);
		CHECK(target == "hello 🌍"_utf8);

		// Source should be empty after move
		CHECK(moved.empty());
		CHECK(moved.size() == 0);
		CHECK(moved.length() == 0);
		CHECK(moved.size_in_bytes() == 0);

		// Self move assignment should be safe
		target = std::move(target);
		CHECK(target.size() == 7);
		CHECK(target.length() == 7);
		CHECK(target.size_in_bytes() == 10);
		CHECK(target == "hello 🌍"_utf8);

		// Chain of moves
		utf8::string a("test 🌍");
		utf8::string b = std::move(a);
		utf8::string c = std::move(b);
		CHECK(a.empty());
		CHECK(b.empty());
		CHECK(c == "test 🌍"_utf8);
		CHECK(c.size() == 6);
	}


	SECTION("assign c-string")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.valid() == true);
		CHECK(std::string(str.c_str()) == "hello 🌍"sv);
		//


		str = "hello";
		CHECK(str.size() == 5);
		CHECK(str.length() == 5);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 5);
		CHECK(str.valid() == true);
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

	SECTION("prepend")
	{
		//
		utf8::string a("hello 🌍");

		CHECK(a.size() == 7);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 0x1'f30d);


		a.prepend("."sv);

		CHECK(a.size() == 8);
		CHECK(a.front() == '.');
		CHECK(a.back() == 0x1'f30d);

		char32 q = 0x274c; // ❌

		a.prepend(q);
		CHECK(a.size() == 9);
		CHECK(a.front() == 0x274c);
		CHECK(a.back() == 0x1'f30d);

		utf8::string b("a");
		a.prepend(b);
		CHECK(a.size() == 10);
		CHECK(a.front() == 'a');
		CHECK(a.back() == 0x1'f30d);
	}

	SECTION("length")
	{
		utf8::string a("jμΛIα");
		CHECK(a.size() == 5);
		CHECK(a.length() == 5);
	}

	SECTION("substr")
	{
		utf8::string a("hello world");

		CHECK(a.size() == 11);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 'd');

		auto sub = a.substr(6, 5);

		CHECK(sub.size() == 5);
		CHECK(sub.front() == 'w');
		CHECK(sub.back() == 'd');


		utf8::string b("hello 🌍");
		auto         c = b.substr(6, 1);
		CHECK(c.size() == 1);
		CHECK(c.front() == 0x1'f30d);
		CHECK(c.back() == 0x1'f30d);

		utf8::string d("hello 🌍");
		auto         e = b.substr(6, 25);
		CHECK(e.size() == 1);
		CHECK(e.front() == 0x1'f30d);
		CHECK(e.back() == 0x1'f30d);


		// utf8::string f("hello 🌍");
		// auto         g = f.substr(100, 1);
		// CHECK(g.size() == 0);
	}

	SECTION("substr iterator")
	{
		utf8::string a("hello 🌍");

		CHECK(a.size() == 7);
		CHECK(a.front() == 'h');
		CHECK(a.back() == 0x1'f30d);

		utf8::string b = a.substr(a.begin(), 5);
		CHECK(b.size() == 5);
		CHECK(b[0] == 'h');
		CHECK(b[1] == 'e');
		CHECK(b[2] == 'l');
		CHECK(b[3] == 'l');
		CHECK(b[4] == 'o');

		utf8::string c = a.substr(a.begin() + 6, 1);
		CHECK(c.size() == 1);
		CHECK(c[0] == 0x1'f30d);
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

		CHECK(a.starts_with('A') == true);

		char32 q = 0x41; // 'A' U+0041
		CHECK(a.starts_with(q) == true);
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


		CHECK(a.find("🌍", 10) == 17);
	}

	SECTION("resize (smaller->larger)")
	{
		utf8::string a("hello 🌍");
		CHECK(a.size() == 7);

		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);

		a.resize(10);
		CHECK(a.size() == 10);
		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);
		CHECK(a[7] == 0);
		CHECK(a[8] == 0);
		CHECK(a[9] == 0);
	}

	SECTION("resize (smaller->larger)")
	{
		utf8::string a("hello 🌍 hello");

		CHECK(a.size() == 13);
		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);
		CHECK(a[7] == ' ');
		CHECK(a[8] == 'h');
		CHECK(a[9] == 'e');
		CHECK(a[10] == 'l');
		CHECK(a[11] == 'l');
		CHECK(a[12] == 'o');

		a.resize(7);

		CHECK(a.size() == 7);
		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);
	}

	SECTION("resize empty")
	{
		utf8::string a;
		a.resize(7);

		CHECK(a.size() == 7);
		CHECK(a[0] == 0);
		CHECK(a[1] == 0);
		CHECK(a[2] == 0);
		CHECK(a[3] == 0);
		CHECK(a[4] == 0);
		CHECK(a[5] == 0);
		CHECK(a[6] == 0);
	}

	SECTION("resize to empty")
	{
		utf8::string a("hello 🌍");
		CHECK(a.size() == 7);

		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);

		a.resize(0);
		CHECK(a.size() == 0);
	}

	SECTION("resize same size")
	{
		utf8::string a("hello 🌍");

		CHECK(a.size() == 7);
		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);

		a.resize(a.size());

		CHECK(a.size() == 7);
		CHECK(a[0] == 'h');
		CHECK(a[1] == 'e');
		CHECK(a[2] == 'l');
		CHECK(a[3] == 'l');
		CHECK(a[4] == 'o');
		CHECK(a[5] == ' ');
		CHECK(a[6] == 0x1'f30d);
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

	SECTION("iterator begin/end deref ascii")
	{
		utf8::string str("hello ");

		auto begin = str.begin();
		auto end   = str.end() - 1;

		CHECK(*begin == (u32)'h');
		CHECK(*end == (u32)' ');
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


		utf8::string abc("abcdefg");
		auto         it2 = abc.begin() + 0;
		CHECK(*it2 == 'a');

		it2 += 1;
		CHECK(*it2 == 'b');

		it2 += 4;
		CHECK(*it2 == 'f');

		it2 -= 4;
		CHECK(*it2 == 'b');

		it2 -= 1;
		CHECK(*it2 == 'a');
	}

	SECTION("construct string from iterator")
	{
		utf8::string str("🌍hello🌍 world🌍");

		auto it    = str.begin() + str.find("hello"sv);
		auto itend = it + 6;

		utf8::string str_from_iter(it, itend);
		CHECK(str_from_iter.size() == 6);
		CHECK(str_from_iter[0] == (u32)'h');
		CHECK(str_from_iter[1] == (u32)'e');
		CHECK(str_from_iter[2] == (u32)'l');
		CHECK(str_from_iter[3] == (u32)'l');
		CHECK(str_from_iter[4] == (u32)'o');
		CHECK(str_from_iter[5] == 0x1'f30d);
	}

	SECTION("find_first_of")
	{
		utf8::string str("🌍hello❌ world🌍");
		utf8::string w("🌍"_utf8);
		auto         found = str.find_first_of(w);
		CHECK(found == 0);

		found = str.find_first_of("w"sv);
		CHECK(found == 8);


		w     = "d"_utf8;
		found = str.find_first_of(w.subview());
		CHECK(found == 12);


		char32 q = 0x274c;   // ❌
		found    = str.find_first_of(q);
		CHECK(found == 6);

		char a = 'e';
		found  = str.find_first_of(a);
		CHECK(found == 2);

	}

	SECTION("find_first_not_of")
	{ 
		utf8::string str("🌍hello❌ world🌍");
		utf8::string w("🌍❌oleh "_utf8);

		auto found = str.find_first_not_of(w);
		CHECK(found == 8);

		char32 q = 0x274c; // ❌
		found    = str.find_first_not_of(q);
		CHECK(found == 0);
		
		CHECK(0 == str.find_first_not_of('e'));
	}

	SECTION("find_last_of")
	{ 
		utf8::string str("12 i12 12345");
		utf8::string w("i12"_utf8);

		auto found = str.find_last_of(w);
		CHECK(found == 8);

		utf8::string str2("12 ❌q2 12345");
		utf8::string w2("❌q2"_utf8);

		found = str2.find_last_of(w2);
		CHECK(found == 8);

		//found = str2.find_last_of("q2"sv);
		//CHECK(found == 5);
		
	}

	SECTION("find_last_not_of")
	{
		//
	}

	SECTION("rfind")
	{
		//
	}



	SECTION("hash")
	{
		utf8::string str("🌍hello🌍 world🌍");
		CHECK(str.size() == 14);
		CHECK(str.length() == 14);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 23);
		CHECK(str.valid() == true);
		//	CHECK(std::hash<utf8::string>{}(str) == 0x25);
	}

	SECTION("reserve")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.capacity() == 31);


		auto str2 = str;
		for (int i = 0; i < 5; i++)
			str2 += "hello 🌍";

		CHECK(str2.size() == 42);
		CHECK(str2.capacity() == 72);

		str.reserve(256);

		CHECK(str.size() == 7);
		CHECK(str.capacity() == 256);

		str.append(str2);
		CHECK(str.size() == 49);
		CHECK(str.capacity() == 256);

		str.append(str2);
		CHECK(str.size() == 91);
		CHECK(str.capacity() == 256);
	}

	SECTION("capacity")
	{

		utf8::string str("hello 🌍");
		utf8::string str2 = str;


		CHECK(str.size() == 7);
		CHECK(str.capacity() == 31);

		str.append(str2);
		CHECK(str.size() == 14);

		str.append(str2);
		CHECK(str.size() == 21);
		CHECK(str.capacity() == 31);

		str.append(str2);
		CHECK(str.size() == 28);
		CHECK(str.capacity() == 48);

		str.append(str2);
		CHECK(str.size() == 35);
		CHECK(str.capacity() == 72);
	}

	SECTION("subspan")
	{
		utf8::string str("hello 🌍 world");
		CHECK(str.size() == 13);
		CHECK(str.capacity() == 31);

		auto hello = str.subspan(0, 7);
		CHECK(hello.size() == 10);

		auto world = str.subspan(8, 64);
		CHECK(world.size() == 5);

		auto glyph = str.subspan(6, 1);
		CHECK(glyph.size() == 4);
	}


	SECTION("valid")
	{
		utf8::string str("hello 🌍 world");

		CHECK(str.size() == 13);
		CHECK(str.capacity() == 31);
		CHECK(str.valid() == true);

		{                                   // 1-byte
			std::array<u8, 1> err = {0x21}; // !' U+0021
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{                                         // 2-byte
			std::array<u8, 2> err = {0xC3, 0xA9}; // é, U+00E9
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{                                               // 3-byte
			std::array<u8, 3> err = {0xE2, 0x82, 0xAC}; // '€', U+20AC
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{                                                     // 4-byte
			std::array<u8, 4> err = {0xF0, 0x9F, 0x98, 0x80}; // 😀 U+1F600
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{                                                     // 4-byte
			std::array<u8, 4> err = {0xF4, 0x8F, 0xBF, 0xBF}; // U+10FFFF, last codepoint

			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}
	}

	SECTION("graphemes")
	{
		{
			std::array<u8, 5> err = {
			  // 5 ascii
			  'h',
			  'e',
			  'l',
			  'l',
			  'o',
			};

			utf8::string str(err);
			CHECK(str.size() == 5);
			CHECK(str.size_in_bytes() == 5);
			CHECK(str.graphemes() == 5);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{
			std::array<u8, 3> err = {
			  // e + acute
			  0x65,
			  0xcc,
			  0x81, // e + acute (U+0301)
			};

			utf8::string str(err);
			CHECK(str.size() == 2);
			CHECK(str.size_in_bytes() == 3);
			CHECK(str.graphemes() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{
			// e + acute + o
			std::array<u8, 4> err = {
			  0x67,
			  0xcc,
			  0x88,
			  0x6f, // e + acute (U+0301) + o
			};

			utf8::string str(err);
			CHECK(str.size() == 3);
			CHECK(str.size_in_bytes() == 4);
			CHECK(str.graphemes() == 2);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{
			// 3 bytes, 1 grapheme
			// Devanagari Ka क
			std::array<u8, 3> err = {0xE0, 0xA4, 0x95};
			utf8::string      str(err);
			CHECK(str.size() == 1);
			CHECK(str.size_in_bytes() == 3);
			CHECK(str.graphemes() == 1);

			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{
			// 6 bytes, 1 grapheme
			// Ka + virama
			std::array<u8, 6> err = {0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8D};
			utf8::string      str(err);
			CHECK(str.size() == 2);
			CHECK(str.size_in_bytes() == 6);
			CHECK(str.graphemes() == 2);

			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}
		{
			// 9 bytes, 1 grapheme
			// Ka + virama + Ya (conjunct) क्य
			std::array<u8, 9> err = {0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xAF};
			utf8::string      str(err);
			CHECK(str.size() == 3);
			CHECK(str.size_in_bytes() == 9);
			CHECK(str.graphemes() == 3);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{
			// 11 bytes, 1 grapheme
			// emoji zwj emoji

			// clang-format off
			std::array<u8, 11> err = {
				0xF0, 0x9F, 0x91, 0xA9, // 👩
				0xE2, 0x80, 0x8D,		// ZWJ
				0xF0, 0x9F,0x9A,0x80	//🚀
			};
			// clang-format on

			utf8::string str(err);
			CHECK(str.size() == 3);
			CHECK(str.size_in_bytes() == 11);
			CHECK(str.graphemes() == 3); // TODO: should be one 👩‍🚀

			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}

		{
			// clang-format off
			std::array<u8, 10> err = {
				'e',				//
				0xCC, 0x81,			// combining acute (U+0301)
				'l', 'o',			//
				0xE4, 0xB8, 0xAD,	// 中 ( 3-byte chinese)
				0xCC, 0x88,			// combining diaeresis
			};
			// clang-format on

			utf8::string str(err);
			CHECK(str.size() == 6);
			CHECK(str.graphemes() == 4);
			CHECK(str.capacity() == 31);
			CHECK(str.valid() == true);
		}
	}

	SECTION("invalid, overlong")
	{
		// clang-format off
		std::array<u8, 2> overlong = {
		  0xC0, 0xAF,				// overlong encoding of '/'
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("Overlong 2-byte") == true);
		CHECK(err.error().contains("index 0") == true);
	}

	SECTION("invalid, missing continuation byte")
	{
		// clang-format off
		std::array<u8, 5> overlong = {
		  'A','B','C',
		  0xE2, 0x28,				// 3-byte sequence, missing 3rd byte
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("continuation") == true);
		CHECK(err.error().contains("index 3") == true);
	}


	SECTION("invalid, surrogate half")
	{
		// clang-format off
		std::array<u8, 3> overlong = {
		  0xED, 0xA0, 0x80,				// U+D800 encoded directly
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("surrogate") == true);
		CHECK(err.error().contains("index 0") == true);
	}

	SECTION("invalid, surrogate half")
	{
		// clang-format off
		std::array<u8, 5> overlong = {
		  'A',
		  0xF0, 0x80, 0x80, 0xAF,				// overlong encoding
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("Overlong 4-byte") == true);
		CHECK(err.error().contains("index 1") == true);
	}

	SECTION("invalid, codepoint beyond U+10FFFF")
	{
		// clang-format off
		std::array<u8, 6> overlong = {
		  'A', 'B',
		  0xF4, 0x90, 0x80, 0x80,				// codepoint beyond U+10FFFF
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("beyond") == true);
		CHECK(err.error().contains("index 2") == true);
	}


	SECTION("invalid, single continuation byte")
	{
		// clang-format off
		std::array<u8, 5> overlong = {
		  'A', 'B', 'C', 'D',
		  0x80,				// continuation byte
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("leading") == true);
		CHECK(err.error().contains("index 4") == true);
	}

	SECTION("invalid, bad leading byte")
	{
		// clang-format off
		std::array<u8, 3> overlong = {
		  'A', 'B',
		  0xFF,				// invalid leading byte
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("leading") == true);
		CHECK(err.error().contains("index 2") == true);
	}

	SECTION("invalid, truncated 4-byte sequence")
	{
		// clang-format off
		std::array<u8, 6> overlong = {
		  'A', 'B', 'C',
		  0xF0, 0x9F, 0x92				// missing 4th byte
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("continuation") == true);
		CHECK(err.error().contains("index 3") == true);
	}

	SECTION("invalid, overlong")
	{
		// clang-format off
		std::array<u8, 9> overlong = {
		  0x48,						// H
		  0xC0, 0x81,				// overlong encoding
		  0x65,						// e
		  0xF4, 0x90, 0x80, 0x80,	// > U+10FFFF
		  0x21						// !
		};
		// clang-format on

		utf8::string str(overlong);

		CHECK(str.size() == 0);
		CHECK(str.capacity() == 31);

		const auto err = str.valid();
		CHECK(err.error().contains("Overlong 2-byte") == true);
		CHECK(err.error().contains("index 1") == true);
	}
}

// #####################################
// #####################################
// #####################################
// #####################################


TEST_CASE("utf8::view", "[utf8][utf8view]")
{
	SECTION("c-tors")
	{
		std::array<u8, 4> buffer{'A', 'B', 'C', 'D'};

		utf8::view v(buffer);
		CHECK(v.size() == 4);

		utf8::string str("🌍hello🌍");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		//
	}

	SECTION("compare")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);


		utf8::view w2(str.data());
		CHECK(w2.size() == 7);

		CHECK(w == w2);
	}

	SECTION("decode")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		CHECK(*w == 0x1'f30d);

		w++;
		CHECK(*w == 'h');
		w++;
		CHECK(*w == 'e');
		w++;
		CHECK(*w == 'l');
		w++;
		CHECK(*w == 'l');
		w++;
		CHECK(*w == 'o');

		w++;
		CHECK(*w == 0x1'f30d);

		w--;
		CHECK(*w == 'o');

		w--;
		w--;
		w--;
		w--;
		CHECK(*w == 'h');


		w += 5;
		CHECK(*w == 0x1'f30d);

		w -= 6;
		CHECK(*w == 0x1'f30d);

		w += 3;
		CHECK(*w == 'l');
		w -= 1;
		CHECK(*w == 'e');
	}

	SECTION("indexing")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   w(str.subspan());
		CHECK(w.size() == 7);
		CHECK(w[0] == 0x1'f30d);
		CHECK(w[1] == 'h');
		CHECK(w[2] == 'e');
		CHECK(w[3] == 'l');
		CHECK(w[4] == 'l');
		CHECK(w[5] == 'o');
		CHECK(w[6] == 0x1'f30d);
	}


	SECTION("subview")
	{
		utf8::string str("🌍hello🌍");

		utf8::view v(str.subview(1, 5));
		CHECK(v.size() == 5);
		CHECK(v[0] == 'h');
		CHECK(v[1] == 'e');
		CHECK(v[2] == 'l');
		CHECK(v[3] == 'l');
		CHECK(v[4] == 'o');

		v = str.subview(6, 1);
		CHECK(v.size() == 1);
		CHECK(v[0] == 0x1'f30d);


		v = utf8::view(str);
		CHECK(v.size() == 7);
		CHECK(v[0] == 0x1'f30d);
		CHECK(v[1] == 'h');
		CHECK(v[2] == 'e');
		CHECK(v[3] == 'l');
		CHECK(v[4] == 'l');
		CHECK(v[5] == 'o');
		CHECK(v[6] == 0x1'f30d);
	}

	SECTION("hash")
	{
		utf8::string str1("hello 🌍");
		utf8::string str2("hello 🌍");
		utf8::string str3("different string");

		std::hash<utf8::string> hasher;

		CHECK(hasher(str1) == hasher(str2));
		CHECK(hasher(str1) != hasher(str3));
	}
}
