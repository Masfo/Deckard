#include <catch2/catch_test_macros.hpp>


import std;
import deckard.uint128;
using namespace deckard::uint128;

TEST_CASE("uint128", "[uint128]")
{
	SECTION("ctor")
	{
		//
		uint128 a(2, 0);
		uint128 b(4, 0);

		CHECK(a + b == uint128(6, 0));


		// Test max uint64_t addition
		uint128 max64_1(0, UINT64_MAX);
		uint128 max64_2(0, UINT64_MAX);
		uint128 expected(1, UINT64_MAX - 1);

		CHECK(max64_1 + max64_2 == expected);

		// Test carry propagation
		uint128 c(0, UINT64_MAX);
		uint128 d(1, 0);
		uint128 sum = c + d;
		CHECK(sum == uint128(1, UINT64_MAX));

		sum = sum + uint128(0, 2);


		CHECK(sum == uint128(2, 1));

		// Test maximum value addition
		uint128 max1(UINT64_MAX, UINT64_MAX);
		uint128 one(0, 1);
		CHECK(max1 + one == uint128(0, 0)); // Should wrap to zero


		uint128 dc(UINT64_MAX - 1, UINT64_MAX);
		CHECK(dc + 1u == uint128(UINT64_MAX, 0)); 
	}

	SECTION("sub")
	{
		uint128 max64(UINT64_MAX, UINT64_MAX);
		uint128 max_one(0, UINT64_MAX);
		uint128 one(0, 1);

		auto sub = max64 - max_one;

		CHECK(sub == uint128(UINT64_MAX, 0));

		sub = sub - one;
		CHECK(sub == uint128(UINT64_MAX - 1, UINT64_MAX));
	}

	SECTION("mul")
	{
		uint128 a = uint128(0xFF'FF12'3567'8931) * uint128(0x12'3568'FFF4'5656);
		CHECK(a == uint128("369309726874948344242154908060790"));
	}
}
