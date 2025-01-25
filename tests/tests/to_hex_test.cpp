#include <catch2/catch_test_macros.hpp>

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
		CHECK(to_hex_string<u32>({&value, 1}, {.endian_swap = false, .lowercase = true, .show_hex = true}) == "0xefbeadde"s);
		CHECK(to_hex_string<u32>({&value, 1}, {.lowercase = false, .show_hex = true}) == "0xDEADBEEF"s);
	}

	SECTION("array of u32 values")
	{
		std::array<u32, 2> values{0x1122'3344, 0xDEAD'BEEF};
		CHECK(to_hex_string<u32>(values) == "0x11223344, 0xDEADBEEF"s);
		CHECK(to_hex_string<u32>(values, {.show_hex = false}) == "11223344, DEADBEEF"s);
		CHECK(to_hex_string<u32>(values, {.delimiter = "-", .show_hex = true}) == "0x11223344-0xDEADBEEF"s);
	}


	SECTION("array of u8 values")
	{
		std::array<u8, 8> values{1, 2, 3, 4, 5, 6, 7, 8};

		CHECK(to_hex_string<u8>(values) == "0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08"s);

		HexOption option{.delimiter = "<$>", .show_hex = false};
		CHECK(to_hex_string<u8>(values, option) == "01<$>02<$>03<$>04<$>05<$>06<$>07<$>08"s);
	}

	SECTION("normal string (hello world)")
	{
		std::string value = "Hello world";

		CHECK(to_hex_string(value) == "0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 0x6F, 0x72, 0x6C, 0x64"s);

		HexOption option{.delimiter = "", .lowercase = true, .show_hex = false};
		CHECK(to_hex_string(value, option) == "48656c6c6f20776f726c64"s);

		option = {.delimiter = "/", .lowercase = false, .show_hex = false};
		CHECK(to_hex_string(value, option) == "48/65/6C/6C/6F/20/77/6F/72/6C/64"s);


		value = "Hello 🌍";
		CHECK(to_hex_string(value) == "0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0xF0, 0x9F, 0x8C, 0x8D"s);
	}
}
