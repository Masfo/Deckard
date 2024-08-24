

#include <catch2/catch_test_macros.hpp>

import deckard.ringbuffer;
import deckard.types;
import std;

using namespace deckard;

TEST_CASE("ringbuffer", "[ringbuffer]")
{
	SECTION("empty ringbuffer")
	{
		ringbuffer<u32> rb;

		REQUIRE(rb.size() == 0);
		REQUIRE(rb.capacity() == 8);
		REQUIRE(rb.empty() == true);
	}

	SECTION("ringbuffer clear")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);

		rb.clear();

		REQUIRE(rb.size() == 0);
		REQUIRE(rb.capacity() == 2);
	}

	SECTION("ringbuffer push")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);
		rb.push(15);


		REQUIRE(rb.size() == 2);
		REQUIRE(rb.capacity() == 2);

		REQUIRE(rb.front() == 10);
		REQUIRE(rb.back() == 15);
	}


	SECTION("ringbuffer push/pop")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);
		rb.push(15);
		(void)rb.pop();
		rb.push(20);


		REQUIRE(rb.size() == 2);
		REQUIRE(rb.capacity() == 2);

		REQUIRE(rb.front() == 15);
		REQUIRE(rb.back() == 20);
	}


	SECTION("ringbuffer push/pop/reference")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);
		rb.push(15);
		auto& item = rb.pop();
		item       = 30;
		rb.push(20);
		auto item2 = rb.pop();

		REQUIRE(rb.size() == 1);
		REQUIRE(rb.capacity() == 2);

		REQUIRE(item == 30);
		REQUIRE(item2 == 20);
	}


	SECTION("ringbuffer at")
	{
		ringbuffer<u32> rb(4);

		rb.push(5);
		rb.push(10);
		rb.push(15);
		rb.push(20);

		auto item = rb.at(2);
		REQUIRE(item == 15);

		REQUIRE(rb.size() == 4);
		REQUIRE(rb.capacity() == 4);
	}


	SECTION("ringbuffer front/back")
	{
		ringbuffer<u32> rb(4);

		rb.push(5);
		rb.push(10);
		rb.push(15);
		rb.push(20);
		rb.push(25);


		auto item = rb.front();
		REQUIRE(item == 10);

		auto item2 = rb.back();
		REQUIRE(item2 == 25);

		REQUIRE(rb.size() == 4);
		REQUIRE(rb.capacity() == 4);
	}

	SECTION("ringbuffer full")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		REQUIRE(rb.full() == false);
		rb.push(10);
		REQUIRE(rb.full() == true);
		(void)rb.pop();
		REQUIRE(rb.full() == false);
		rb.push(20);
		REQUIRE(rb.full() == true);


		REQUIRE(rb.size() == 2);
		REQUIRE(rb.capacity() == 2);
	}


	SECTION("ringbuffer last")
	{
		ringbuffer<u32> rb(16);

		for (i32 i = 0; i < 128; i++)
			rb.push(i);


		auto last = rb.last();
		REQUIRE(last == 127);

		auto last4 = rb.last(4);
		REQUIRE(last4 == std::vector<u32>{127, 126, 125, 124});


		REQUIRE(rb.size() == 16);
		REQUIRE(rb.capacity() == 16);
	}
}
