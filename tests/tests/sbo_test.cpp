#include <catch2/catch_test_macros.hpp>


import std;
import deckard.sbo;
import deckard.helpers;

using namespace deckard;

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
			sbo.push_back('0'+i);

		REQUIRE(sbo.size() == sbo.capacity());
		REQUIRE(sbo.capacity() == 14);

		for (u32 i : range(0, sbo.capacity()))
			REQUIRE(sbo[i] == '0'+i);

		sbo.resize(2);

		REQUIRE(sbo.size() == 2);
		REQUIRE(sbo.capacity() == 14);

		REQUIRE(sbo[0] == '0');
		REQUIRE(sbo[1] == '1');
		for (u32 i : range(2, sbo.capacity()))
			REQUIRE(sbo[i] == 0);

	}

	SECTION("push_back until resize to heap")
	{
		//
		basic_smallbuffer<16> sbo;
	}

	SECTION("resize from heap back to small buffer")
	{
		//
		basic_smallbuffer<16> sbo;
	}
}
