#include <catch2/catch_test_macros.hpp>


import std;
import deckard.sbo;
import deckard.as;
import deckard.helpers;
import deckard.debug;

using namespace deckard;

TEST_CASE("sbo", "[sbov2]")
{
	using namespace v2;
	SECTION("constructor (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
	}

	SECTION("move c-tor (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss[0] == 'A');

		sbo<32> copy(std::move(ss));

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 31);
		REQUIRE(copy[0] == 'A');

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
	}

	SECTION("move assign self (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss[0] == 'A');

		ss = std::move(ss);

		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss[0] == 'A');
	}

	SECTION("move assignment (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss[0] == 'A');

		sbo<32> copy;

		copy = std::move(ss);

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 31);
		REQUIRE(copy[0] == 'A');

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
	}

	SECTION("move c-tor (large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);

		ss.resize(128);

		sbo<32> copy(std::move(ss));

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 128);
		REQUIRE(copy[0] == 'A');

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
	}

	SECTION("move assignment (large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);

		ss.resize(128);

		sbo<32> copy;
		copy = std::move(ss);

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 128);
		REQUIRE(copy[0] == 'A');

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
	}

	SECTION("move assignment self (large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);

		ss.resize(128);

		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 128);

		ss = std::move(ss);

		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 128);
	}

	SECTION("copy c-tor (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss[0] == 'A');

		sbo<32> copy(ss);

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 31);
		REQUIRE(copy[0] == 'A');
	}

	SECTION("copy assign (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss[0] == 'A');

		sbo<32> copy;
		copy = ss;

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 31);
		REQUIRE(copy[0] == 'A');
	}


	SECTION("copy c-tor (large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);

		ss.resize(128);

		sbo<32> copy(ss);

		REQUIRE(copy.size() == 6);
		REQUIRE(copy.capacity() == 128);
		REQUIRE(copy[0] == 'A');
	}


	SECTION("indexing (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		REQUIRE(ss[0] == 0);
		REQUIRE(ss.size() == 0);
	}


	SECTION("push_back (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('Q');

		REQUIRE(ss[0] == 'Q');
		REQUIRE(ss.size() == 1);

		ss.push_back('B');
		REQUIRE(ss[1] == 'B');
		REQUIRE(ss.size() == 2);
	}

	SECTION("push_back (small -> large)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);


		repeat<64> = [&] { ss.push_back('A'); };

		REQUIRE(ss[0] == 'A');
		REQUIRE(ss.size() == 64);
		REQUIRE(ss.capacity() == 64);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);


		ss.push_back('B');
		REQUIRE(ss.size() == 65);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	// popback
	SECTION("pop_back (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('A');
		ss.push_back('B');


		REQUIRE(ss[0] == 'A');
		REQUIRE(ss[1] == 'B');
		REQUIRE(ss.size() == 2);
		REQUIRE(ss.capacity() == 31);

		ss.pop_back();
		REQUIRE(ss[0] == 'A');
		REQUIRE(ss.size() == 1);
		REQUIRE(ss.capacity() == 31);
	}

	SECTION("pop_back (large)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		repeat<65> = [&] { ss.push_back('A'); };


		REQUIRE(ss[0] == 'A');
		REQUIRE(ss.size() == 65);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		repeat<55> = [&] { ss.pop_back(); };

		REQUIRE(ss[0] == 'A');
		REQUIRE(ss.size() == 10);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);


		ss.shrink_to_fit();

		REQUIRE(ss.size() == 10);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
	}

	SECTION("resize (small -> large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		ss.push_back('A');
		ss.push_back('B');

		REQUIRE(ss[0] == 'A');
		REQUIRE(ss[1] == 'B');
		REQUIRE(ss.size() == 2);
		REQUIRE(ss.capacity() == 31);

		ss.pop_back();
		REQUIRE(ss[0] == 'A');
		REQUIRE(ss[1] == 'B');
		REQUIRE(ss.size() == 1);
		REQUIRE(ss.capacity() == 31);
	}


	SECTION("resize (larger -> large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.resize(128);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);


		ss.resize(40);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 40);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("resize (large -> small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);


		ss.resize(128);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);


		ss.resize(15);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 15);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("clear (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.push_back('A');
		REQUIRE(ss.size() == 1);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.clear();
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
	}


	SECTION("shrink_to_fit (small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.shrink_to_fit();
		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
	}


	SECTION("shrink_to_fit (large -> small)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.push_back('A');
		ss.push_back('B');
		ss.push_back('C');
		ss.push_back('D');
		ss.push_back('E');
		ss.push_back('F');


		ss.resize(128);

		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
		REQUIRE(ss[0] == 'A');


		ss.shrink_to_fit();
		REQUIRE(ss.size() == 6);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
		REQUIRE(ss[0] == 'A');
	}

	SECTION("append (small, small -> large, large -> large)")
	{
		std::array<u8, 8> arr{1, 2, 3, 4, 5, 6, 7, 8};

		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.append(arr);

		REQUIRE(ss.size() == 8);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.append(arr);

		REQUIRE(ss.size() == 16);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.append(ss.data());

		REQUIRE(ss.size() == 32);
		REQUIRE(ss.capacity() == 64);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		ss.append(ss.data());

		REQUIRE(ss.size() == 64);
		REQUIRE(ss.capacity() == 64);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		ss.append(arr);

		REQUIRE(ss.size() == 64 + 8);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("append (large -> larger)")
	{
		sbo<32> ss;

		ss.reserve(128);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 128);

		std::vector<u8> buffer;

		for (const auto& i : upto(128))
		{
			ss.push_back(as<u8>(i + '0'));
			buffer.push_back('B');
			buffer.push_back('C');
			buffer.push_back('D');
			buffer.push_back('E');
		}

		REQUIRE(buffer.size() == 128 * 4);

		REQUIRE(ss.size() == 128);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		ss.append(buffer);


		REQUIRE(ss.size() == 128 + 512);
		REQUIRE(ss.capacity() == 128 + 512);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("clear (large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.push_back('A');
		ss.resize(128);

		REQUIRE(ss.size() == 1);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		ss.clear();
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("push_back (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);

		ss.push_back('Q');

		REQUIRE(ss[0] == 'Q');
		REQUIRE(ss.size() == 1);

		ss.push_back('B');
		REQUIRE(ss[1] == 'B');
		REQUIRE(ss.size() == 2);
	}

	SECTION("front / back (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		for (u32 i = 0; i < ss.capacity(); i++)
			ss.push_back(as<u8>(i));

		REQUIRE(ss.front() == 0);
		REQUIRE(ss.back() == 30);

		REQUIRE(ss.size() == 31);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
	}

	SECTION("reserve (small)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.reserve(20);

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
	}

	SECTION("reserve (small -> large)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.reserve(128);

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		ss.reserve(256);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 256);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		ss.reserve(128);
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 256);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);
	}

	SECTION("front / back (large)")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		ss.reserve(256);

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 256);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < 256; i++)
			ss.push_back(as<u8>(i));

		REQUIRE(ss.size() == 256);
		REQUIRE(ss.capacity() == 256);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < 256; i++)
			REQUIRE(ss[i] == i);

		REQUIRE(ss.front() == 0x00);
		REQUIRE(ss.back() == 0xFF);
	}
	SECTION("do all the things")
	{
		sbo<32> ss;

		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		for (u32 i = 0; i < 31; i++)
			ss.push_back(as<u8>(i));

		REQUIRE(ss.size() == 31);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		for (u32 i = 0; i < ss.size(); i++)
			REQUIRE(ss[i] == i);

		ss.push_back(as<u8>(31));

		REQUIRE(ss.size() == 32);
		REQUIRE(ss.capacity() == 64);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < ss.size(); i++)
			REQUIRE(ss[i] == i);

		ss.pop_back();

		REQUIRE(ss.size() == 31);
		REQUIRE(ss.capacity() == 64);
		REQUIRE(ss.max_size() == 0xFFFF'FFFF);

		for (u32 i = 0; i < ss.size(); i++)
			REQUIRE(ss[i] == i);

		ss.shrink_to_fit();

		REQUIRE(ss.size() == 31);
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);

		for (u32 i = 0; i < ss.size(); i++)
			REQUIRE(ss[i] == i);

	}
}

// OLD
TEST_CASE("basic_small_buffer", "[sbo]")
{
	SECTION("constructor (small buffer)")
	{
		basic_smallbuffer<16> sbo;

		REQUIRE(sbo.size() == 0);
		REQUIRE(sbo.capacity() == 14);
	}

	SECTION("fill (small buffer)")
	{
		basic_smallbuffer<16> sbo;
		REQUIRE(sbo.size() == 0);
		REQUIRE(sbo.capacity() == 14);

		sbo.fill('x');

		REQUIRE(sbo.size() == sbo.capacity());
		REQUIRE(sbo.capacity() == 14);

		for (u32 i = 0; i < sbo.capacity(); ++i)
			REQUIRE(sbo[i] == (u8)'x');
	}

	SECTION("push_back 1 (small buffer)")
	{
		basic_smallbuffer<16> sbo;

		sbo.push_back('X');

		REQUIRE(sbo.size() == 1);
		REQUIRE(sbo.capacity() == 14);

		REQUIRE(sbo[0] == 'X');
		for (u32 i : range(1, sbo.capacity()))
			REQUIRE(sbo[i] == 0);
	}


	SECTION("push_back small buffer full then resize to smaller")
	{
		basic_smallbuffer<16> sbo;

		for (u32 i : upto(sbo.capacity()))
			sbo.push_back(as<u8>('0' + i));

		REQUIRE(sbo.size() == sbo.capacity());
		REQUIRE(sbo.capacity() == 14);

		for (u32 i : range(0, sbo.capacity()))
			REQUIRE(sbo[i] == '0' + i);

		sbo.resize(2);

		REQUIRE(sbo.size() == 2);
		REQUIRE(sbo.capacity() == 14);

		REQUIRE(sbo[0] == '0');
		REQUIRE(sbo[1] == '1');
		for (u32 i : range(2, sbo.capacity()))
			REQUIRE(sbo[i] == 0);
	}

	SECTION("resize same size")
	{
		//
		basic_smallbuffer<16> sbo;

		for (u32 i : upto(sbo.capacity()))
			sbo.push_back(as<u8>('0' + i));

		REQUIRE(sbo.size() == sbo.capacity());
		REQUIRE(sbo.capacity() == 14);

		sbo.resize(sbo.size());

		REQUIRE(sbo.size() == sbo.capacity());
		REQUIRE(sbo.capacity() == 14);
	}

	SECTION("resize from SBO to non SBO")
	{
		//
		basic_smallbuffer<16> sbo;

		REQUIRE(sbo.size() == 0);
		REQUIRE(sbo.capacity() == 14);


		sbo.resize(sbo.capacity() * 2);

		REQUIRE(sbo.size() == 0);
		REQUIRE(sbo.capacity() == 14 * 2);
	}
}
