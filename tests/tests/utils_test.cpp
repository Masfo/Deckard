#include <catch2/catch_test_macros.hpp>


import deckard.types;
import deckard.uuid;
import deckard.utf8;

using namespace deckard;

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

	//
}
