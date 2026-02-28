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
			std::array<std::byte, 3> buf{std::byte{0xF0}, std::byte{0x9F}, std::byte{0x98}}; // truncated 4-byte sequence
			std::vector<char32>      cps;
			for (auto cp : utf8::yield_codepoints(buf))
				cps.push_back(cp);
			REQUIRE(cps.size() == 1);
			CHECK(cps[0] == utf8::REPLACEMENT_CHARACTER);
		}

		{
			std::array<std::byte, 2> buf{std::byte{0x80}, std::byte{0x80}}; // two invalid standalone continuation bytes
			std::vector<char32>      cps;
			for (auto cp : utf8::yield_codepoints(buf))
				cps.push_back(cp);
			REQUIRE(cps.size() == 2);
			CHECK(cps[0] == utf8::REPLACEMENT_CHARACTER);
			CHECK(cps[1] == utf8::REPLACEMENT_CHARACTER);
		}
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

		CHECK(std::string(str.c_str()) == "hello world"sv);


		str = "\xF0\x9F\xA4\xA6\xF0\x9F\x8F\xBC\xE2\x80\x8D\xE2\x99\x82\xEF\xB8\x8F";
		CHECK(str.length() == 5);
		CHECK(str.size_in_bytes() == 17);
		// CHECK(str.count() == 2); // TODO: should be 1

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
		CHECK(str.c_str() == std::string_view("hello"));


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

	SECTION("subview(start_codepoint, count)")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   w(str.data());
		CHECK(w.size() == 7);

		auto v = w.subview(1, 5);
		CHECK(v.size() == 5);
		CHECK(v[0] == 'h');
		CHECK(v[1] == 'e');
		CHECK(v[2] == 'l');
		CHECK(v[3] == 'l');
		CHECK(v[4] == 'o');

		auto first = w.subview(0, 1);
		CHECK(first.size() == 1);
		CHECK(first[0] == 0x1'f30d);

		auto last = w.subview(6, 1);
		CHECK(last.size() == 1);
		CHECK(last[0] == 0x1'f30d);

		auto clamped = w.subview(6, 99);
		CHECK(clamped.size() == 1);
		CHECK(clamped[0] == 0x1'f30d);

		auto empty = w.subview(7, 1);
		CHECK(empty.size() == 0);
	}

	SECTION("subview(count) — from current position")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   w(str.data());

		++w;
		auto v = w.subview(5);
		CHECK(v.size() == 5);
		CHECK(v[0] == 'h');
		CHECK(v[4] == 'o');

		utf8::view w2(str.data());
		auto all = w2.subview(7);
		CHECK(all.size() == 7);
		CHECK(all[0] == 0x1'f30d);
		CHECK(all[6] == 0x1'f30d);
	}

	SECTION("subview(view start, count)")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   root(str.data());
		CHECK(root.size() == 7);

		utf8::view cursor = root;
		++cursor; 
		auto v = root.subview(cursor, 5);
		CHECK(v.size() == 5);
		CHECK(v[0] == 'h');
		CHECK(v[4] == 'o');

		utf8::view cursor2 = root;
		auto first3 = root.subview(cursor2, 3);
		CHECK(first3.size() == 3);
		CHECK(first3[0] == 0x1'f30d);
		CHECK(first3[1] == 'h');
		CHECK(first3[2] == 'e');
	}

	SECTION("subview_bytes(start_byte, byte_length)")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   w(str.data());

		auto earth = w.subview_bytes(0, 4);
		CHECK(earth.size() == 1);
		CHECK(earth[0] == 0x1'f30d);

		auto hell = w.subview_bytes(4, 4);
		CHECK(hell.size() == 4);
		CHECK(hell[0] == 'h');
		CHECK(hell[3] == 'l');

		auto last = w.subview_bytes(9, 4);
		CHECK(last.size() == 1);
		CHECK(last[0] == 0x1'f30d);
	}

	SECTION("subview_bytes(view start, byte_length)")
	{
		utf8::string str("🌍hello🌍");
		utf8::view   root(str.data());

		utf8::view cursor = root;
		auto earth = root.subview_bytes(cursor, 4);
		CHECK(earth.size() == 1);
		CHECK(earth[0] == 0x1'f30d);

		++cursor;
		auto hello = root.subview_bytes(cursor, 5);
		CHECK(hello.size() == 5);
		CHECK(hello[0] == 'h');
		CHECK(hello[4] == 'o');
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
		utf8::string w("🌍");
		auto         found = str.find_first_of(w);
		CHECK(found == 0);

		found = str.find_first_of("w"sv);
		CHECK(found == 8);


		w     = "d";
		found = str.find_first_of(w.subview());
		CHECK(found == 12);


		char32 q = 0x274c; // ❌
		found    = str.find_first_of(q);
		CHECK(found == 6);

		char a = 'e';
		found  = str.find_first_of(a);
		CHECK(found == 2);
	}

	SECTION("find_first_not_of")
	{
		utf8::string str("🌍hello❌ world🌍");
		utf8::string w("🌍❌oleh ");

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
		utf8::string w("i12");

		auto found = str.find_last_of(w);
		CHECK(found == 8);

		utf8::string str2("12 ❌q2 12345");
		utf8::string w2("❌q2");

		found = str2.find_last_of(w2);
		CHECK(found == 8);

		char32 q = 0x274c; // ❌
		CHECK(str2.find_last_of(q) == 3);

		CHECK(str2.find_last_of('q') == 4);
		// found = str2.find_last_of("q2"sv);
		// CHECK(found == 5);
	}

	SECTION("find_last_not_of")
	{
		utf8::string str("🌍hello❌ world🌍");
		utf8::string w("🌍❌helord ");

		auto found = str.find_last_not_of(w);
		CHECK(found == 8);

		found = str.find_last_not_of("hello world"sv);
		CHECK(found == 13);

		char32 q = 0x1'f30d; // 🌍
		found    = str.find_last_not_of(q);
		CHECK(found == 12);
	}

	SECTION("rfind")
	{
		//
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
		utf8::string str("  🌍23❌  ");
		CHECK(str.size() == 8);
		CHECK(str == "  🌍23❌  ");
		str.trim();
		CHECK(str.size() == 4);
		CHECK(str == "🌍23❌");
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
		CHECK(err.error().contains("index 1") == true);
	}
}
