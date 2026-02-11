#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>


import std;
import deckard.uint128;
using namespace deckard;

TEST_CASE("uint128", "[uint128]")
{
	SECTION("ctor")
	{
		uint128 a(1, 2);
		CHECK(a == uint128(1, 2));

		uint128 b(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF);
		CHECK(b == uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));

		uint128 c("18446744073709551616");
		CHECK(c == uint128(1, 0));

		uint128 empty("");
		CHECK(empty == uint128(0));

		uint128 hex_prefix_only("0x");
		CHECK(hex_prefix_only == uint128(0));
	}

	SECTION("add")
	{
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
		uint128 a2(1, 0);
		uint128 b2(0, UINT64_MAX);
		CHECK(a2 + b2 == uint128(1, UINT64_MAX));
		CHECK(uint128(0, 5) + uint128(0, 7) == uint128(0, 12));
		CHECK(uint128(1, 0) + uint128(0, 1) == uint128(1, 1));

		uint128 max1(UINT64_MAX, UINT64_MAX);
		uint128 one(0, 1);
		CHECK(max1 + one == uint128(0, 0)); // Should wrap to zero


		uint128 c(0, UINT64_MAX);
		uint128 d(1, 0);
		uint128 sum = c + d;
		CHECK(sum == uint128(1, UINT64_MAX));

		sum = sum + uint128(0, 2);
		CHECK(sum == uint128(2, 1));
		uint128 dc(UINT64_MAX - 1, UINT64_MAX);
		CHECK(dc + 1u == uint128(UINT64_MAX, 0));

		CHECK(uint128(UINT64_MAX, UINT64_MAX) + 1 == uint128());
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

		uint128 zero(0);
		CHECK(zero - one == uint128(UINT64_MAX, UINT64_MAX));

		CHECK(zero - 1 == uint128(UINT64_MAX, UINT64_MAX));
	}

	SECTION("parse")
	{
		uint128 value("18446744073709551616");
		CHECK(value == uint128(1, 0));


		uint128 zero_bad("12a34");
		CHECK(zero_bad == uint128(0));

		uint128 hex_value("0x1FFFFFFFFFFFFFFFF");
		CHECK(hex_value == uint128(0x1, 0xFFFF'FFFF'FFFF'FFFF));

		uint128 max_uint64_hex("0xFFFFFFFFFFFFFFFF");
		CHECK(max_uint64_hex == uint128(0, 0xFFFF'FFFF'FFFF'FFFF));

		uint128 max_uint64_dec("18446744073709551615");
		CHECK(max_uint64_dec == uint128(0, 0xFFFF'FFFF'FFFF'FFFF));
		CHECK(max_uint64_hex == max_uint64_dec);

		uint128 max_hex("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
		CHECK(max_hex == uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));

		uint128 max_dec("340282366920938463463374607431768211455");
		CHECK(max_dec == uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));

		uint128 zero_dec("0");
		CHECK(zero_dec == uint128(0));

		uint128 zero_hex("0x0");
		CHECK(zero_hex == uint128(0));

		uint128 leading_zeros("00000042");
		CHECK(leading_zeros == uint128(0, 42));

		uint128 leading_zeros_hex("0x0000002A");
		CHECK(leading_zeros_hex == uint128(0, 42));

		uint128 invalid_hex("0xG");
		CHECK(invalid_hex == uint128(0));

		uint128 invalid_dec("-1");
		CHECK(invalid_dec == uint128(0));

		CHECK(max_hex == max_dec);
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
		uint128 a(0xFF00'FF00'FF00'FF00, 0x0F0F'0F0F'0F0F'0F0F);
		uint128 b(0x00FF'00FF'00FF'00FF, 0xF0F0'F0F0'F0F0'F0F0);
		CHECK((a & b) == uint128(0x0000'0000'0000'0000, 0x0000'0000'0000'0000));
		CHECK((a | b) == uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
		CHECK((a ^ b) == uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
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

	SECTION("mul")
	{
		uint128 a = uint128(0xFF'FF12'3567'8931) * uint128(0x12'3568'FFF4'5656);
		CHECK(a == uint128("369309726874948344242154908060790"));
		CHECK(uint128(0, 10) * uint128(0, 20) == uint128(0, 200));
		CHECK(uint128(0, 0) * uint128(0, 12345) == uint128(0));
		CHECK(uint128(1, 0) * uint128(0, 2) == uint128(2, 0));
		CHECK(uint128(0, UINT64_MAX) * uint128(0, UINT64_MAX) == uint128("340282366920938463426481119284349108225"));


		uint128 c("0xFFFFFFFFFFFFFFFF");
		uint128 d("0xFFFFFFFFFFFFFFFF");
		CHECK(c * d == uint128("340282366920938463426481119284349108225"));
	}

	SECTION("convert to floats")
	{
		CHECK_THAT(f64{uint128("123456789")}, Catch::Matchers::WithinAbs(123456789.0, 0.0001));

		CHECK_THAT(f32{uint128("123456789")}, Catch::Matchers::WithinAbs(123456789.0f, 0.0001f));
	}

	SECTION("to_string")
	{
		CHECK(uint128(0).to_string() == "0");
		CHECK(uint128(1, 0).to_string() == "18446744073709551616");
		CHECK(uint128("123456789012345678901").to_string() == "123456789012345678901");

		CHECK(uint128(UINT64_MAX, UINT64_MAX).to_string() == "340282366920938463463374607431768211455");
		CHECK(uint128(UINT64_MAX, UINT64_MAX).to_string(16) == "0xffffffffffffffffffffffffffffffff");
		CHECK(uint128(UINT64_MAX, UINT64_MAX).to_string(2) ==
			  "0b11111111111111111111111111111111111111111111111111111111111111111111111111"
			  "111111111111111111111111111111111111111111111111111111");

		CHECK(uint128(0, 255).to_string(16) == "0xff");
		CHECK(uint128(0, 255).to_string(2) == "0b11111111");
		CHECK(uint128(0, 35).to_string(36) == "z");
		CHECK(uint128(1, 0).to_string(16) == "0x10000000000000000");

		CHECK(uint128(uint128(0, UINT64_MAX) * uint128(0, UINT64_MAX) - 1u).to_string(2) ==
			  "0b111111111111111111111111111111111111111111111111111111111111111"
			  "00000000000000000000000000000000000000000000000000000000000000000");
	}
}
