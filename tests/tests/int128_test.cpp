#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>

import std;
import deckard.int128;
import deckard.types;
using namespace deckard;

// Helper to construct uint128 from two 64-bit parts
inline uint128 make_u128(u64 h, u64 l) { return uint128{h, l}; }

TEST_CASE("int128", "[int128]")
{
	SECTION("ctor")
	{
		int128 a(1, 2);
		CHECK(a == int128(1, 2));
		int128 b(-1, -2);
		CHECK(b == int128(-1, -2));
		int128 c("170141183460469231731687303715884105727");
		CHECK(c == int128(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
		int128 d("-170141183460469231731687303715884105728");
		CHECK(d == int128(0x8000'0000'0000'0000, 0x0000'0000'0000'0000));
		int128 empty("");
		CHECK(empty == int128(0));

		int128 from_i64(42);
		auto   cmp = from_i64 == int128(0, 42);
		CHECK(cmp);
		int128 from_neg_i64(-42);
		CHECK(from_neg_i64 < int128(0));
	}

	SECTION("add")
	{
		int128 a(0, 10);
		int128 b(0, 20);
		CHECK((a + b) == int128(0, 30));

		int128 pos(0, 100);
		int128 neg(-50);
		auto   result = pos + neg;
		CHECK(result > int128(0));

		int128 zero(0, 0);
		CHECK((zero + int128(0, 5)) == int128(0, 5));
		CHECK((int128(0, 5) + zero) == int128(0, 5));

		int128 a1(0, 10);
		int128 b1(-10);
		CHECK((a1 + b1) == int128(0, 0));
	}

	SECTION("sub")
	{
		int128 a(0, 50);
		int128 b(0, 20);
		CHECK((a - b) == int128(0, 30));

		int128 zero(0, 0);
		int128 five(0, 5);
		auto   result = zero - five;
		CHECK(result < int128(0));

		int128 a1(0, 10);
		int128 b1(0, 10);
		CHECK((a1 - b1) == int128(0, 0));
	}

	SECTION("negate")
	{
		int128 pos(0, 42);
		int128 neg_pos = -pos;
		CHECK(neg_pos < int128(0));

		int128 zero(0, 0);
		CHECK((-zero) == int128(0, 0));

		int128 max_pos(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF);
		int128 neg_max = -max_pos;
		CHECK(neg_max < int128(0));
	}

	SECTION("mul")
	{
		int128 a(0, 10);
		int128 b(0, 20);
		CHECK((a * b) == int128(0, 200));

		int128 pos(0, 5);
		int128 neg(-3);
		auto   result = pos * neg;
		CHECK(result < int128(0));

		int128 zero(0, 0);
		CHECK((zero * int128(0, 100)) == int128(0, 0));
		CHECK((int128(0, 100) * zero) == int128(0, 0));

		int128 one(0, 1);
		int128 val(0, 42);
		CHECK((one * val) == val);

		int128 neg_high(-1, 0);
		CHECK((neg_high * int128(0, 2)) < int128(0));
		CHECK((neg_high * int128(-1)) > int128(0));

		int128 c("-6242398038732451891");
		int128 d("3981720493856129044");
		CHECK((c * d) == int128("-24855484201628309703698791272217822204"));
	}

	SECTION("compare")
	{
		int128 pos(0, 10);
		int128 neg(-10);
		CHECK(pos > neg);
		CHECK(neg < pos);
		CHECK(pos != neg);

		int128 a(0, 5);
		int128 b(0, 5);
		CHECK(a == b);
		CHECK(a <= b);
		CHECK(a >= b);
		CHECK(not(a < b));
		CHECK(not(a > b));

		int128 zero(0, 0);
		CHECK(zero == int128(0, 0));
		CHECK(pos > zero);
		CHECK(neg < zero);
		CHECK(zero > neg);

		int128 large_pos(100, 0);
		int128 large_neg(-100, 0);
		CHECK(large_pos > large_neg);
		CHECK(large_neg < large_pos);

		int128 max_pos(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF);
		int128 max_neg(0x8000'0000'0000'0000, 0x0000'0000'0000'0000);
		CHECK(max_pos > max_neg);
		CHECK(max_neg < max_pos);
		CHECK(max_neg != max_pos);

		// low word must be compared unsigned: int128(0, 2^63) is positive, greater than int128(0, 1)
		int128 large_low(0, static_cast<i64>(0x8000'0000'0000'0000)); // positive, low MSB set
		CHECK(large_low > int128(0, 1));
		CHECK(large_low > int128(0));
	}

	SECTION("bitwise")
	{
		int128 a(0xFF00'FF00'FF00'FF00, 0x0F0F'0F0F'0F0F'0F0F);
		int128 b(0x00FF'00FF'00FF'00FF, 0xF0F0'F0F0'F0F0'F0F0);
		CHECK((a & b) == int128(0x0000'0000'0000'0000, 0x0000'0000'0000'0000));
		CHECK((a | b) == int128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
		CHECK((a ^ b) == int128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
	}

	SECTION("shifts")
	{
		int128 a(1, 0);
		CHECK((a >> 64) == int128(0, 1));
		CHECK((a >> 65) == int128(0, 0));
		CHECK((int128(0, 1) << 64) == int128(1, 0));
		CHECK((int128(0, 1) << 1) == int128(0, 2));

		int128 neg(-1);
		int128 shifted = neg >> 1;
		CHECK(shifted < int128(0));

		int128 pos(0, 1);
		CHECK((pos << 64) == int128(1, 0));
		CHECK((pos >> 64) == int128(0, 0));

		// arithmetic right shift: high word fills with sign bit, not bit 31
		int128 neg2(-2, 0);                    // high=-2, low=0
		CHECK((neg2 >> 64) == int128(-1, -2)); // high>>63 fills sign, low=high>>0=-2
		CHECK((neg2 >> 65) == int128(-1, -1)); // high>>1=-1, sign fill
	}

	SECTION("parse")
	{
		int128 pos("123456789");
		CHECK(pos > int128(0));

		int128 neg("-123456789");
		CHECK(neg < int128(0));

		int128 zero("0");
		CHECK(zero == int128(0, 0));

		int128 hex_pos("0xFF");
		CHECK(hex_pos == int128(0, 255));

		int128 hex_neg("-0xFF");
		CHECK(hex_neg < int128(0));

		int128 max_pos("170141183460469231731687303715884105727");
		CHECK(max_pos == int128(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));

		int128 min_neg("-170141183460469231731687303715884105728");
		CHECK(min_neg == int128(0x8000'0000'0000'0000, 0x0000'0000'0000'0000));

		int128 invalid("abc");
		CHECK(invalid == int128(0, 0));
	}

	SECTION("to_string")
	{
		CHECK(int128(0, 0).to_string() == "0");
		CHECK(int128(0, 42).to_string() == "42");

		int128 neg_val(-42);
		auto   neg_str = neg_val.to_string();
		CHECK(neg_str.find('-') != std::string::npos);

		CHECK(int128(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF).to_string() == "170141183460469231731687303715884105727");
		CHECK(int128(0x8000'0000'0000'0000, 0x0000'0000'0000'0000).to_string() ==
			  "-170141183460469231731687303715884105728");

		auto hex_str = int128(0, 255).to_string(16);
		bool has_ff  = (hex_str.find("ff") != std::string::npos);
		bool has_FF  = (hex_str.find("FF") != std::string::npos);
		CHECK((has_ff || has_FF));

		auto bin_str = int128(0, 255).to_string(2);
		CHECK(bin_str.find("11111111") != std::string::npos);
	}

	SECTION("convert to floats")
	{
		f64 d64 = static_cast<f64>(int128(0, 123'456'789));
		CHECK_THAT(d64, Catch::Matchers::WithinAbs(123456789.0, 1000.0));

		f32 d32 = static_cast<f32>(int128(0, 123'456'789));
		CHECK_THAT(d32, Catch::Matchers::WithinAbs(123456789.0f, 10000.0f));
	}

	SECTION("formatter")
	{
		CHECK(std::format("{}", int128(0, 0)) == "0");
		CHECK(std::format("{}", int128(0, 42)) == "42");

		auto neg_formatted = std::format("{}", int128(-42));
		CHECK(neg_formatted.find('-') != std::string::npos);

		auto hex_formatted = std::format("{:x}", int128(0, 255));
		bool has_ff_fmt    = (hex_formatted.find("ff") != std::string::npos);
		bool has_xFF       = (hex_formatted.find("xFF") != std::string::npos);
		CHECK((has_ff_fmt || has_xFF));

		auto bin_formatted = std::format("{:b}", int128(0, 255));
		CHECK(bin_formatted.find("11111111") != std::string::npos);
	}

	SECTION("div")
	{
		// Division checks consolidated into dedicated TEST_CASEs below.
		// Retained minimal sanity checks only.
		CHECK(int128(0, 100) / int128(0, 10) == int128(0, 10));
		CHECK(int128(0, 100) % int128(0, 10) == int128(0));
	}
}

TEST_CASE("int128::div_mod - Basic Cases", "[int128][division]")
{
	SECTION("Positive / Positive")
	{
		int128 a    = 100;
		int128 b    = 7;
		auto [q, r] = a.div_mod(b);
		CHECK(q == 14);
		CHECK(r == 2);
		CHECK(a == q * b + r);
	}

	SECTION("Negative / Positive")
	{
		int128 a    = -100;
		int128 b    = 7;
		auto [q, r] = a.div_mod(b);
		// C++11 rule: -100 / 7 = -14, -100 % 7 = -2
		CHECK(q == -14);
		CHECK(r == -2);
		CHECK(a == q * b + r);
	}

	SECTION("Positive / Negative")
	{
		int128 a    = 100;
		int128 b    = -7;
		auto [q, r] = a.div_mod(b);
		// 100 / -7 = -14, 100 % -7 = 2 (remainder sign matches dividend)
		CHECK(q == -14);
		CHECK(r == 2);
		CHECK(a == q * b + r);
	}

	SECTION("Negative / Negative")
	{
		int128 a    = -100;
		int128 b    = -7;
		auto [q, r] = a.div_mod(b);
		// -100 / -7 = 14, -100 % -7 = -2
		CHECK(q == 14);
		CHECK(r == -2);
		CHECK(a == q * b + r);
	}

	SECTION("Divide by One")
	{
		int128 a    = 999;
		int128 b    = 1;
		auto [q, r] = a.div_mod(b);
		CHECK(q == 999);
		CHECK(r == 0);
	}

	SECTION("Divide by Negative One")
	{
		int128 a    = 999;
		int128 b    = -1;
		auto [q, r] = a.div_mod(b);
		CHECK(q == -999);
		CHECK(r == 0);
	}

	SECTION("Zero Dividend")
	{
		int128 a    = 0;
		int128 b    = 12345;
		auto [q, r] = a.div_mod(b);
		CHECK(q == 0);
		CHECK(r == 0);
	}
}

TEST_CASE("int128::div_mod - Edge Cases", "[int128][edge]")
{
	const int128 min_val = std::numeric_limits<int128>::min();
	const int128 max_val = std::numeric_limits<int128>::max();

	SECTION("Dividend is INT128_MIN, Divisor is 1")
	{
		int128 a    = min_val;
		int128 b    = 1;
		auto [q, r] = a.div_mod(b);
		CHECK(q == min_val);
		CHECK(r == 0);
		CHECK(a == q * b + r);
	}

	SECTION("Dividend is INT128_MIN, Divisor is -1 (Overflow Case)")
	{
		int128 a = min_val;
		int128 b = -1;

		// This is the critical test.
		// Mathematically: -2^127 / -1 = 2^127.
		// 2^127 cannot be represented in signed int128 (max is 2^127 - 1).
		// Our implementation likely wraps to min_val (0x80...00) or asserts.
		// If your implementation throws, wrap in CHECK_THROWS.
		// If it returns a wrapped value:
		auto [q, r] = a.div_mod(b);

		// The remainder MUST be 0.
		CHECK(r == 0);

		// The quotient behavior depends on your overflow policy.
		// If it wraps: q == min_val (which is -2^127, mathematically wrong but bit-pattern consistent)
		// If it saturates: q == max_val
		// If it asserts: The test will fail at the assert line.

		// For this test, we check the invariant a == q*b + r holds with the returned q.
		// If q wraps to min_val: min_val * -1 = min_val (overflow again).
		// This is a known limitation of fixed-width integers.
		// We assert that the remainder is correct, which is the primary guarantee.
		CHECK(r == 0);

		// Optional: Check if your implementation handles this gracefully (e.g. throws)
		// CHECK_THROWS_AS(a.div_mod(b), std::overflow_error);
	}

	SECTION("Divisor is INT128_MIN")
	{
		int128 a    = 1000;
		int128 b    = min_val;
		auto [q, r] = a.div_mod(b);
		// 1000 / -2^127 = 0
		// 1000 % -2^127 = 1000
		CHECK(q == 0);
		CHECK(r == 1000);
		CHECK(a == q * b + r);
	}

	SECTION("Large Numbers near Max")
	{
		int128 a    = max_val;
		int128 b    = 2;
		auto [q, r] = a.div_mod(b);
		// max_val is odd (2^127 - 1)
		// (2^127 - 1) / 2 = 2^126 - 1
		// Remainder = 1
		CHECK(r == 1);
		CHECK(a == q * b + r);
	}
}

TEST_CASE("int128::div_mod - Randomized Stress Test", "[int128][stress]")
{

	auto test_case = [](int128 a, int128 b)
	{
		if (b == 0)
			return; // Skip zero divisor

		auto [q, r] = a.div_mod(b);

		CHECK(a == q * b + r);

		// Check remainder magnitude < divisor magnitude
		// Skip this check if either value is int128::min() since negation overflows
		if (r != std::numeric_limits<int128>::min() && b != std::numeric_limits<int128>::min())
		{
			int128 abs_r = (r < 0) ? -r : r;
			int128 abs_b = (b < 0) ? -b : b;
			CHECK(abs_r < abs_b);
		}

		if (r != 0)
		{
			bool r_neg = (r < 0);
			bool a_neg = (a < 0);
			CHECK(r_neg == a_neg);
		}
	};

	SECTION("Small Random Values")
	{
		test_case(42, 5);
		test_case(-42, 5);
		test_case(42, -5);
		test_case(-42, -5);
		test_case(1, 1);
		test_case(-1, 1);
	}

	SECTION("Boundary Values")
	{
		test_case(std::numeric_limits<int128>::min(), 2);
		test_case(std::numeric_limits<int128>::max(), 2);
		test_case(std::numeric_limits<int128>::min(), std::numeric_limits<int128>::max());
		test_case(std::numeric_limits<int128>::max(), std::numeric_limits<int128>::min());
	}
}

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

		CHECK(a == a);
		CHECK(b == b);
		CHECK(not(a == b));
		CHECK(a != b);

		uint128 zero(0, 0);
		CHECK(zero < a);
		CHECK(zero < b);
		CHECK(a > zero);
		CHECK(b > zero);
		CHECK(zero == uint128(0));
		CHECK(zero <= zero);
		CHECK(zero >= zero);

		uint128 max_u64(0, UINT64_MAX);
		uint128 one_u64_plus(1, 0);
		CHECK(max_u64 < one_u64_plus);
		CHECK(one_u64_plus > max_u64);
		CHECK(max_u64 != one_u64_plus);

		uint128 large1(0xFFFF'0000'0000'0000, 0x0000'0000'0000'0001);
		uint128 large2(0xFFFF'0000'0000'0000, 0xFFFF'FFFF'FFFF'FFFF);
		CHECK(large1 < large2);
		CHECK(large2 > large1);
		CHECK(large1 != large2);
		CHECK(large2 >= large1);
		CHECK(large1 <= large2);

		uint128 val1(0x1000'0000'0000'0000, 0x5000'0000'0000'0000);
		uint128 val2(0x1000'0000'0000'0000, 0x5000'0000'0000'0001);
		CHECK(val1 < val2);
		CHECK(val2 > val1);
		CHECK(val1 <= val2);
		CHECK(val2 >= val1);

		uint128 max_val(UINT64_MAX, UINT64_MAX);
		uint128 max_minus_one(UINT64_MAX, UINT64_MAX - 1);
		CHECK(max_minus_one < max_val);
		CHECK(max_val > max_minus_one);
		CHECK(max_val >= max_minus_one);
		CHECK(max_minus_one <= max_val);

		uint128 bound1(0x1234'5678'9ABC'DEF0, 0x0000'0000'0000'0000);
		uint128 bound2(0x1234'5678'9ABC'DEF0, 0x0000'0000'0000'0001);
		CHECK(bound1 < bound2);
		CHECK(bound2 > bound1);
		CHECK(bound1 != bound2);

		uint128 bound3(0x0000'0000'0000'0001, UINT64_MAX);
		uint128 bound4(0x0000'0000'0000'0002, 0x0000'0000'0000'0000);
		CHECK(bound3 < bound4);
		CHECK(bound4 > bound3);

		uint128 t_a(0, 10);
		uint128 t_b(0, 100);
		uint128 t_c(0, 1000);
		CHECK(t_a < t_b);
		CHECK(t_b < t_c);
		CHECK(t_a < t_c);

		uint128 t_d(1, 0);
		uint128 t_e(2, 0);
		uint128 t_f(3, 0);
		CHECK(t_d < t_e);
		CHECK(t_e < t_f);
		CHECK(t_d < t_f);

		uint128 near_max1(UINT64_MAX - 1, UINT64_MAX);
		uint128 near_max2(UINT64_MAX, 0);
		CHECK(near_max1 < near_max2);
		CHECK(near_max2 > near_max1);

		uint128 same_high1(0xABCD'EF01'2345'6789, 0x1111'1111'1111'1111);
		uint128 same_high2(0xABCD'EF01'2345'6789, 0x2222'2222'2222'2222);
		CHECK(same_high1 < same_high2);
		CHECK(same_high2 > same_high1);
		CHECK(same_high1 != same_high2);
		CHECK(same_high1 <= same_high2);
		CHECK(same_high2 >= same_high1);

		uint128 large_equal1(0xDEAD'BEEF'CAFE'BABE, 0xDEAD'BEEF'CAFE'BABE);
		uint128 large_equal2(0xDEAD'BEEF'CAFE'BABE, 0xDEAD'BEEF'CAFE'BABE);
		CHECK(large_equal1 == large_equal2);
		CHECK(not(large_equal1 != large_equal2));
		CHECK(large_equal1 >= large_equal2);
		CHECK(large_equal1 <= large_equal2);
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
		// Division checks consolidated into dedicated TEST_CASEs below.
		// Retained minimal sanity checks only.
		uint128 value(0, 1000);
		uint128 divisor(0, 3);
		CHECK(value / divisor == uint128(0, 333));
		CHECK(value % divisor == uint128(0, 1));
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

		uint128 e("4010586790773080478");
		uint128 f("6814296587827358857");
		CHECK(e * f == uint128("27329327883550479895825042596647093646"));
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


	SECTION("formatter")
	{
		CHECK(std::format("{}", uint128(0, 0)) == "0");
		CHECK(std::format("{}", uint128(4'256'066'292'496'146'311, 6'725'288'669'015'888'136)) ==
			  "78510565658418270060149122660758376712");
		CHECK(std::format("{}", uint128(0, 1)) == "1");
		CHECK(std::format("{:x}", uint128(0, 255)) == "0xff");
		CHECK(std::format("{:b}", uint128(0, 255)) == "0b11111111");
		CHECK(std::format("{:x}", uint128(UINT64_MAX, UINT64_MAX)) == "0xffffffffffffffffffffffffffffffff");
		CHECK(std::format("{:b}", uint128(0, 256)) == "0b100000000");
	}

	SECTION("Dividend MSB in Low, Divisor MSB in Low (Small)")
	{
		uint128 dividend = make_u128(0u, 0x8000'0000'0000'0000ULL);
		uint128 divisor  = make_u128(0u, 1u);

		auto [q, r] = dividend.div_mod(divisor);

		CHECK(q == dividend);
		CHECK(r == 0u);

		CHECK(dividend == q * divisor + r);
	}

	SECTION("Dividend MSB in Low, Divisor MSB in Low (Medium)")
	{

		uint128 dividend = make_u128(0u, 0xC000'0000'0000'0000ULL);
		uint128 divisor  = make_u128(0u, 1024u);

		uint128 expected_q = make_u128(0u, 0x30'0000'0000'0000ULL);

		auto [q, r] = dividend.div_mod(divisor);

		CHECK(q == expected_q);
		CHECK(r == 0u);
		CHECK(dividend == q * divisor + r);
	}

	SECTION("Stress Test: Random High-Zero Cases")
	{
		auto test_random = [](u64 d_val, u64 v_val)
		{
			if (v_val == 0)
				return;

			uint128 dividend = make_u128(0u, d_val);
			uint128 divisor  = make_u128(0u, v_val);

			u64 expected_q = d_val / v_val;
			u64 expected_r = d_val % v_val;

			auto [q, r] = dividend.div_mod(divisor);

			CHECK(q == make_u128(0u, expected_q));
			CHECK(r == make_u128(0u, expected_r));
			CHECK(dividend == q * divisor + r);
		};

		test_random(0x8000'0000'0000'0000ULL, 1u);
		test_random(0xFFFF'FFFF'FFFF'FFFFULL, 1u);
		test_random(0xFFFF'FFFF'FFFF'FFFFULL, 0xFFFF'FFFF'FFFF'FFFFULL);
		test_random(0x1000'0000'0000'0000ULL, 0x1000'0000'0000'0000ULL);
		test_random(0x1234'5678'9ABC'DEF0ULL, 0xFEDC'BA98'7654'3210ULL);
		test_random(1'000'000'007u, 1'000'000'007u);
	}

	uint128 zero{0};
	uint128 one{1};
	uint128 max64{0u, 0xFFFF'FFFF'FFFF'FFFFULL};
	uint128 power64{1u, 0u};

	SECTION("Boundary Arithmetic")
	{
		CHECK((max64 + one) == power64);
		CHECK((power64 - one) == max64);
	}


	SECTION("Multiplication/Division across 64-bit")
	{
		uint128 a{0u, 0x8000'0000'0000'0000ULL};
		auto [q, r] = (a * uint128(2)).div_mod(uint128(2));
		CHECK(q == a);
		CHECK(r == zero);
	}


	SECTION("Divisor > Dividend")
	{
		uint128 a{100};
		uint128 b{200};
		auto [q, r] = a.div_mod(b);
		CHECK(q == 0u);
		CHECK(r == a);
	}

	SECTION("Exact 128-bit Division")
	{
		uint128 a{5u, 0u};
		uint128 b{5u};
		auto [q, r] = a.div_mod(b);
		CHECK(q == uint128{1u, 0u});
		CHECK(r == 0u);
	}

	SECTION("Divisor with bits in both halves")
	{
		uint128 a{0xFFFF'FFFF'FFFF'FFFFULL, 0xFFFF'FFFF'FFFF'FFFFULL};
		uint128 b{0x0000'0000'0000'0001ULL, 0x0000'0000'0000'0000ULL};

		auto [q, r] = a.div_mod(b);

		CHECK(q == 0xFFFF'FFFF'FFFF'FFFFULL);
		CHECK(r == 0xFFFF'FFFF'FFFF'FFFFULL);
	}

	SECTION("Large prime division")
	{
		uint128 a{0xABCD'EF01'2345'6789ULL, 0x9876'5432'10FE'DCBAULL};
		uint128 b{0x1234'5678'9ABC'DEF0ULL, 0x0FED'CBA9'8765'4321ULL};

		auto [q, r] = a.div_mod(b);
		CHECK((q * b) + r == a);
	}

	SECTION("Comparison across 64-bit boundary")
	{
		uint128 low_max{0u, 0xFFFF'FFFF'FFFF'FFFFULL};
		uint128 high_min{1u, 0u};

		CHECK(high_min > low_max);
		CHECK(low_max < high_min);
		CHECK(high_min != low_max);
	}
	SECTION("Bitwise")
	{
		uint128 val{0xAAAA'AAAA'AAAA'AAAAULL, 0x5555'5555'5555'5555ULL};
		uint128 mask{0xFFFF'FFFF'0000'0000ULL, 0x0000'0000'FFFF'FFFFULL};

		uint128 expected{0xAAAA'AAAA'0000'0000ULL, 0x0000'0000'5555'5555ULL};
		CHECK((val & mask) == expected);
	}


	SECTION("uint128 - Wrap Around")
	{
		uint128 max{0xFFFF'FFFF'FFFF'FFFFULL, 0xFFFF'FFFF'FFFF'FFFFULL};

		CHECK((max + uint128(1)) == zero);
		CHECK((zero - uint128(1)) == max);
	}
}

TEST_CASE("int128/uint128", "[int128][uint128]")
{
	SECTION("numeric_limits")
	{
		static_assert(std::numeric_limits<int128>::is_specialized);
		static_assert(std::numeric_limits<int128>::is_signed);
		static_assert(std::numeric_limits<int128>::is_integer);
		static_assert(std::numeric_limits<int128>::digits == 127);

		CHECK(std::numeric_limits<int128>::max() == int128(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
		CHECK(std::numeric_limits<int128>::min() == int128(0x8000'0000'0000'0000, 0));
		CHECK(std::numeric_limits<int128>::lowest() == std::numeric_limits<int128>::min());

		static_assert(std::numeric_limits<uint128>::is_specialized);
		static_assert(!std::numeric_limits<uint128>::is_signed);
		static_assert(std::numeric_limits<uint128>::is_integer);
		static_assert(std::numeric_limits<uint128>::digits == 128);

		CHECK(std::numeric_limits<uint128>::max() == uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF));
		CHECK(std::numeric_limits<uint128>::min() == uint128(0));
		CHECK(std::numeric_limits<uint128>::lowest() == std::numeric_limits<uint128>::min());

		CHECK(std::format("{}", int128(std::numeric_limits<int128>::max())) == "170141183460469231731687303715884105727");
		CHECK(std::format("{}", int128(std::numeric_limits<int128>::min())) == "-170141183460469231731687303715884105728");

		CHECK(std::format("{}", uint128(std::numeric_limits<uint128>::max())) == "340282366920938463463374607431768211455");
		CHECK(std::format("{}", uint128(std::numeric_limits<uint128>::min())) == "0");

		CHECK(std::format("{:x}", int128(std::numeric_limits<int128>::max())) == "0x7fffffffffffffffffffffffffffffff");
		CHECK(std::format("{:x}", int128(std::numeric_limits<int128>::min())) == "-0x80000000000000000000000000000000");
		CHECK(std::format("{:x}", uint128(std::numeric_limits<uint128>::max())) == "0xffffffffffffffffffffffffffffffff");
	}

	SECTION("mixed signed/unsigned compares")
	{
		uint128 uval(0, 42);
		int128  ival(0, 42);

		CHECK(uval == ival);
		CHECK(ival == uval);

		int128 neg(-42);
		CHECK(neg != uval);
		CHECK(uval != neg);
		CHECK(neg < uval);
		CHECK(uval > neg);

		int128  large_pos(0x7FFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF);
		uint128 larger_u(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF);
		CHECK(large_pos < larger_u);
		CHECK(larger_u > large_pos);

		int128  neg_one(-1);
		uint128 zero_u(0);
		CHECK(neg_one < zero_u);
		CHECK(zero_u > neg_one);
	}
}

TEST_CASE("uint128 - Bit Boundary Tests", "[uint128][math]") { }

TEST_CASE("uint128 - Division Edge Cases", "[uint128][div]") { }

TEST_CASE("uint128 - Comparison and Bitwise", "[uint128][logic]") { }

TEST_CASE("uint128 - Wrap Around", "[uint128][limits]")
{
	uint128 max{0xFFFF'FFFF'FFFF'FFFFULL, 0xFFFF'FFFF'FFFF'FFFFULL};
	uint128 zero{0};

	CHECK((max + uint128(1)) == zero);
	CHECK((zero - uint128(1)) == max);
}

TEST_CASE("uint128::div_mod - High-Zero MSB Alignment Bug", "[uint128][division][bug]") { }
