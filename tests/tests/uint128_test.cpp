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
		CHECK(uint128(0, 5) + uint128(0, 7) == uint128(0, 12));
		CHECK(uint128(1, 0) + uint128(0, 1) == uint128(1, 1));
		CHECK(uint128(UINT64_MAX, UINT64_MAX) + uint128(0, 0) == uint128(UINT64_MAX, UINT64_MAX));

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
		CHECK(uint128(1, 0) - uint128(0, 1) == uint128(0, UINT64_MAX));
		CHECK(uint128(0, 10) - uint128(0, 5) == uint128(0, 5));

		sub = sub - one;
		CHECK(sub == uint128(UINT64_MAX - 1, UINT64_MAX));
	}

	SECTION("parse")
	{
		uint128 value("18446744073709551616");
		CHECK(value == uint128(1, 0));

		uint128 zero_bad("12a34");
		CHECK(zero_bad == uint128(0));
	}

	SECTION("compare")
	{
		uint128 a(1, 0);
		uint128 b(0, UINT64_MAX);
		CHECK(a > b);
		CHECK(b < a);
		CHECK(a >= a);
		CHECK(b <= a);
	}

	SECTION("bitwise")
	{
		uint128 a(0xFF00FF00FF00FF00, 0x0F0F0F0F0F0F0F0F);
		uint128 b(0x00FF00FF00FF00FF, 0xF0F0F0F0F0F0F0F0);
		CHECK((a & b) == uint128(0x0000000000000000, 0x0000000000000000));
		CHECK((a | b) == uint128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF));
		CHECK((a ^ b) == uint128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF));
		CHECK((~uint128(0)) == uint128(UINT64_MAX, UINT64_MAX));
	}

	SECTION("shifts")
	{
		uint128 a(1, 0);
		CHECK((a >> 64) == uint128(0, 1));
		CHECK((a >> 65) == uint128(0, 0));
		CHECK((uint128(0, 1) << 64) == uint128(1, 0));
		CHECK((uint128(0, 1) << 1) == uint128(0, 2));
	}

	SECTION("div_mod")
	{
		uint128 value(0, 1000);
		uint128 divisor(0, 3);
		CHECK(value / divisor == uint128(0, 333));
		CHECK(value % divisor == uint128(0, 1));

		uint128 large(1, 0);
		CHECK(large / uint128(0, 2) == uint128(0, 0x8000'0000'0000'0000));
		CHECK(large % uint128(0, 2) == uint128(0));
		CHECK(uint128(0, 9) / uint128(0, 3) == uint128(0, 3));
		CHECK(uint128(0, 9) % uint128(0, 3) == uint128(0));
		CHECK(uint128(0, 9) / uint128(0, 2) == uint128(0, 4));
		CHECK(uint128(0, 9) % uint128(0, 2) == uint128(0, 1));
	}

	SECTION("to_string")
	{
		CHECK(uint128(0).to_string() == "0");
		CHECK(uint128(1, 0).to_string() == "18446744073709551616");
		CHECK(uint128("123456789012345678901").to_string() == "123456789012345678901");
	}

	SECTION("mul")
	{
		uint128 a = uint128(0xFF'FF12'3567'8931) * uint128(0x12'3568'FFF4'5656);
		CHECK(a == uint128("369309726874948344242154908060790"));
		CHECK(uint128(0, 10) * uint128(0, 20) == uint128(0, 200));
		CHECK(uint128(0, 0) * uint128(0, 12345) == uint128(0));
		CHECK(uint128(1, 0) * uint128(0, 2) == uint128(2, 0));
	}
}
