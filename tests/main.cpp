#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import std;
using namespace std::chrono_literals;

TEST_CASE("name" * doctest::description("shouldn't take more than 500ms") * doctest::timeout(0.5) *
		  doctest::may_fail(true) /* * doctest::skip(true)*/)
{
	// asserts
	std::this_thread::sleep_for(0.3s);
	// WARN_EQ(1, 2);
}

TEST_CASE("test name" * doctest::description("hello description"))
{

	WARN_EQ(1, 1);
	// do asserts with data
}

TEST_SUITE("math")
{
	TEST_CASE("mul")
	{
		//
		CHECK(1 == 1);
	}
	TEST_CASE("div")
	{
		//
		CHECK(1 == 1);
	}
}

TEST_CASE("all binary assertions")
{
	WARN_EQ(1, 1);
	CHECK_EQ(1, 1);
	REQUIRE_EQ(1, 1);
	WARN_NE(1, 0);
	CHECK_NE(1, 0);
	REQUIRE_NE(1, 0);
	WARN_GT(1, 0);
	CHECK_GT(1, 0);
	REQUIRE_GT(1, 0);
	WARN_LT(0, 1);
	CHECK_LT(0, 1);
	REQUIRE_LT(0, 1);
	WARN_GE(1, 1);
	CHECK_GE(1, 1);
	REQUIRE_GE(1, 1);
	WARN_LE(1, 1);
	CHECK_LE(1, 1);
	REQUIRE_LE(1, 1);
	WARN_UNARY(1);
	CHECK_UNARY(1);
	REQUIRE_UNARY(1);
	WARN_UNARY_FALSE(0);
	CHECK_UNARY_FALSE(0);
	REQUIRE_UNARY_FALSE(0);
}
