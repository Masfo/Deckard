#include <catch2/catch_test_macros.hpp>


import std;
import deckard.sbo;
import deckard.as;
import deckard.helpers;
import deckard.debug;

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

		CHECK(ss.size() == 8);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.append(arr);

		CHECK(ss.size() == 16);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		ss.append(ss.data());

		CHECK(ss.size() == 32);
		CHECK(ss.capacity() == 48);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append(ss.data());

		CHECK(ss.size() == 64);
		CHECK(ss.capacity() == 72);
		CHECK(ss.max_size() == 0xFFFF'FFFF);

		ss.append(arr);

		CHECK(ss.size() == 64 + 8);
		CHECK(ss.capacity() == 72);
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

		CHECK(ss.size() == 0);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);

		for (u32 i = 0; i < 31; i++)
			ss.push_back(as<u8>(i));

		CHECK(ss.size() == 31);
		CHECK(ss.capacity() == 31);
		CHECK(ss.max_size() == 31);


		u8 i = 0;
		for (auto it = ss.begin(); it != ss.end(); it++)
			CHECK(*it == i++);

		i = 0;
		for (const auto& c : ss)
			CHECK(c == i++);


		auto it = ss.begin();
		CHECK(*it == 0);

		it++;
		CHECK(*it == 1);

		it--;
		CHECK(*it == 0);

		it += 5;
		CHECK(*it == 5);

		it -= 5;
		CHECK(*it == 0);
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
}
