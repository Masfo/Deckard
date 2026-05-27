#include <catch2/catch_test_macros.hpp>


import std;
import deckard.utf8;
import deckard.as;
import deckard.types;

using namespace deckard;
using namespace deckard::utf8;
using namespace std::string_view_literals;

TEST_CASE("utf8::ascii", "[utf8]")
{
	//
	SECTION("ascii")
	{
		//
	}

	SECTION("yield_codepoints invalid/truncated")
	{

		{
			std::array<u8, 3>   buf{u8{0xF0}, u8{0x9F}, u8{0x98}}; // truncated 4-byte sequence
			std::vector<char32> cps;
			for (auto cp : utf8::yield_codepoints(buf))
				cps.push_back(cp);
			CHECK(cps.size() == 1);
			CHECK(cps[0] == utf8::REPLACEMENT_CHARACTER);
		}

		{
			std::array<u8, 2>   buf{u8{0x80}, u8{0x80}}; // two invalid standalone continuation bytes
			std::vector<char32> cps;
			for (auto cp : utf8::yield_codepoints(buf))
				cps.push_back(cp);
			CHECK(cps.size() == 2);
			CHECK(cps[0] == utf8::REPLACEMENT_CHARACTER);
			CHECK(cps[1] == utf8::REPLACEMENT_CHARACTER);
		}
	}

	SECTION("codepoint widths")
	{
		CHECK(utf8::codepoint_width(U'$') == 1);
		CHECK(utf8::codepoint_width(U'£') == 2);
		CHECK(utf8::codepoint_width(U'€') == 3);
		CHECK(utf8::codepoint_width(U'₿') == 3);
		CHECK(utf8::codepoint_width(U'💶') == 4);
		CHECK(utf8::codepoint_width(U'💵') == 4);
		CHECK(utf8::codepoint_width(U'💴') == 4);
		CHECK(utf8::codepoint_width(U'💷') == 4);
	}
}

// Additional sections for utf8::string tests
TEST_CASE("utf8::string", "[utf8]")
{

	SECTION("ascii c-tor")
	{
		utf8::string str("hello world");
		CHECK(str.size() == 11);
		CHECK(str.length() == 11);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 11);

		CHECK(std::string(str.as_string_view()) == "hello world"sv);


		str = "\xF0\x9F\xA4\xA6\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F";
		CHECK(str.length() == 5);
		CHECK(str.graphemes() == 1);
		CHECK(str.size_in_bytes() == 17);

		str = "नमस्ते";
		CHECK(str.length() == 6);
		CHECK(str.size_in_bytes() == 18);
	}

	SECTION("initialize from array of bytes")
	{
		std::array<u8, 5> bytes = {'h', 'e', 'l', 'l', 'o'};
		utf8::string      str(bytes);
		CHECK(str.size() == 5);
		CHECK(str.size_in_bytes() == 5);
		CHECK(str.as_string_view() == std::string_view("hello"));

		std::array<u8, 10> bytes2 = {'h', 'e', 'l', 'l', 'o', ' ', 0xF0, 0x9F, 0x8C, 0x8D}; // "hello 🌍"
		utf8::string       str2(bytes2);
		CHECK(str2.size() == 7);
		CHECK(str2.size_in_bytes() == 10);

		auto data = str2.span();
		CHECK(data[0] == 'h');
		CHECK(data[5] == ' ');
		CHECK(data[6] == 0xF0);
		CHECK(data[7] == 0x9F);
		CHECK(data[8] == 0x8C);
		CHECK(data[9] == 0x8D);
	}


	SECTION("unicode c-tor")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.valid());
		CHECK(std::string(str.as_string_view()) == "hello 🌍"sv);
	}

	SECTION("initialize from vector of codepoints")
	{
		utf8::string str(std::vector<char32>{'h', 'e', 'l', 'l', 'o', ' ', '🌍'});
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.valid());
		CHECK(std::string(str.as_string_view()) == "hello 🌍"sv);
	}


	SECTION("copy assignment")
	{

		utf8::string a("hello 🌍");
		auto         b = a;

		CHECK(a.size() == 7);
		CHECK(a.length() == 7);
		CHECK(a.empty() == false);
		CHECK(a.size_in_bytes() == 10);
		CHECK(a.valid());
		CHECK(std::string(a.as_string_view()) == "hello 🌍"sv);

		CHECK(b.size() == 7);
		CHECK(b.length() == 7);
		CHECK(b.empty() == false);
		CHECK(b.size_in_bytes() == 10);
		CHECK(b.valid());
		CHECK(std::string(b.as_string_view()) == "hello 🌍"sv);

		CHECK(a == b);
	}

	SECTION("move")
	{

		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);


		auto moved = std::move(str);

		CHECK(moved.size() == 7);
		CHECK(moved == "hello 🌍");

		CHECK(str.size() == 0);
		CHECK(str.empty());
		CHECK(str != "hello 🌍");

		// Test self-move
		moved = std::move(moved);
		CHECK(moved.size() == 7);
		CHECK(moved == "hello 🌍");
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
		CHECK(moved == "hello 🌍");

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
		CHECK(target == "hello 🌍");

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
		CHECK(target == "hello 🌍");

		// Chain of moves
		utf8::string a("test 🌍");
		utf8::string b = std::move(a);
		utf8::string c = std::move(b);
		CHECK(a.empty());
		CHECK(b.empty());
		CHECK(c == "test 🌍");
		CHECK(c.size() == 6);
	}


	SECTION("assign c-string")
	{
		utf8::string str("hello 🌍");
		CHECK(str.size() == 7);
		CHECK(str.length() == 7);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 10);
		CHECK(str.valid());
		CHECK(std::string(str.as_string_view()) == "hello 🌍"sv);
		//


		str = "hello";
		CHECK(str.size() == 5);
		CHECK(str.length() == 5);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 5);
		CHECK(str.valid());
		CHECK(std::string(str.as_string_view()) == "hello"sv);
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

		// Minimal regression cases
		utf8::string e("a");
		e.insert(e.begin(), "b");
		CHECK(e.size() == 2);
		CHECK(e[0] == 'b');
		CHECK(e[1] == 'a');

		utf8::string f("a");
		f.insert(f.begin(), "🌍");
		CHECK(f.size() == 2);
		CHECK(f[0] == 0x1'f30d);
		CHECK(f[1] == 'a');
		f.insert(f.end(), "🌍");
		CHECK(f.size() == 3);
		CHECK(f[2] == 0x1'f30d);
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

	SECTION("self insert/append")
	{
		utf8::string a("hello");
		a.append(a);
		CHECK(a == "hellohello");
		CHECK(a.size() == 10);

		utf8::string b("ab");
		b.prepend(b);
		CHECK(b == "abab");
		CHECK(b.size() == 4);

		utf8::string c("hi");
		c.insert(c.end(), c);
		CHECK(c == "hihi");
		CHECK(c.size() == 4);

		utf8::string d("🌍");
		d.append(d);
		CHECK(d.size() == 2);
		CHECK(d[0] == 0x1'f30d);
		CHECK(d[1] == 0x1'f30d);
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
		str.replace(6, 5, "🌍");
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

	SECTION("replace going past end")
	{
		utf8::string str("hello 🌍");
		str.replace(6, 99, "world");
		CHECK(str == "hello world");
		CHECK(str.size() == 11);

		str = "hello 🌍";
		utf8::string src("ABCDE");
		utf8::view   src_view(src.data());
		str.replace(6, 99, src_view.subview(src_view, 3));
		CHECK(str == "hello ABC");
		CHECK(str.size() == 9);
	}

	SECTION("replace with another string 2")
	{
		utf8::string str("abc ascii 123");
		utf8::string src("ABC");

		str.replace(4, 5, src);
		CHECK(str.size() == 11);
		CHECK(str == "abc ABC 123");
	}
}

TEST_CASE("utf8::view", "[utf8][utf8view]")
{


	SECTION("indexing")
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

	SECTION("indexing multibyte")
	{
		// "🌍hello🌍" — 🌍 is U+1F30D (4 bytes), total 7 codepoints
		utf8::string str("🌍hello🌍");
		CHECK(str.size() == 7);

		CHECK(str[0] == 0x1'f30d); // 🌍
		CHECK(str[1] == (u32)'h');
		CHECK(str[2] == (u32)'e');
		CHECK(str[3] == (u32)'l');
		CHECK(str[4] == (u32)'l');
		CHECK(str[5] == (u32)'o');
		CHECK(str[6] == 0x1'f30d); // 🌍
	}

	SECTION("iterator::operator[] by codepoint index")
	{
		utf8::string str("🌍hello🌍");
		CHECK(str.size() == 7);

		auto it = str.begin();
		CHECK(it[0] == 0x1'f30d); // 🌍
		CHECK(it[1] == (u32)'h');
		CHECK(it[2] == (u32)'e');
		CHECK(it[3] == (u32)'l');
		CHECK(it[4] == (u32)'l');
		CHECK(it[5] == (u32)'o');
		CHECK(it[6] == 0x1'f30d); // 🌍

		CHECK(*it == 0x1'f30d);

		++it; // h
		CHECK(it[0] == (u32)'h');
		CHECK(it[1] == (u32)'e');
		CHECK(it[2] == (u32)'l');
		CHECK(it[3] == (u32)'l');
		CHECK(it[4] == (u32)'o');
		CHECK(it[5] == 0x1'f30d); // 🌍

		// still h
		CHECK(*it == (u32)'h');
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

		auto pos = str.find("hello"sv);
		CHECK(pos.has_value());
		auto it    = str.begin() + static_cast<utf8::iterator::difference_type>(pos.value());
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


	SECTION("trim_left")
	{
		utf8::string str("  🌍23❌");
		CHECK(str.size() == 6);
		CHECK(str == "  🌍23❌");

		str.trim_left();

		CHECK(str.size() == 4);
		CHECK(str == "🌍23❌");
	}

	SECTION("trim_right")
	{
		utf8::string str("🌍23❌  ");
		CHECK(str.size() == 6);
		CHECK(str == "🌍23❌  ");

		str.trim_right();

		CHECK(str.size() == 4);
		CHECK(str == "🌍23❌");
	}

	SECTION("trim")
	{
		utf8::string str("  hello 🌍  ");
		utf8::view   v = str;

		auto tl = v.trim_left();
		CHECK(tl.size() == 9);
		CHECK(tl[0] == 'h');
		CHECK(tl[6] == 0x1'f30d);

		auto tr = v.trim_right();
		CHECK(tr.size() == 9);
		CHECK(tr[0] == ' ');
		CHECK(tr[8] == 0x1'f30d);

		auto t = v.trim();
		CHECK(t.size() == 7);
		CHECK(t[0] == 'h');
		CHECK(t[6] == 0x1'f30d);
	}


	SECTION("hash")
	{
		utf8::string str("🌍hello🌍 world🌍");
		CHECK(str.size() == 14);
		CHECK(str.length() == 14);
		CHECK(str.empty() == false);
		CHECK(str.size_in_bytes() == 23);
		CHECK(str.valid());
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

	SECTION("subview")
	{
		utf8::string str("hello 🌍 world");

		CHECK(str.size() == 13);
		CHECK(str.capacity() == 31);

		utf8::view sub = str.subview(0, 7);
		CHECK(sub.size() == 7);
	}


	SECTION("valid")
	{
		utf8::string str("hello 🌍 world");

		CHECK(str.size() == 13);
		CHECK(str.capacity() == 31);
		CHECK(str.valid());

		{                                   // 1-byte
			std::array<u8, 1> err = {0x21}; // !' U+0021
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{                                         // 2-byte
			std::array<u8, 2> err = {0xC3, 0xA9}; // é, U+00E9
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{                                               // 3-byte
			std::array<u8, 3> err = {0xE2, 0x82, 0xAC}; // '€', U+20AC
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{                                                     // 4-byte
			std::array<u8, 4> err = {0xF0, 0x9F, 0x98, 0x80}; // 😀 U+1F600
			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{                                                     // 4-byte
			std::array<u8, 4> err = {0xF4, 0x8F, 0xBF, 0xBF}; // U+10FFFF, last codepoint

			str.assign(err);
			CHECK(str.size() == 1);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}
	}

	SECTION("length")
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
			CHECK(str.length() == 5);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{
			std::array<u8, 3> err = {
			  // e + acute
			  0x65,
			  0xCC,
			  0x81, // e + acute (U+0301)
			};

			utf8::string str(err);
			CHECK(str.size() == 2);
			CHECK(str.graphemes() == 1);
			CHECK(str.size_in_bytes() == 3);
			CHECK(str.length() == 2);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{
			// e + acute + o
			std::array<u8, 4> err = {
			  0x67,
			  0xCC,
			  0x88,
			  0x6F, // e + acute (U+0301) + o
			};

			utf8::string str(err);
			CHECK(str.size() == 3);
			CHECK(str.size_in_bytes() == 4);
			CHECK(str.length() == 3);
			CHECK(str.graphemes() == 2);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{
			// 3 bytes, 1 grapheme
			// Devanagari Ka क
			std::array<u8, 3> err = {0xE0, 0xA4, 0x95};
			utf8::string      str(err);
			CHECK(str.size() == 1);
			CHECK(str.size_in_bytes() == 3);
			CHECK(str.length() == 1);

			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}

		{
			// 6 bytes, 1 grapheme
			// Ka + virama
			std::array<u8, 6> err = {0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8D};
			utf8::string      str(err);
			CHECK(str.size() == 2);
			CHECK(str.size_in_bytes() == 6);
			CHECK(str.length() == 2);

			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}
		{
			// 9 bytes, 1 grapheme
			// Ka + virama + Ya (conjunct) क्य
			std::array<u8, 9> err = {0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xAF};
			utf8::string      str(err);
			CHECK(str.size() == 3);
			CHECK(str.size_in_bytes() == 9);
			CHECK(str.length() == 3);
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
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
			CHECK(str.graphemes() == 1);
			CHECK(str.size_in_bytes() == 11);
			CHECK(str.length() == 3);
			CHECK(str.graphemes() == 1);

			CHECK(str.capacity() == 31);
			CHECK(str.valid());
		}
		{
			// 28-byte flag,
			std::array<u8, 28> flag = {0xF0, 0x9F, 0x8F, 0xB4, 0xF3, 0xA0, 0x81, 0xA7, 0xF3, 0xA0, 0x81, 0xA2, 0xF3, 0xA0,
									   0x81, 0xA5, 0xF3, 0xA0, 0x81, 0xAE, 0xF3, 0xA0, 0x81, 0xA7, 0xF3, 0xA0, 0x81, 0xBF};

			utf8::string str(flag);
			CHECK(str.length() == 7);
			CHECK(str.graphemes() == 1);
			CHECK(str.valid());
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
			CHECK(str.length() == 6);
			CHECK(str.graphemes() == 4); // e + acute, l, o, 中 + diaeresis
			CHECK(str.capacity() == 31);
			CHECK(str.valid());
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
		CHECK(err.error().contains("line 1, column 1") == true);
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
		CHECK(err.error().contains("line 1, column 4") == true);
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
		CHECK(err.error().contains("line 1, column 1") == true);
	}

	SECTION("invalid, overlong 4-byte")
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
		CHECK(err.error().contains("line 1, column 2") == true);
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
		CHECK(err.error().contains("line 1, column 3") == true);
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
		CHECK(err.error().contains("line 1, column 5") == true);
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
		CHECK(err.error().contains("line 1, column 3") == true);
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
		CHECK(err.error().contains("line 1, column 4") == true);
	}

	SECTION("invalid, overlong 2-byte mixed")
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
		CHECK(err.error().contains("line 1, column 2") == true);
	}

	SECTION("utf8 string security checks")
	{
		{
			std::array<u8, 2> overlong_nul = {0xC0, 0x80};
			utf8::string      str(overlong_nul);

			CHECK(str.size() == 0);
			CHECK(str.capacity() == 31);

			const auto err = str.valid();
			CHECK(err.error().contains("Overlong 2-byte") == true);
			CHECK(err.error().contains("line 1, column 1") == true);
		}

		{
			std::array<u8, 1> truncated_2 = {0xC2};
			utf8::string      str(truncated_2);

			CHECK(str.size() == 0);
			CHECK(str.capacity() == 31);

			const auto err = str.valid();
			CHECK(err.error().contains("continuation") == true);
			CHECK(err.error().contains("line 1, column 1") == true);
		}

		{
			std::array<u8, 2> truncated_3 = {0xE2, 0x82};
			utf8::string      str(truncated_3);

			CHECK(str.size() == 0);
			CHECK(str.capacity() == 31);

			const auto err = str.valid();
			CHECK(err.error().contains("continuation") == true);
			CHECK(err.error().contains("line 1, column 1") == true);
		}

		for (u8 lead : {u8{0xF5}, u8{0xF6}, u8{0xF7}, u8{0xF8}, u8{0xF9}, u8{0xFA}, u8{0xFB}, u8{0xFC}, u8{0xFD}, u8{0xFE}})
		{
			std::array<u8, 1> invalid_leading = {lead};
			utf8::string      str(invalid_leading);

			CHECK(str.size() == 0);
			CHECK(str.capacity() == 31);

			const auto err = str.valid();
			CHECK_FALSE(err);
			CHECK(err.error().contains("line 1, column 1") == true);
		}

		{
			std::array<u8, 1> one_byte = {0x7F};
			utf8::string      str(one_byte);

			CHECK(str.size() == 1);
			CHECK(str[0] == 0x7F);
			CHECK(str.valid());
		}

		{
			std::array<u8, 2> two_byte = {0xC2, 0x80};
			utf8::string      str(two_byte);

			CHECK(str.size() == 1);
			CHECK(str[0] == 0x80);
			CHECK(str.valid());
		}

		{
			std::array<u8, 2> two_byte_max = {0xDF, 0xBF};
			utf8::string      str(two_byte_max);

			CHECK(str.size() == 1);
			CHECK(str[0] == 0x7FF);
			CHECK(str.valid());
		}

		{
			std::array<u8, 3> three_byte_min = {0xE0, 0xA0, 0x80};
			utf8::string      str(three_byte_min);

			CHECK(str.size() == 1);
			CHECK(str[0] == 0x800);
			CHECK(str.valid());
		}

		{
			std::array<u8, 3> three_byte_max = {0xEF, 0xBF, 0xBF};
			utf8::string      str(three_byte_max);

			CHECK(str.size() == 1);
			CHECK(str[0] == 0xFFFF);
			CHECK(str.valid());
		}

		{
			std::array<u8, 4> four_byte_min = {0xF0, 0x90, 0x80, 0x80};
			utf8::string      str(four_byte_min);

			CHECK(str.size() == 1);
			CHECK(str[0] == 0x1'0000);
			CHECK(str.valid());
		}

		{
			std::array<u8, 3> embedded_nul = {0x41, 0x00, 0x42};
			utf8::string      str(embedded_nul);

			CHECK(str.size() == 3);
			CHECK(str[0] == 'A');
			CHECK(str[1] == 0);
			CHECK(str[2] == 'B');
			CHECK(str.valid());
		}
	}
}

TEST_CASE("utf8::view", "[utf8]") { }

TEST_CASE("utf8::iterator::try_codepoint", "[utf8]")
{
	SECTION("ascii - valid positions")
	{
		utf8::string str("hello");
		auto         it = str.begin();

		auto cp = it.try_codepoint();
		CHECK(cp.has_value());
		CHECK(cp.value() == u8'h');

		++it;
		cp = it.try_codepoint();
		CHECK(cp.has_value());
		CHECK(cp.value() == u8'e');
	}

	SECTION("multibyte codepoint")
	{
		utf8::string str("\xF0\x9F\x98\x80"); // 😀 U+1F600
		auto         it = str.begin();

		auto cp = it.try_codepoint();
		CHECK(cp.has_value());
		CHECK(cp.value() == U'\U0001F600');
	}

	SECTION("end iterator returns nullopt")
	{
		utf8::string str("hi");
		auto         it = str.end();
		CHECK(it.try_codepoint().has_value() == false);
	}

	SECTION("empty string returns nullopt")
	{
		utf8::string str;
		auto         it = str.begin();
		CHECK(it.try_codepoint().has_value() == false);
	}

	SECTION("advancing past all codepoints returns nullopt")
	{
		utf8::string str("ab");
		auto         it = str.begin();

		CHECK(it.try_codepoint().has_value() == true);  // 'a'
		++it;
		CHECK(it.try_codepoint().has_value() == true);  // 'b'
		++it;
		CHECK(it.try_codepoint().has_value() == false); // end
	}

	SECTION("try_codepoint does not advance the iterator")
	{
		utf8::string str("xyz");
		auto         it = str.begin();

		auto cp1 = it.try_codepoint();
		auto cp2 = it.try_codepoint();
		CHECK(cp1.has_value());
		CHECK(cp2.has_value());
		CHECK(cp1.value() == cp2.value());
	}

	SECTION("multibyte string - all codepoints accessible")
	{
		utf8::string str("\xC3\xA9\xC3\xA0\xC3\xBC"); // é à ü (2 bytes each)
		auto         it = str.begin();

		auto cp = it.try_codepoint();
		CHECK(cp.has_value());
		CHECK(cp.value() == U'\u00e9');

		++it;
		cp = it.try_codepoint();
		CHECK(cp.has_value());
		CHECK(cp.value() == U'\u00e0');

		++it;
		cp = it.try_codepoint();
		CHECK(cp.has_value());
		CHECK(cp.value() == U'\u00fc');

		++it;
		CHECK(it.try_codepoint().has_value() == false);
	}

	SECTION("is valid codepoint")
	{
		char32 valid_char   = u8'A';     // 0x0041
		char32 emoji        = U'🚀';     // 0x1F680
		char32 surrogate    = 0xD801;    // Invalid scalar
		char32 out_of_range = 0x11'0000; // Invalid

		CHECK(is_valid_codepoint(valid_char));
		CHECK(is_valid_codepoint(emoji));
		CHECK_FALSE(is_valid_codepoint(surrogate));
		CHECK_FALSE(is_valid_codepoint(out_of_range));
	}


	SECTION("is_xid_start")
	{
		CHECK(is_xid_start(0x24));                    // '$'
		CHECK(is_xid_start(0x41));                    // 'A'
		CHECK(is_xid_start(0x5A));                    // 'Z'
		CHECK(is_xid_start(0x61));                    // 'a'
		CHECK(is_xid_start(0x7A));                    // 'z'
		CHECK(is_xid_start(0xD8));                    // 'Ø', a non-ascii letterc
		CHECK(is_xid_start(decode_codepoint("ℹ"sv))); // ℹ U+2139  INFORMATION SOURCE
		CHECK(is_xid_start(decode_codepoint("п")));   // 'п' U+043F CYRILLIC SMALL LETTER PE


		CHECK_FALSE(is_xid_start(0x30));              // '0'
		CHECK_FALSE(is_xid_start(0x20));              // Space
		CHECK_FALSE(is_xid_start(0x7F));              // DEL

		CHECK(is_xid_start(max_xid_start));
		CHECK_FALSE(is_xid_start(max_xid_start + 1));

		CHECK(is_xid_start(0x370));
		CHECK_FALSE(is_xid_start(0x2C2));

		CHECK(is_xid_start(0x3'3479)); // U+33479, a CJK ideograph
		CHECK_FALSE(is_xid_start(0x3'3480));
	}

	SECTION("compare string with view")
	{
		utf8::string str("hello");

		CHECK(str == utf8::view("hello"sv));
		CHECK(str != utf8::view("world"sv));
	}

	SECTION("contains view")
	{
		utf8::string str("hello world");

		CHECK(str.contains(utf8::view("hello"sv)));
		CHECK(str.contains(utf8::view("world"sv)));
		CHECK(str.contains(utf8::view(" "sv)));
		CHECK_FALSE(str.contains(utf8::view("xyz"sv)));
		CHECK_FALSE(str.contains(utf8::view("HELLO"sv)));

		utf8::string unicode("こんにちは世界");
		CHECK(unicode.contains(utf8::view("にちは"sv)));
		CHECK_FALSE(unicode.contains(utf8::view("hello"sv)));
	}

	SECTION("starts_with view")
	{
		utf8::string str("hello world");

		CHECK(str.starts_with(utf8::view("hello"sv)));
		CHECK(str.starts_with(utf8::view("hello world"sv)));
		CHECK_FALSE(str.starts_with(utf8::view("world"sv)));
		CHECK_FALSE(str.starts_with(utf8::view("HELLO"sv)));

		utf8::string unicode("こんにちは");
		CHECK(unicode.starts_with(utf8::view("こんに"sv)));
		CHECK_FALSE(unicode.starts_with(utf8::view("にちは"sv)));
	}

	SECTION("ends_with view")
	{
		utf8::string str("hello world");

		CHECK(str.ends_with(utf8::view("world"sv)));
		CHECK(str.ends_with(utf8::view("hello world"sv)));
		CHECK_FALSE(str.ends_with(utf8::view("hello"sv)));
		CHECK_FALSE(str.ends_with(utf8::view("WORLD"sv)));

		utf8::string unicode("こんにちは");
		CHECK(unicode.ends_with(utf8::view("にちは"sv)));
		CHECK_FALSE(unicode.ends_with(utf8::view("こんに"sv)));
	}

	SECTION("operator+ view")
	{
		utf8::string str("hello");

		auto result = str + utf8::view(" world"sv);
		CHECK(result == "hello world"sv);

		utf8::string unicode("こんにちは");
		auto         result2 = unicode + utf8::view("世界"sv);
		CHECK(result2 == "こんにちは世界"sv);
	}

	SECTION("operator+= view")
	{
		utf8::string str("hello");
		str += utf8::view(" world"sv);
		CHECK(str == "hello world"sv);

		utf8::string unicode("こんにちは");
		unicode += utf8::view("世界"sv);
		CHECK(unicode == "こんにちは世界"sv);
	}

	SECTION("short string in a vector")
	{
		std::vector<utf8::string> valid_strings = {
		  "hello"sv,
		  "こんにちは"sv,
		  "😀"sv,
		  "éàü"sv,
		  "data\0with\0nul"sv,
		  "\xF0\x9F\x98\x80"sv, // 😀 U+1F600
		};

		CHECK(valid_strings.size() == 6);
		CHECK(valid_strings[0] == utf8::string("hello"sv));
		CHECK(valid_strings[1] == utf8::string("こんにちは"sv));
		CHECK(valid_strings[2] == utf8::string("😀"sv));
		CHECK(valid_strings[3] == utf8::string("éàü"sv));
		CHECK(valid_strings[4] == utf8::string("data\0with\0nul"sv));
		CHECK(valid_strings[5] == utf8::string("\xF0\x9F\x98\x80"sv));
	}

	SECTION("short in a set")
	{
		std::unordered_set<utf8::string> set;

		set.insert("hello"sv);
		set.insert("world"sv);

		CHECK(set.size() == 2);
		CHECK(set.contains("hello"sv));
		CHECK_FALSE(set.contains("qwerty"sv));
	}

	SECTION("long string in vector")
	{
		utf8::string              str("a long string, it is not an SSO buffer, lorem ipsum 42");
		std::vector<utf8::string> lv;
		lv.push_back(str);

		CHECK(lv.size() == 1);
		CHECK(lv[0] == str);

		lv.clear();
		CHECK(lv.empty());

		str += str;
		str += str;
		lv.push_back(str);
		CHECK(lv.size() == 1);
		CHECK(lv[0] == str);
	}

	SECTION("long string in set")
	{
		std::unordered_set<utf8::string> set;
		utf8::string                     long_str("a long string, it is not an SSO buffer, lorem ipsum 42");
		set.insert(long_str);
		CHECK(set.size() == 1);
		CHECK(set.contains(long_str));
		long_str += long_str;
		long_str += long_str;
		set.insert(long_str);
		CHECK(set.size() == 2);
		CHECK(set.contains(long_str));
	}

	SECTION("long string in map")
	{
		std::unordered_map<utf8::string, int> map;
		utf8::string                          long_str("a long string, it is not an SSO buffer, lorem ipsum 42");
		map[long_str] = 42;
		CHECK(map.size() == 1);
		CHECK(map.contains(long_str));
		CHECK(map[long_str] == 42);
	}

	SECTION("security checks")
	{
		utf8::string latin_d("data");    // latin d - data
		utf8::string cyrillic_d("ԁata"); // cyrillic d - ԁata

		CHECK(latin_d != cyrillic_d);

		utf8::string data_string("data");           // normal data
		utf8::string rlo_data_string("da\u202Eta"); // data with right-to-left override
		CHECK(rlo_data_string.length() == 5);

		CHECK(data_string != rlo_data_string);
	}

	SECTION("grapheme cluster")
	{
		// https://hsivonen.fi/string-length/
		utf8::string man("🤦🏼‍♂️");
		// U + 1F926  🤦 FACE PALM
		// U + 1F3FC  🏼 skin tone modifier(medium - light)
		// U + 200D ZWJ
		// U + 2642   ♂ male sign
		// U + FE0F variation selector - 16
		CHECK(man.graphemes() == 1);
		CHECK(man.length() == 5);
		CHECK(man.size_in_bytes() == 17);

		utf8::string family("👨‍👩‍👧‍👦");
		// U + 1F468
		// U + 200D ZWJ
		// U + 1F469
		// U + 200D
		// U + 1F467
		// U + 200D
		// U + 1F466


		CHECK(family.length() == 7);
		CHECK(family.graphemes() == 1);

		utf8::string longer("👩‍❤️‍💋‍👩");
		CHECK(longer.graphemes() == 1);
		CHECK(longer.length() == 8);


		utf8::string combined("a👩‍❤️‍💋‍👩b");
		CHECK(combined.length() == 10);
		CHECK(combined.graphemes() == 3);

		utf8::string sentence_with_clusters("hello "); //  6 - 6
		sentence_with_clusters.append(man);            //  1 - 5
		sentence_with_clusters.append(" world ");      //  7 - 7
		sentence_with_clusters.append(family);         //  1 - 7
		sentence_with_clusters.append(longer);         //  1 - 8
													   // 16 - 33

		CHECK(sentence_with_clusters.graphemes() == 16);
		CHECK(sentence_with_clusters.size() == 33);
	}
}

TEST_CASE("view", "[utf8][view]")
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

	SECTION("contains")
	{
		utf8::string str("🌍hello😊");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		CHECK(w.contains("hello"sv));
		CHECK(w.contains("😊"sv));
		CHECK(w.contains("🌍"sv));
		CHECK_FALSE(w.contains("world"sv));
	}

	SECTION("starts_with")
	{
		utf8::string str("🌍hello😊");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		CHECK(w.starts_with("🌍"sv));
		CHECK_FALSE(w.starts_with("😊"sv));
	}

	SECTION("starts_with string")
	{
		utf8::string str("🌍hello😊");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		CHECK(w.starts_with("🌍he"sv));
		CHECK_FALSE(w.starts_with("😊it"sv));
	}


	SECTION("ends_with")
	{
		utf8::string str("🌍hello😊");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		CHECK(w.ends_with("😊"sv));
		CHECK_FALSE(w.ends_with("🌍"sv));
	}

	SECTION("ends_with string")
	{
		utf8::string str("🌍hello😊");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		CHECK(w.ends_with("lo😊"sv));
		CHECK_FALSE(w.ends_with("he"sv));
	}

	SECTION("string find from span")
	{
		utf8::string      str("🌍hello🌍");
		std::array<u8, 5> bytes = {'h', 'e', 'l', 'l', 'o'};

		auto f = str.find({bytes.data(), bytes.size()});
		CHECK(f == 1);
	}


	SECTION("view trim")
	{
		utf8::string str("  hello 🌍 world  ");
		utf8::view   v(str.data());

		auto tl = v.trim_left(); // "hello 🌍 world  "
		CHECK(tl.size() == 15);
		CHECK(tl[0] == 'h');
		CHECK(tl[6] == 0x1'f30d);

		auto tr = v.trim_right(); // "  hello 🌍 world"
		CHECK(tr.size() == 15);
		CHECK(tr[0] == ' ');
		CHECK(tr[14] == 'd');

		auto t = v.trim(); // "hello 🌍 world"
		CHECK(t.size() == 13);
		CHECK(t[0] == 'h');
		CHECK(t[6] == 0x1'f30d);
		CHECK(t[12] == 'd');
	}

	SECTION("view starts_with / ends_with")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		CHECK(v.starts_with("hello"sv));
		CHECK_FALSE(v.starts_with("world"sv));
		CHECK(v.ends_with("world"sv));
		CHECK_FALSE(v.ends_with("hello"sv));
	}

	SECTION("view contains")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		CHECK(v.contains("hello"sv));
		CHECK(v.contains("🌍"sv));
		CHECK(v.contains("world"sv));
		CHECK_FALSE(v.contains("test"sv));
	}

	SECTION("empty view")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str, 0);
		CHECK(v.empty());
		CHECK(v.size() == 0);
		CHECK(v.valid());
		CHECK_FALSE(v.starts_with("hello"sv));
		CHECK_FALSE(v.ends_with("world"sv));
		CHECK_FALSE(v.contains("hello"sv));
	}

	SECTION("view from view")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		utf8::view   sub = v.subview(0, 5);
		CHECK(sub.size() == 5);
		CHECK(sub[0] == 'h');
		CHECK(sub[4] == 'o');
		CHECK(sub.valid());
	}

	SECTION("view assign")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		utf8::view   sub = v.subview(0, 5);
		CHECK(sub.size() == 5);
		CHECK(sub[0] == 'h');
		CHECK(sub[4] == 'o');
		CHECK(sub.valid());
		sub = v.subview(6, 1);
		CHECK(sub.size() == 1);
		CHECK(sub[0] == 0x1'f30d);
		CHECK(sub.valid());
	}

	SECTION("ascii - operator- returns byte offset")
	{
		utf8::string str  = "hello"sv;
		utf8::view   full = str;
		utf8::view   rest = full;
		rest.remove_prefix(3);

		CHECK((rest - full) == 3uz);
	}

	SECTION("multibyte - operator- returns bytes not codepoints")
	{
		utf8::string str  = "café"sv;
		utf8::view   full = str;
		utf8::view   rest = full;
		rest.remove_prefix(3);

		CHECK((rest - full) == 3uz);

		rest.remove_prefix(1);
		CHECK((rest - full) == 5uz);
	}

	SECTION("multibyte - two-byte codepoints at start")
	{
		utf8::string str  = "éàü"sv;
		utf8::view   full = str;
		utf8::view   rest = full;
		rest.remove_prefix(1);

		CHECK((rest - full) == 2uz);

		rest.remove_prefix(1);
		CHECK((rest - full) == 4uz);

		rest.remove_prefix(1);
		CHECK((rest - full) == 6uz);
	}

	SECTION("view find")
	{
		utf8::string str("hello 🌍 world"sv);
		utf8::view   v(str.data());
		auto         pos = v.find("🌍"sv);
		CHECK(pos == 6);

		pos = v.find("world"sv);
		CHECK(pos == 8);
	}

	SECTION("view find not found")
	{
		utf8::string str("hello 🌍 world"sv);
		utf8::view   v(str.data());
		auto         pos = v.find("test"sv);
		CHECK(pos == utf8::view::npos);
	}

	SECTION("view rfind")
	{
		utf8::string str("hello 🌍 world 🌍"sv);
		utf8::view   v(str.data());
		auto         pos = v.rfind("🌍"sv);
		CHECK(pos == 14);

		pos = v.rfind("world"sv);
		CHECK(pos == 8);
	}

	SECTION("view iterator operator []")
	{
		utf8::string str("hello 🌍 world"sv);
		utf8::view   v(str.data());
		CHECK(v[0] == 'h');
		CHECK(v[6] == 0x1'f30d);
		CHECK(v[12] == 'd');
	}


	SECTION("view from subspan")
	{
		utf8::string str("hello 🌍 world"sv);
		utf8::view   v(str, 7);
		CHECK(v.size() == 7);
		CHECK(v[0] == 'h');
		CHECK(v[6] == 0x1'f30d);
		CHECK(v.valid());
	}

	SECTION("view, sub_str")
	{
		utf8::string str("hello 🌍 world"sv);
		utf8::view   v(str);

		utf8::string sub = v.sub_str(0, 5);
		CHECK(sub == "hello");
		CHECK(sub.size() == 5);

		utf8::string sub2 = v.sub_str(6, 1);
		CHECK(sub2 == "🌍");
		CHECK(sub2.size() == 1);
	}

	SECTION("view from subspan offset")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str, 6, 3);
		CHECK(v.size() == 3);
		CHECK(v[0] == U'🌍');
		CHECK(v[1] == U' ');
		CHECK(v[2] == U'w');

		CHECK(v.valid());
	}

	SECTION("invalid view")
	{
		std::array<u8, 2> err = {0xC3, 0x28};
		utf8::view        v(err);
		CHECK(v.size() == 0);
		CHECK(v.size_in_bytes() == 2);
		CHECK_FALSE(v.valid());
	}

	SECTION("at view")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		CHECK(v.at(0) == 'h');
		CHECK(v.at(6) == 0x1'f30d);
		CHECK(v.at(12) == 'd');
	}

	SECTION("view front / back")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		CHECK(v.front() == 'h');
		CHECK(v.back() == 'd');
	}

	SECTION("view iterator")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		auto         it = v.begin();
		CHECK(*it == 'h');
		++it;
		CHECK(*it == 'e');
		it += 5;
		CHECK(*it == 0x1'f30d);
		it += 6;
		CHECK(*it == 'd');
	}

	SECTION("view reverse iterator")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		auto         it = v.rbegin();
		CHECK(*it == 'd');
		++it;
		CHECK(*it == 'l');
		it += 5;
		CHECK(*it == 0x1'f30d);
		it += 6;
		CHECK(*it == 'h');
	}

	SECTION("view iterator += / -=")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		auto         it = v.begin();
		CHECK(*it == 'h');
		it += 6;
		CHECK(*it == 0x1'f30d);
		it -= 6;
		CHECK(*it == 'h');
	}

	SECTION("view iterator distance")
	{
		utf8::string str("hello 🌍 world");
		utf8::view   v(str.data());
		auto         it1 = v.begin();
		auto         it2 = v.end();
		CHECK(std::distance(it1, it2) == 13);
	}
}

TEST_CASE("scanner", "[utf8][scanner]")
{
	SECTION("scanner")
	{
		utf8::string  str("🌍hello🌍");
		utf8::scanner w{str};

		CHECK(w.size() == 7);

		CHECK(w.next() == U'🌍');
		CHECK(w.next() == 'h');
		CHECK(w.next() == 'e');
		CHECK(w.next() == 'l');
		CHECK(w.next() == 'l');
		CHECK(w.next() == 'o');
		CHECK(w.next() == U'🌍'); // w is now at the end

		w--;
		CHECK(*w == U'🌍');


		w--; // o
		w--; // l
		w--; // l
		w--; // e
		w--; // h
		CHECK(*w == 'h');


		w += 5;
		CHECK(*w == U'🌍');

		w -= 6;
		CHECK(*w == U'🌍');

		w += 3;
		CHECK(*w == 'l');
		w -= 1;
		CHECK(*w == 'e');
	}

	SECTION("scanner out of bounds")
	{
		utf8::string  str("hello");
		utf8::scanner w{str};
		CHECK(w.next() == 'h');
		CHECK(w.next() == 'e');
		CHECK(w.next() == 'l');
		CHECK(w.next() == 'l');
		CHECK(w.next() == 'o');
		CHECK_FALSE(w.has_next());
	}

	SECTION("scanner take")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(utf8::view("hello"sv) == w.take(5));
		w.skip_whitespace();
		CHECK(utf8::view("world"sv) == w.take(5));
	}

	SECTION("scanner take until")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(utf8::view("hello"sv) == w.take_until(' '));
		CHECK(w.next() == ' ');
		CHECK(utf8::view("world"sv) == w.take_until(' '));
	}

	SECTION("scanner take while")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(utf8::view("hello"sv) == w.take_while([](char32 c) { return utf8::is_ascii_alphanumeric(c); }));
		CHECK(w.next() == ' ');
		CHECK(utf8::view("world"sv) == w.take_while([](char32 c) { return utf8::is_ascii_alphanumeric(c); }));
	}

	SECTION("scanner take while with multibyte")
	{
		utf8::string  str("hello 🌍");
		utf8::scanner w{str};
		CHECK(utf8::view("hello"sv) == w.take_while([](char32 c) { return utf8::is_ascii_alphanumeric(c); }));
		CHECK(w.next() == ' ');
		CHECK(utf8::view("🌍"sv) == w.take_while([](char32 c) { return not utf8::is_ascii(c); }));
	}

	SECTION("scanner take line")
	{
		utf8::string  str("hello world\nthis is a test");
		utf8::scanner w{str};
		CHECK(utf8::view("hello world"sv) == w.take_line());
		CHECK(utf8::view("this is a test"sv) == w.take_line());
		CHECK_FALSE(w.has_next());
	}

	SECTION("scanner remaining")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(w.next() == 'h');
		CHECK(w.next() == 'e');
		CHECK(utf8::view("llo world"sv) == w.remaining_view());
		CHECK(w.remaining() == 9);
	}

	SECTION("scanner full view")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(utf8::view("hello world"sv) == w.full_view());
	}

	SECTION("scanner expect char")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(w.expect('h'));
		CHECK(w.expect('e'));
		CHECK_FALSE(w.expect('x'));
	}

	SECTION("scanner expect string")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(w.expect("hello"sv));
		w.skip_whitespace();
		CHECK(w.expect("world"sv));
		CHECK_FALSE(w.expect("!"sv));
	}

	SECTION("scanner expect string with multibyte")
	{
		utf8::string  str("hello 🌍");
		utf8::scanner w{str};
		CHECK(w.expect("hello"sv));
		w.skip_whitespace();
		CHECK(w.expect("🌍"sv));
		CHECK_FALSE(w.expect("!"sv));
	}

	SECTION("scanner skip")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(w.skip('h'));
		CHECK(w.skip('e'));
		CHECK_FALSE(w.skip('x'));
		CHECK(w.skip_while([](char32 c) { return utf8::is_ascii_alphanumeric(c); }));
		CHECK(w.next() == ' ');
		CHECK(w.skip_while([](char32 c) { return utf8::is_ascii_alphanumeric(c); }));
		CHECK_FALSE(w.has_next());
	}

	SECTION("scanner skip with multibyte")
	{
		utf8::string  str("hello 🌍");
		utf8::scanner w{str};
		CHECK(w.skip('h'));
		CHECK(w.skip('e'));
		CHECK_FALSE(w.skip('x'));
		CHECK(w.skip_while([](char32 c) { return utf8::is_ascii_alphanumeric(c); }));
		CHECK(w.next() == ' ');
		CHECK(w.skip_while([](char32 c) { return not utf8::is_ascii(c); }));
		CHECK_FALSE(w.has_next());
	}

	SECTION("scanner starts_with")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(w.starts_with("hello"sv));
		CHECK_FALSE(w.starts_with("world"sv));
	}

	SECTION("scanner starts_with with multibyte")
	{
		utf8::string  str("hello 🌍");
		utf8::scanner w{str};
		CHECK(w.starts_with("hello"sv));
		CHECK(w.starts_with("hello 🌍"sv));
		CHECK_FALSE(w.starts_with("world"sv));
	}

	SECTION("scanner ends_with")
	{
		utf8::string  str("hello world");
		utf8::scanner w{str};
		CHECK(w.ends_with("world"sv));
		CHECK_FALSE(w.ends_with("hello"sv));
	}

	SECTION("scanner ends_with with multibyte")
	{
		utf8::string  str("hello 🌍");
		utf8::scanner w{str};
		CHECK(w.ends_with("🌍"sv));
		CHECK(w.ends_with("hello 🌍"sv));
		CHECK_FALSE(w.ends_with("world"sv));
	}

	SECTION("scanner out of bounds on expect/skip")
	{
		utf8::string  str("hi");
		utf8::scanner w{str};
		CHECK(w.expect('h'));
		CHECK(w.expect('i'));
		CHECK_FALSE(w.expect('x'));
		CHECK_FALSE(w.skip('x'));
	}

	SECTION("scanner out of bounds on take")
	{
		utf8::string  str("hi");
		utf8::scanner w{str};
		CHECK(w.take(1) == "h"sv);
		CHECK(w.take(1) == "i"sv);
		CHECK(w.take(1) == ""sv);
	}

	SECTION("scanner peek / peek_back")
	{
		utf8::string  str("hello 🌍");
		utf8::scanner w{str};
		CHECK(w.peek(0) == 'h');
		CHECK(w.expect("hello"sv));
		CHECK(w.peek(0) == ' ');
		CHECK(w.peek_back() == 'o');
	}
}
