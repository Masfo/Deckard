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

		repeat<64> = [&] { ss.push_back('A'); };

		REQUIRE(ss[0] == 'A');
		REQUIRE(ss.size() == 64);
		REQUIRE(ss.capacity() == 64);

		ss.push_back('B');
		REQUIRE(ss.size() == 65);
		REQUIRE(ss.capacity() == 128);
	}

	SECTION("resize (small -> large)")
	{
		sbo<32> ss;
		REQUIRE(ss.size() == 0);
		REQUIRE(ss.capacity() == 31);


		ss.push_back('A');
		REQUIRE(ss[0] == 'A');

		ss.resize(128);

		REQUIRE(ss.size() == 1);
		REQUIRE(ss.capacity() == 128);
		REQUIRE(ss[0] == 'A');
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
		REQUIRE(ss.capacity() == 31);
		REQUIRE(ss.max_size() == 31);
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
