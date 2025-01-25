

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

		CHECK(rb.size() == 0);
		CHECK(rb.capacity() == 8);
		CHECK(rb.empty() == true);
	}

	SECTION("ringbuffer clear")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);

		rb.clear();

		CHECK(rb.size() == 0);
		CHECK(rb.capacity() == 2);
	}

	SECTION("ringbuffer push")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);
		rb.push(15);


		CHECK(rb.size() == 2);
		CHECK(rb.capacity() == 2);

		CHECK(rb.front() == 10);
		CHECK(rb.back() == 15);
	}


	SECTION("ringbuffer push/pop")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		rb.push(10);
		rb.push(15);
		(void)rb.pop();
		rb.push(20);


		CHECK(rb.size() == 2);
		CHECK(rb.capacity() == 2);

		CHECK(rb.front() == 15);
		CHECK(rb.back() == 20);
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

		CHECK(rb.size() == 1);
		CHECK(rb.capacity() == 2);

		CHECK(item == 30);
		CHECK(item2 == 20);
	}


	SECTION("ringbuffer at")
	{
		ringbuffer<u32> rb(4);

		rb.push(5);
		rb.push(10);
		rb.push(15);
		rb.push(20);

		auto item = rb.at(2);
		CHECK(item == 15);

		CHECK(rb.size() == 4);
		CHECK(rb.capacity() == 4);
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
		CHECK(item == 10);

		auto item2 = rb.back();
		CHECK(item2 == 25);

		CHECK(rb.size() == 4);
		CHECK(rb.capacity() == 4);
	}

	SECTION("ringbuffer full")
	{
		ringbuffer<u32> rb(2);

		rb.push(5);
		CHECK(rb.full() == false);
		rb.push(10);
		CHECK(rb.full() == true);
		(void)rb.pop();
		CHECK(rb.full() == false);
		rb.push(20);
		CHECK(rb.full() == true);


		CHECK(rb.size() == 2);
		CHECK(rb.capacity() == 2);
	}


	SECTION("ringbuffer last")
	{
		ringbuffer<u32> rb(16);

		for (i32 i = 0; i < 128; i++)
			rb.push(i);


		auto last = rb.last();
		CHECK(last == 127);

		auto last4 = rb.last(4);
		CHECK(last4 == std::vector<u32>{127, 126, 125, 124});


		CHECK(rb.size() == 16);
		CHECK(rb.capacity() == 16);
	}
}
