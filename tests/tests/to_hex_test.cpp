#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;

import deckard.types;
import deckard.helpers;
import std;

using namespace deckard;
using namespace std::string_literals;

TEST_CASE("to_hex", "[to_hex]")
{
	SECTION("single u32 value")
	{
		u32 value = 0x1122'3344;
		CHECK(to_hex_string<u32>({&value, 1}) == "0x11223344"s);
		CHECK(to_hex_string<u32>({&value, 1}, {.show_hex = false}) == "11223344"s);

		value = 0xDEAD'BEEF;
		CHECK(to_hex_string<u32>({&value, 1}, {.lowercase = true, .show_hex = false}) == "deadbeef"s);
		CHECK(to_hex_string<u32>({&value, 1}, {.byteswap = false, .lowercase = true, .show_hex = true}) == "0xefbeadde"s);
		CHECK(to_hex_string<u32>({&value, 1}, {.lowercase = false, .show_hex = true}) == "0xDEADBEEF"s);
	}

	SECTION("array of u32 values")
	{
		std::array<u32, 2> values{0x1122'3344, 0xDEAD'BEEF};
		REQUIRE(to_hex_string<u32>(values) == "0x11223344, 0xDEADBEEF"s);
		REQUIRE(to_hex_string<u32>(values, {.show_hex = false}) == "11223344, DEADBEEF"s);
		REQUIRE(to_hex_string<u32>(values, {.delimiter = "-", .show_hex = true}) == "0x11223344-0xDEADBEEF"s);
	}
}
