#include <catch2/catch_test_macros.hpp>


import deckard.types;
import deckard.uuid;
import deckard.utf8;
import deckard.bytepool;
import deckard.stringpool;
import deckard.helpers;

using namespace deckard;
using namespace std::string_view_literals;

TEST_CASE("utility", "[utility]")
{
	SECTION("limits")
	{
		//
		CHECK(524'287 == limits::bits_to_unsigned_max(19));
		CHECK(262'143 == limits::bits_to_signed_max(19));

		CHECK(127 == limits::bits_to_signed_max(8));
		CHECK(524'287 == limits::bits_to_unsigned_max(19));

		CHECK(-128 == limits::bits_to_signed_min(8));
		CHECK(-262'144 == limits::bits_to_signed_min(19));

		CHECK(0xFFFF'FFFF == limits::max<u32>);
		CHECK(-2'147'483'648 == limits::min<i32>);
	}

	SECTION("is_equal")
	{
		std::array<u8, 5> a{1, 2, 3, 4, 5};
		std::array<u8, 5> b{1, 2, 3, 4, 5};
		std::array<u8, 5> c{5, 6, 7, 8, 9};
		CHECK(is_equal(a, b));
		CHECK_FALSE(is_equal(a, c));
	}
}

TEST_CASE("uuid", "[uuid]")
{
	SECTION("v4")
	{
		uuid::v4::uuid id;

		CHECK(id.ab == 0);
		CHECK(id.cd == 0);

		id = uuid::v4::generate();

		CHECK(id.ab != 0);
		CHECK(id.cd != 0);

		// lower
		auto fmt = std::format("{}", id);
		CHECK(not fmt.empty());
		CHECK(std::ranges::all_of(fmt, [](char32 c) { return c == '-' or utf8::is_ascii_hex_digit_lower(c); }));

		// lower
		fmt = std::format("{:x}", id);
		CHECK(not fmt.empty());
		CHECK(std::ranges::all_of(fmt, [](char32 c) { return c == '-' or utf8::is_ascii_hex_digit_lower(c); }));

		// upper
		fmt = std::format("{:X}", id);
		CHECK(not fmt.empty());
		CHECK(std::ranges::all_of(fmt, [](char32 c) { return c == '-' or utf8::is_ascii_hex_digit_upper(c); }));
	}

	SECTION("v7")
	{
		uuid::v7::uuid id;
		CHECK(id.ab == 0);
		CHECK(id.cd == 0);

		id = uuid::v7::generate();

		// lower
		auto fmt = std::format("{}", id);
		CHECK(not fmt.empty());
		CHECK(std::ranges::all_of(fmt, [](char32 c) { return c == '-' or utf8::is_ascii_hex_digit_lower(c); }));

		// lower
		fmt = std::format("{:x}", id);
		CHECK(not fmt.empty());
		CHECK(std::ranges::all_of(fmt, [](char32 c) { return c == '-' or utf8::is_ascii_hex_digit_lower(c); }));

		// upper
		fmt = std::format("{:X}", id);
		CHECK(not fmt.empty());
		CHECK(std::ranges::all_of(fmt, [](char32 c) { return c == '-' or utf8::is_ascii_hex_digit_upper(c); }));
	}
}

TEST_CASE("bytepool", "[bytepool]")
{
	SECTION("ctor")
	{
		bytepool pool(1024);

		CHECK(pool.size() == 0);
	}

	SECTION("reset")
	{
		bytepool          pool(1024);
		std::array<u8, 5> data{1, 2, 3, 4, 5};
		(void)pool.add(data);
		CHECK(pool.size() == 1);
		pool.reset();
		CHECK(pool.empty());
		CHECK(pool.size() == 0);
	}

	SECTION("add and retrieve")
	{
		bytepool pool(1024);

		std::array<u8, 5> data{1, 2, 3, 4, 5};

		auto index = pool.add(data);

		CHECK(index == 0);
		CHECK(pool.size() == 1);

		const auto retrieved = pool.get(index);
		CHECK(is_equal(retrieved, data));
	}

	SECTION("merge")
	{
		bytepool pool(1024);

		std::array<u8, 5> data{1, 2, 3, 4, 5};

		(void)pool.add(data);

		CHECK(pool.size() == 1);

		// pool 2
		bytepool pool2(1024);

		std::array<u8, 5> data2{5, 6, 7, 8, 9};
		(void)pool2.add(data2);

		CHECK(pool2.size() == 1);

		pool.merge(pool2);

		CHECK(pool.size() == 2);
		CHECK(pool2.empty());

		const auto retrieved0 = pool.get(0);
		const auto retrieved1 = pool.get(1);

		CHECK(is_equal(retrieved0, data));
		CHECK(is_equal(retrieved1, data2));
	}

	SECTION("combine") 
	{
		bytepool pool(1024);

		std::array<u8, 5> data{1, 2, 3, 4, 5};

		(void)pool.add(data);

		CHECK(pool.size() == 1);

		// pool 2
		bytepool pool2(1024);
		std::array<u8, 5> data2{5, 6, 7, 8, 9};
		(void)pool2.add(data2);
		CHECK(pool2.size() == 1);

		// combine pool2 into pool
		pool.combine(pool2);

		CHECK(pool.size() == 2);
		CHECK(pool2.size() == 1);

		auto retrieved0 = pool.get(0);
		auto retrieved1 = pool.get(1);
		auto retrieved2 = pool2.get(0);
		CHECK(is_equal(retrieved0,data));
		CHECK(is_equal(retrieved1,data2));
		CHECK(is_equal(retrieved2, data2));

	}

	SECTION("contains")
	{
		bytepool          pool(1024);
		std::array<u8, 5> data{1, 2, 3, 4, 5};
		(void)pool.add(data);
		CHECK(pool.contains(data));
		CHECK_FALSE(pool.contains(std::array<u8, 5>{5, 6, 7, 8, 9}));
	}
}

TEST_CASE("stringpool", "[stringpool]")
{
	SECTION("ctor")
	{
		string_pool pool(1024);
		CHECK(pool.empty());
		CHECK(pool.size() == 0);
	}

	SECTION("reset")
	{
		string_pool pool(1024);
		(void)pool.add("hello world"sv);

		CHECK_FALSE(pool.empty());
		CHECK(pool.size() == 1);

		pool.reset();

		CHECK(pool.empty());
		CHECK(pool.size() == 0);
	}


	SECTION("add and retrieve (ascii)")
	{
		string_pool      pool(1024);
		std::string_view str   = "hello world";
		auto              index = pool.add(str);
		CHECK(index == 0);
		CHECK(pool.size() == 1);

		const auto retrieved = pool.get(index);
		CHECK(retrieved == str);
	}


	SECTION("add and retrieve (unicode)")
	{
		string_pool pool(1024);
		utf8::view  str = "привет мир"sv;

		auto index = pool.add(str);
		CHECK(index == 0);
		CHECK(pool.size() == 1);

		const auto retrieved = pool.get(index);
		CHECK(retrieved == str);
	}

	SECTION("merge")
	{
		string_pool pool(1024);
		utf8::view  str = "привет мир"sv;
		(void)pool.add(str);

		string_pool pool2;
		(void)pool2.add("hello world"sv);

		CHECK(pool2.size() == 1);

		pool.merge(pool2);

		CHECK(pool2.empty());

		CHECK(pool.size() == 2);
		CHECK(pool.get(0) == str);
		CHECK(pool.get(1) == "hello world"sv);
	}

	SECTION("combine")
	{
		string_pool pool(1024);
		utf8::view  str = "привет мир"sv;
		(void)pool.add(str);

		string_pool pool2(1024);
		(void)pool2.add("hello world"sv);

		CHECK(pool2.size() == 1);

		pool.combine(pool2);

		CHECK(pool.size() == 2);
		CHECK(pool2.size() == 1);

		auto retrieved0 = pool.get(0);
		auto retrieved1 = pool.get(1);
		CHECK(retrieved0 == str);
		CHECK(retrieved1 == "hello world"sv);

	}


	SECTION("contains")
	{
		string_pool pool(1024);
		utf8::view  str = "привет мир"sv;
		(void)pool.add(str);
		CHECK(pool.contains(str));
		CHECK(pool.contains("привет мир"sv));
		CHECK_FALSE(pool.contains("hello world"sv));
	}
}
