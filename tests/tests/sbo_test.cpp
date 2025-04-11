#include <catch2/catch_test_macros.hpp>


import std;
import deckard.types;
import deckard.sbo;
import deckard.as;
import deckard.helpers;
import deckard.debug;
using namespace std::string_view_literals;

using namespace deckard;

TEST_CASE("sbo", "[sbo]")
{
	SECTION("constructor (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(sizeof(ss) == 32);
	}

	SECTION("initializer list (small)")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(sizeof(ss) == 32);


		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss[2] == 'C');
		CHECK(ss[3] == 'D');
		CHECK(ss[4] == 'E');
		CHECK(ss[5] == 'F');
	}

	SECTION("initializer list (large)")
	{
		sbo<32> ss{
		  '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'Q', 'W', 'E', 'R',
		  '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'Q', 'W', 'E', 'R',
		};

		CHECK(ss.size() == 48);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		CHECK(sizeof(ss) == 32);


		CHECK(ss[0] == '1');
		CHECK(ss[1] == '2');
		CHECK(ss[2] == '3');
		CHECK(ss[3] == '4');
		CHECK(ss[4] == '5');
		CHECK(ss[5] == '6');

		CHECK(ss[42] == 'K');
		CHECK(ss[43] == 'L');
		CHECK(ss[44] == 'Q');
		CHECK(ss[45] == 'W');
		CHECK(ss[46] == 'E');
		CHECK(ss[47] == 'R');
	}

	SECTION("move c-tor (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');

		sbo<32> copy(std::move(ss));

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 31);
		CHECK(copy[0] == 'A');

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
	}

	SECTION("move assign self (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');

		ss = std::move(ss);

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');
	}

	SECTION("move assignment (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');

		sbo<32> copy;

		copy = std::move(ss);

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 31);
		CHECK(copy[0] == 'A');

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
	}

	SECTION("move c-tor (large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		ss.resize(128);

		sbo<32> copy(std::move(ss));

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 128);
		CHECK(copy[0] == 'A');

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
	}

	SECTION("move assignment (large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		ss.resize(128);

		sbo<32> copy;
		copy = std::move(ss);

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 128);
		CHECK(copy[0] == 'A');

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
	}

	SECTION("move assignment self (large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		ss.resize(128);

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 128);

		ss = std::move(ss);

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 128);
	}

	SECTION("copy c-tor (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');

		sbo<32> copy(ss);

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 31);
		CHECK(copy[0] == 'A');
	}

	SECTION("copy assign (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');

		sbo<32> copy;
		copy = ss;

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 31);
		CHECK(copy[0] == 'A');
	}


	SECTION("copy c-tor (large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		ss.resize(128);

		sbo<32> copy(ss);

		CHECK(copy.size() == 6);
		CHECK(copy.capacity() == 128);
		CHECK(copy[0] == 'A');
	}


	SECTION("indexing (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);


		CHECK(ss[0] == 0);
		CHECK(ss.size() == 0);
	}


	SECTION("push_back (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('Q');

		CHECK(ss[0] == 'Q');
		CHECK(ss.size() == 1);

		ss.push_back('B');
		CHECK(ss[1] == 'B');
		CHECK(ss.size() == 2);
	}

	SECTION("push_back (small -> large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
		CHECK(sizeof ss == 32);


		repeat<64> = [&] { ss.push_back('A'); };

		CHECK(ss[0] == 'A');
		CHECK(ss.size() == 64);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);


		ss.push_back('B');
		CHECK(ss.size() == 65);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	// popback
	SECTION("pop_back (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');


		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss.size() == 2);
		CHECK(ss.capacity() == 31);

		ss.pop_back();
		CHECK(ss[0] == 'A');
		CHECK(ss.size() == 1);
		CHECK(ss.capacity() == 31);
	}

	SECTION("pop_back (large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		repeat<65> = [&] { ss.push_back('A'); };


		CHECK(ss[0] == 'A');
		CHECK(ss.size() == 65);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		repeat<55> = [&] { ss.pop_back(); };

		CHECK(ss[0] == 'A');
		CHECK(ss.size() == 10);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);


		ss.shrink_to_fit();

		CHECK(ss.size() == 10);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
	}

	SECTION("resize (small -> large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');

		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss.size() == 2);
		CHECK(ss.capacity() == 31);

		ss.pop_back();
		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss.size() == 1);
		CHECK(ss.capacity() == 31);
	}


	SECTION("resize (larger -> large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.resize(128);
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.resize(40);
		CHECK(ss.size() == 40);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("resize (large -> small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);


		ss.resize(128);
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);


		ss.resize(15);
		CHECK(ss.size() == 15);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("clear (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.push_back('A');
		CHECK(ss.size() == 1);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.clear();
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
	}


	SECTION("shrink_to_fit (small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.shrink_to_fit();
		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
	}


	SECTION("shrink_to_fit (large -> small)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		ss.resize(128);

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
		CHECK(ss[0] == 'A');


		ss.shrink_to_fit();
		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
		CHECK(ss[0] == 'A');
	}

	SECTION("append (small, small -> large, large -> large)")
	{
		std::array<u8, 8> arr{1, 2, 3, 4, 5, 6, 7, 8};

		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.append(arr);

		ss.append('X');

		CHECK(ss.size() == 9);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.append(arr);

		CHECK(ss.size() == 17);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.append(ss.data());

		CHECK(ss.size() == 34);
		CHECK(ss.capacity() == 48);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append(ss.data());

		CHECK(ss.size() == 68);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append(arr);

		CHECK(ss.size() == 76);
		CHECK(ss.capacity() == 112);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append({'1', '2', '3'});
		CHECK(ss.size() == 79);
		CHECK(ss.back() == '3');
		CHECK(ss.capacity() == 112);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("append (large -> larger)")
	{
		sbo<32> ss;

		ss.reserve(128);
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 128);

		std::vector<u8> buffer;

		for (const auto& i : upto(128))
		{
			ss.push_back(as<u8>(i + '0'));
			buffer.push_back('B');
			buffer.push_back('C');
			buffer.push_back('D');
			buffer.push_back('E');
		}

		CHECK(buffer.size() == 128 * 4);

		CHECK(ss.size() == 128);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append(buffer);


		CHECK(ss.size() == 128 + 512);
		CHECK(ss.capacity() == 128 + 512);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("operator + (small)")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		ss += ss;

		CHECK(ss.size() == 12);
		CHECK(ss.capacity() == 31);

		auto ss2 = ss + ss;

		CHECK(ss2.size() == 24);
		CHECK(ss2.capacity() == 31);

		CHECK(ss.size() == 12);
		CHECK(ss.capacity() == 31);
	}


	SECTION("operator + (large)")
	{
		sbo<32> ss;

		ss.resize(128);
		ss.fill('A');

		CHECK(ss.size() == 128);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss += ss;


		CHECK(ss.size() == 128 + 128);
		CHECK(ss.capacity() == 128 + 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("clear (large)")
	{
		sbo<32> ss;
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.push_back('A');
		ss.resize(128);

		CHECK(ss.size() == 1);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.clear();
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("push_back (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('Q');

		CHECK(ss[0] == 'Q');
		CHECK(ss.size() == 1);

		ss.push_back('B');
		CHECK(ss[1] == 'B');
		CHECK(ss.size() == 2);
	}

	SECTION("front / back (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);


		for (u32 i = 0; i < ss.capacity(); i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.front() == 0);
		CHECK(ss.back() == 30);

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
	}

	SECTION("reserve (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.reserve(20);

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);
	}

	SECTION("reserve (small -> large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.reserve(128);

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 128);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.reserve(256);
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 256);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.reserve(128);
		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 256);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}


	SECTION("front / back (large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.reserve(256);

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 256);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < 256; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 256);
		CHECK(ss.capacity() == 256);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < 256; i++)
			CHECK(ss[i] == i);

		CHECK(ss.front() == 0x00);
		CHECK(ss.back() == 0xFF);
	}
	SECTION("do all the things")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		// push_back to small
		for (u32 i = 0; i < 31; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < ss.size(); i++)
			CHECK(ss[i] == i);

		// push small -> large

		ss.push_back(as<u8>(31));

		CHECK(ss.size() == 32);
		CHECK(ss.capacity() == 48);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < ss.size(); i++)
			CHECK(ss[i] == i);

		// pop large -> large
		ss.pop_back();

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 48);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < ss.size(); i++)
			CHECK(ss[i] == i);


		// shrink to small
		ss.shrink_to_fit();

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);


		// append back to large
		std::vector<u8> v;

		for (u32 i = 0; i < 4096; i++)
			v.emplace_back(i % 256);

		ss.append(v);
		CHECK(ss.size() == 31 + 4096);
		CHECK(ss.capacity() == 31 + 4096);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
		CHECK(ss[2048] == 225);

		//
		ss.resize(1024);
		ss.resize(ss.capacity());

		CHECK(ss.size() == 31 + 4096);
		CHECK(ss.capacity() == 31 + 4096);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
		CHECK(ss[2048] == 0);

		ss.resize(4096);
		CHECK(ss.size() == 4096);
		CHECK(ss.capacity() == 4096 + 31);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append(ss);
		CHECK(ss.size() == 4096 * 2);
		CHECK(ss.capacity() == 8192);
		CHECK(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("iterators (small)")
	{
		sbo<32> ss;

		std::vector<u8> correct;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 31; i++)
		{
			correct.push_back(as<u8>(i));
			ss.push_back(as<u8>(i));
		}

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);


		u8 i = 0;
		for (auto it = ss.begin(); it != ss.end(); it++)
			CHECK(*it == i++);

		CHECK(std::distance(ss.begin(), ss.end()) == 31);


		i = 0;
		for (const auto& c : ss)
			CHECK(c == i++);


		auto it = ss.begin();
		CHECK(sizeof(it) == 8);
		CHECK(*it == 0);

		it++;
		CHECK(*it == 1);

		it--;
		CHECK(*it == 0);

		it += 5;
		CHECK(*it == 5);

		it -= 5;
		CHECK(*it == 0);

		i = 0;
		std::ranges::reverse(correct);
		for (auto it2 = ss.rbegin(); it2 != ss.rend(); it2++)
			CHECK(correct[i++] == *it2);

		CHECK(i == ss.size());
		CHECK(std::distance(ss.rbegin(), ss.rend()) == 31);
	}

	SECTION("iterators (large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 128; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 128);
		CHECK(ss.capacity() == 168);
		CHECK(ss.max_size() == 0xFFFF'FFFF);


		u8 i = 0;
		for (auto it = ss.begin(); it != ss.end(); it++)
			CHECK(*it == i++);

		CHECK(std::distance(ss.begin(), ss.end()) == 128);

		i = 0;
		for (const auto& c : ss)
			CHECK(c == i++);


		auto it = ss.begin();
		CHECK(*it == 0);

		it++;
		CHECK(*it == 1);

		it--;
		CHECK(*it == 0);

		it += 100;
		CHECK(*it == 100);
		CHECK(it == ss.begin() + 100);


		it -= 100;
		CHECK(*it == 0);

		CHECK(it == ss.begin());

		it += 50;
		CHECK(it == ss.begin() + 50);

		CHECK(*it + 5 == 55);
		CHECK(*it - 5 == 45);

		CHECK(it == ss.begin() + 50);

		CHECK(ss.begin() < ss.end());

		CHECK(it < ss.end());

		CHECK(ss.end() > ss.begin());
	}

	SECTION("insert (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');

		CHECK(ss.size() == 4);
		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss[2] == 'C');
		CHECK(ss[3] == 'D');


		ss.insert(ss.begin() + 2, 'X');

		CHECK(ss.size() == 5);
		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'C');
		CHECK(ss[4] == 'D');

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};

		ss.insert(ss.begin(), buffer);

		CHECK(ss.size() == 11);
		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');
		CHECK(ss[6] == 'A');
		CHECK(ss[7] == 'B');
		CHECK(ss[8] == 'X');
		CHECK(ss[9] == 'C');
		CHECK(ss[10] == 'D');

		ss.insert(ss.end(), buffer);
		CHECK(ss.size() == 17);
		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');
		CHECK(ss[6] == 'A');
		CHECK(ss[7] == 'B');
		CHECK(ss[8] == 'X');
		CHECK(ss[9] == 'C');
		CHECK(ss[10] == 'D');
		CHECK(ss[11] == 'Q');
		CHECK(ss[12] == 'W');
		CHECK(ss[13] == 'E');
		CHECK(ss[14] == 'R');
		CHECK(ss[15] == 'T');
		CHECK(ss[16] == 'Y');

		std::array<u8, 2> buffer2{'1', '2'};
		ss.insert(ss.begin() + 6, buffer2);

		CHECK(ss.size() == 19);
		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss[6] == '1');
		CHECK(ss[7] == '2');

		CHECK(ss[8] == 'A');
		CHECK(ss[9] == 'B');
		CHECK(ss[10] == 'X');
		CHECK(ss[11] == 'C');
		CHECK(ss[12] == 'D');
		CHECK(ss[13] == 'Q');
		CHECK(ss[14] == 'W');
		CHECK(ss[15] == 'E');
		CHECK(ss[16] == 'R');
		CHECK(ss[17] == 'T');
		CHECK(ss[18] == 'Y');
	}


	SECTION("insert (small -> large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};

		ss.insert(ss.begin(), buffer);

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		ss.insert(ss.begin(), buffer);
		ss.insert(ss.begin(), buffer);
		ss.insert(ss.begin(), buffer);
		ss.insert(ss.begin(), buffer);

		CHECK(ss.size() == 30);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.insert(ss.begin(), buffer);

		CHECK(ss.size() == 36);
		CHECK(ss.capacity() == 48);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		for (int i = 0; i < ss.size(); i += 6)
		{
			CHECK(ss[i + 0] == 'Q');
			CHECK(ss[i + 1] == 'W');
			CHECK(ss[i + 2] == 'E');
			CHECK(ss[i + 3] == 'R');
			CHECK(ss[i + 4] == 'T');
			CHECK(ss[i + 5] == 'Y');
		}
	}
	SECTION("insert (large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 256; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 256);
		CHECK(ss.capacity() == 256);
		CHECK(ss[0] == 0);
		CHECK(ss[1] == 1);
		CHECK(ss[2] == 2);
		CHECK(ss[3] == 3);
		CHECK(ss[4] == 4);
		CHECK(ss[5] == 5);
		CHECK(ss[6] == 6);
		CHECK(ss.back() == 255);

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};

		ss.insert(ss.begin(), buffer);

		CHECK(ss.size() == 262);
		CHECK(ss.capacity() == 384);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');
		CHECK(ss.back() == 255);


		ss.insert(ss.end(), 'P');
		CHECK(ss.back() == 'P');
		CHECK(ss.size() == 263);
		CHECK(ss.capacity() == 384);
	}

	SECTION("erase (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};

		ss.insert(ss.begin(), buffer);

		CHECK(ss.size() == 6);

		ss.erase(ss.cbegin() + 2, ss.cend() - 2);

		CHECK(ss.size() == 4);
		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'T');
		CHECK(ss[3] == 'Y');


		ss.erase(ss.cbegin() + 2, ss.cend());
		CHECK(ss.size() == 2);
		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		ss.erase(ss.begin() + 1);
		CHECK(ss.size() == 1);
		CHECK(ss[0] == 'Q');

		ss.assign(buffer);
		CHECK(ss.size() == 6);

		ss.erase(0, 2);
		CHECK(ss.size() == 4);
		CHECK(ss[0] == 'E');
		CHECK(ss[1] == 'R');
		CHECK(ss[2] == 'T');
		CHECK(ss[3] == 'Y');

		ss.erase(2, 2);
		CHECK(ss.size() == 2);
		CHECK(ss[0] == 'E');
		CHECK(ss[1] == 'R');

		ss.erase(0, 2);
		CHECK(ss.size() == 0);
	}


	SECTION("erase (large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 256; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 256);
		CHECK(ss.capacity() == 256);

		ss.erase(ss.cbegin() + 2, ss.cend() - 2);

		CHECK(ss.size() == 4);
		CHECK(ss.capacity() == 256);
		CHECK(ss[0] == 0);
		CHECK(ss[1] == 1);
		CHECK(ss[2] == 254);
		CHECK(ss[3] == 255);


		ss.erase(ss.cbegin() + 2, ss.cend());
		CHECK(ss.size() == 2);
		CHECK(ss[0] == 0);
		CHECK(ss[1] == 1);


		ss.erase(ss.begin());
		CHECK(ss.size() == 1);
		CHECK(ss[0] == 1);
	}

	SECTION("replace equal size (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 3> newbuffer{'X', 'Y', 'Z'};

		ss.replace(ss.begin() + 2, ss.begin() + 5, newbuffer);

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Z');

		CHECK(ss[5] == 'Y');

		std::array<u8, 10> nb2{'h', 'e', 'l', 'l', 'o', ' ', 0xF0, 0x9F, 0x8C, 0x8D};
		std::array<u8, 2>  hi{'h', 'i'};
		ss.assign(nb2);
		CHECK(ss.size() == 10);
		CHECK(ss[0] == 'h');
		CHECK(ss[1] == 'e');
		CHECK(ss[2] == 'l');
		CHECK(ss[3] == 'l');
		CHECK(ss[4] == 'o');
		CHECK(ss[5] == ' ');
		CHECK(ss[6] == 0xF0);
		CHECK(ss[7] == 0x9F);
		CHECK(ss[8] == 0x8C);
		CHECK(ss[9] == 0x8D);

		ss.replace(0, 5, hi);
		CHECK(ss.size() == 7);
		CHECK(ss[0] == 'h');
		CHECK(ss[1] == 'i');
		CHECK(ss[2] == ' ');

		CHECK(ss[3] == 0xF0);
		CHECK(ss[4] == 0x9F);
		CHECK(ss[5] == 0x8C);
		CHECK(ss[6] == 0x8D);
	}

	SECTION("replace single range (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 3> newbuffer{'X', 'Y', 'Z'};

		ss.replace(ss.begin() + 2, ss.begin() + 2, newbuffer);

		CHECK(ss.size() == 8);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Z');

		CHECK(ss[5] == 'R');
		CHECK(ss[6] == 'T');
		CHECK(ss[7] == 'Y');
	}

	SECTION("replace smaller size (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 2> newbuffer{'X', 'Y'};

		ss.replace(ss.begin() + 2, ss.begin() + 5, newbuffer);

		CHECK(ss.size() == 5);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Y');
	}


	SECTION("replace larger range (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(ss.begin() + 2, ss.begin() + 4, newbuffer);

		CHECK(ss.size() == 8);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Z');
		CHECK(ss[5] == 'Q');

		CHECK(ss[6] == 'T');
		CHECK(ss[7] == 'Y');
	}

	SECTION("replace smaller to larger")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 31; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 0);
		CHECK(ss[1] == 1);
		CHECK(ss[2] == 2);
		CHECK(ss[3] == 3);
		CHECK(ss[4] == 4);
		CHECK(ss[5] == 5);
		CHECK(ss.back() == 30);

		std::array<u8, 7> newbuffer{'A', 'B', 'C', 'D', 'E', 'F', 'G'};

		ss.replace(ss.begin() + 2, ss.begin() + 5, newbuffer);


		CHECK(ss.size() == 35);
		CHECK(ss.capacity() == 48);

		CHECK(ss[0] == 0);
		CHECK(ss[1] == 1);
		CHECK(ss[2] == 'A');
		CHECK(ss[3] == 'B');
		CHECK(ss[4] == 'C');
		CHECK(ss[5] == 'D');
		CHECK(ss[6] == 'E');
		CHECK(ss[7] == 'F');
		CHECK(ss[8] == 'G');
		CHECK(ss[9] == 5);
		CHECK(ss[10] == 6);
		CHECK(ss.back() == 30);
	}


	SECTION("replace larger size (large)")
	{

		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 256; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 256);
		CHECK(ss.capacity() == 256);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(ss.begin() + 2, ss.begin() + 5, newbuffer);

		CHECK(ss.size() == 257);
		CHECK(ss.capacity() == 384);

		CHECK(ss[0] == 0);
		CHECK(ss[1] == 1);

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Z');
		CHECK(ss[5] == 'Q');

		CHECK(ss[6] == 5);
		CHECK(ss[7] == 6);
		CHECK(ss[8] == 7);
		CHECK(ss[9] == 8);
		CHECK(ss.back() == 255);
	}

	SECTION("replace start, pos and count (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(0, 2, newbuffer);

		CHECK(ss.size() == 8);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'X');
		CHECK(ss[1] == 'Y');
		CHECK(ss[2] == 'Z');
		CHECK(ss[3] == 'Q');

		CHECK(ss[4] == 'E');
		CHECK(ss[5] == 'R');
		CHECK(ss[6] == 'T');
		CHECK(ss[7] == 'Y');
	}

	SECTION("replace middle, pos and count (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(2, 2, newbuffer);

		CHECK(ss.size() == 8);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Z');
		CHECK(ss[5] == 'Q');

		CHECK(ss[6] == 'T');
		CHECK(ss[7] == 'Y');
	}

	SECTION("replace end, pos and count (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(4, 2, newbuffer);

		CHECK(ss.size() == 8);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');

		CHECK(ss[4] == 'X');
		CHECK(ss[5] == 'Y');
		CHECK(ss[6] == 'Z');
		CHECK(ss[7] == 'Q');
	}

	SECTION("replace begin single iterator, (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(ss.begin(), newbuffer);

		CHECK(ss.size() == 9);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'X');
		CHECK(ss[1] == 'Y');
		CHECK(ss[2] == 'Z');
		CHECK(ss[3] == 'Q');

		CHECK(ss[4] == 'W');
		CHECK(ss[5] == 'E');
		CHECK(ss[6] == 'R');
		CHECK(ss[7] == 'T');
		CHECK(ss[8] == 'Y');
	}

	SECTION("replace middle single iterator, (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(ss.begin() + 2, newbuffer);

		CHECK(ss.size() == 9);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');

		CHECK(ss[2] == 'X');
		CHECK(ss[3] == 'Y');
		CHECK(ss[4] == 'Z');
		CHECK(ss[5] == 'Q');

		CHECK(ss[6] == 'R');
		CHECK(ss[7] == 'T');
		CHECK(ss[8] == 'Y');
	}


	SECTION("replace end single iterator, (small)")
	{
		sbo<32> ss;

		std::array<u8, 6> buffer{'Q', 'W', 'E', 'R', 'T', 'Y'};
		ss.append(buffer);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');
		CHECK(ss[5] == 'Y');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 4> newbuffer{'X', 'Y', 'Z', 'Q'};

		ss.replace(ss.end(), newbuffer);

		CHECK(ss.size() == 9);
		CHECK(ss.capacity() == 31);

		CHECK(ss[0] == 'Q');
		CHECK(ss[1] == 'W');
		CHECK(ss[2] == 'E');
		CHECK(ss[3] == 'R');
		CHECK(ss[4] == 'T');

		CHECK(ss[5] == 'X');
		CHECK(ss[6] == 'Y');
		CHECK(ss[7] == 'Z');
		CHECK(ss[8] == 'Q');
	}

	SECTION("assign (small)")
	{
		sbo<32> ss;
		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		CHECK(ss.size() == 3);
		CHECK(ss.capacity() == 31);

		sbo<32> a;

		a.assign(ss);
		CHECK(a.size() == 3);
		CHECK(a.capacity() == 31);


		a.assign('X');
		CHECK(a.size() == 1);
		CHECK(a.capacity() == 31);

		a.assign({'X', 'Y', 'Z'});
		CHECK(a.size() == 3);
		CHECK(a.capacity() == 31);
	}


	SECTION("assign (large)")
	{
		sbo<32> ss;

		for (u32 i = 0; i < 256; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 256);
		CHECK(ss.capacity() == 256);

		sbo<32> a;

		a.assign(ss);
		CHECK(a.size() == 256);
		CHECK(a.capacity() == 256);

		a.assign('X');
		CHECK(a.size() == 1);
		CHECK(a.capacity() == 256);

		a.shrink_to_fit();
		CHECK(a.size() == 1);
		CHECK(a.capacity() == 31);
	}

	SECTION("swap small buffers")
	{
		sbo<32> sbo1;
		sbo<32> sbo2;

		sbo1.push_back('A');
		sbo1.push_back('B');
		sbo1.push_back('C');

		sbo2.push_back('X');
		sbo2.push_back('Y');
		sbo2.push_back('Z');

		sbo1.swap(sbo2);

		CHECK(sbo1.size() == 3);
		CHECK(sbo1[0] == 'X');
		CHECK(sbo1[1] == 'Y');
		CHECK(sbo1[2] == 'Z');

		CHECK(sbo2.size() == 3);
		CHECK(sbo2[0] == 'A');
		CHECK(sbo2[1] == 'B');
		CHECK(sbo2[2] == 'C');
	}

	SECTION("swap large buffers")
	{
		sbo<32> sbo1;
		sbo<32> sbo2;

		for (int i = 0; i < 40; ++i)
		{
			sbo2.push_back(as<u8>('a' + i));
			sbo1.push_back(as<u8>('A' + i));
		}

		sbo1.swap(sbo2);

		CHECK(sbo1.size() == 40);
		for (int i = 0; i < 40; ++i)
			CHECK(sbo1[i] == 'a' + i);

		CHECK(sbo2.size() == 40);
		for (int i = 0; i < 40; ++i)
			CHECK(sbo2[i] == 'A' + i);
	}

	SECTION("find (small)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		auto it = std::find(ss.begin(), ss.end(), 'C');
		CHECK(it != ss.end());
		CHECK(*it == 'C');

		it = std::find(ss.begin(), ss.end(), 'Z');
		CHECK(it == ss.end());
	}

	SECTION("find (large)")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		for (u32 i = 0; i < 128; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 128);
		CHECK(ss.capacity() == 168);

		auto it = std::find(ss.begin(), ss.end(), as<u8>(64));
		CHECK(it != ss.end());
		CHECK(*it == as<u8>(64));

		it = std::find(ss.begin(), ss.end(), as<u8>(200));
		CHECK(it == ss.end());
	}

	SECTION("find range")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		std::array<u8, 3> range{'C', 'D', 'E'};
		auto              it = ss.find(range);
		CHECK(it != ss.end());
		CHECK(*it == 'C');

		std::array<u8, 3> not_in_range{'X', 'Y', 'Z'};
		it = ss.find(not_in_range);
		CHECK(it == ss.end());
	}

	SECTION("contains")
	{
		sbo<32> ss;

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		CHECK(ss.contains('B') == true);
		CHECK(ss.contains('O') == false);

		for (u32 i = 0; i < 128; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 134);
		CHECK(ss.capacity() == 168);

		CHECK(ss.contains(0x7F) == true);
		CHECK(ss.contains(0xFF) == false);
	}

	SECTION("compare (small)")
	{
		sbo<32> a;

		CHECK(a.size() == 0);
		CHECK(a.capacity() == 31);

		a.push_back('A');
		a.push_back('B');
		a.push_back('C');
		a.push_back('D');
		a.push_back('E');
		a.push_back('F');

		sbo<32> b;

		CHECK(b.size() == 0);
		CHECK(b.capacity() == 31);

		b.push_back('A');
		b.push_back('B');
		b.push_back('C');
		b.push_back('D');
		b.push_back('E');
		b.push_back('F');

		CHECK(a == b);
	}

	SECTION("compare (large)")
	{
		sbo<32> a;
		CHECK(a.size() == 0);
		CHECK(a.capacity() == 31);

		for (u32 i = 0; i < 128; i++)
			a.push_back(as<u8>(i));

		CHECK(a.size() == 128);
		CHECK(a.capacity() == 168);

		sbo<32> b;

		CHECK(b.size() == 0);
		CHECK(b.capacity() == 31);

		for (u32 i = 0; i < 128; i++)
			b.push_back(as<u8>(i));

		CHECK(a.size() == 128);
		CHECK(a.capacity() == 168);


		CHECK(a == b);
	}

	SECTION("compare (small with large")
	{
		sbo<32> a;

		CHECK(a.size() == 0);
		CHECK(a.capacity() == 31);

		a.push_back('A');
		a.push_back('B');
		a.push_back('C');
		a.push_back('D');
		a.push_back('E');
		a.push_back('F');

		CHECK(a.size() == 6);
		CHECK(a.capacity() == 31);

		sbo<32> b;

		CHECK(b.size() == 0);
		CHECK(b.capacity() == 31);

		for (u32 i = 0; i < 128; i++)
			b.push_back(as<u8>(i));

		CHECK(b.size() == 128);
		CHECK(b.capacity() == 168);

		CHECK(a != b);
	}

	SECTION("sub_sbo")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};
		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss.front() == 'A');
		CHECK(ss.back() == 'F');

		sbo<32> ss2 = ss.sub_sbo(1, 5);

		CHECK(ss2.size() == 4);
		CHECK(ss2.capacity() == 31);
		CHECK(ss2.front() == 'B');
		CHECK(ss2.back() == 'E');
	}

	SECTION("hash")
	{

		sbo<32> a;

		CHECK(a.size() == 0);
		CHECK(a.capacity() == 31);

		a.push_back('A');
		a.push_back('B');
		a.push_back('C');
		a.push_back('D');
		a.push_back('E');
		a.push_back('F');

		CHECK(a.size() == 6);
		CHECK(a.capacity() == 31);

		std::hash<sbo<32>> hasher;
		CHECK(hasher(a) == 0xdc45'7fe2'cbcd'f06d);


		sbo<32> b = a;
		sbo<32> c = a;

		c.push_back('G');

		CHECK(c.size() == 7);

		CHECK(hasher(a) == hasher(b));
		CHECK(hasher(a) != hasher(c));
	}

	SECTION("const reference")
	{
		const sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);
		CHECK(ss[0] == 'A');
		CHECK(ss[1] == 'B');
		CHECK(ss[2] == 'C');
		CHECK(ss[3] == 'D');
		CHECK(ss[4] == 'E');
		CHECK(ss[5] == 'F');
	}

	SECTION("index operator non-const")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		ss[0] = 'Z';
		CHECK(ss[0] == 'Z');
		CHECK(ss[1] == 'B');
		CHECK(ss[2] == 'C');
		CHECK(ss[3] == 'D');
		CHECK(ss[4] == 'E');
		CHECK(ss[5] == 'F');
	}

	SECTION("iterator +-")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		auto it = ss.begin();
		CHECK(*it == 'A');

		it += 2;
		CHECK(*it == 'C');

		it++;
		CHECK(*it == 'D');

		it -= 2;
		CHECK(*it == 'B');

		--it;
		CHECK(*it == 'A');
	}

	SECTION("iterator distance")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F'};

		CHECK(ss.size() == 6);
		CHECK(ss.capacity() == 31);

		auto start = ss.begin();
		auto end   = ss.end();

		CHECK(std::distance(start, end) == 6);
	}

	SECTION("find_first_of")
	{
		sbo<32> ss{'A', 'B', 'C', 'D', 'E', 'F', 'G'};

		CHECK(ss.size() == 7);
		CHECK(ss.capacity() == 31);

		std::array<u8, 2> search{'C', 'D'};

		auto it = ss.find_first_of(search);
		CHECK(std::distance(ss.begin(), it) == 2);


		// it = ss.find_first_of("Q"sv);


		// it = ss.find_first_of({'D', 'E', 'F'});
	}
}
