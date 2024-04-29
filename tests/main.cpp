#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import std;
using namespace std::chrono_literals;

// * doctest::description("description")
// * doctest::timeout(1)
// * doctest::may_fail(true)
// * doctest::skip(true)

TEST_CASE("example" * doctest::description("description") * doctest::skip(false))
{
	//
	// CHECK_EQ(1, 1);

	SUBCASE("")
	{
		//
	}
}

/*

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
	}
}

TEST_CASE("all binary assertions")
{
	CHECK(1 == 1);
	CHECK_EQ(1, 1);
	CHECK_NE(1, 0);
	CHECK_UNARY_FALSE(0);
	CHECK_GT(1, 0);
	CHECK_GE(1, 1);
	CHECK_LT(0, 1);
	CHECK_LE(1, 1);
	CHECK_UNARY(1);

	REQUIRE_EQ(1, 1);
	REQUIRE_NE(1, 0);
	REQUIRE_GT(1, 0);
	REQUIRE_LT(0, 1);
	REQUIRE_GE(1, 1);
	REQUIRE_LE(1, 1);
	REQUIRE_UNARY(1);
	REQUIRE_UNARY_FALSE(0);

	WARN_EQ(1, 1);
	WARN_LE(1, 1);
	WARN_NE(1, 0);
	WARN_GT(1, 0);
	WARN_LT(0, 1);
	WARN_GE(1, 1);
	WARN_UNARY(1);
	WARN_UNARY_FALSE(0);
}
*/
