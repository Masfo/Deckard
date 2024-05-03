
#include <catch2/catch_test_macros.hpp>

import deckard.enums;
import deckard.types;
import std;

using namespace deckard;
using namespace std::string_literals;

namespace EnumFlagTest__
{
	namespace Filesystem
	{
		enum class Permission : u8
		{
			Read    = BIT(0),
			Write   = BIT(1),
			Execute = BIT(2),

			// Width = 3,
		};
		consteval void enable_bitmask_operations(Permission);

		template<EnumFlagType T>
		struct std::formatter<T> : formatter<int>
		{
			// parse is optional
			constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

			auto format(T f, format_context& ctx) const
			{
				//
				return std::format_to(ctx.out(), "{:03b}", std::to_underlying(f));
			}
		};

	}; // namespace Filesystem
} // namespace EnumFlagTest__

TEST_CASE("enumflags", "[enum]")
{

	using EnumFlagTest__::Filesystem::Permission;

	Permission rwx{};


	// Add read
	rwx += Permission::Read;
	REQUIRE(rwx == (Permission::Read));

	// Add write + execute
	rwx += Permission::Write | Permission::Execute;
	REQUIRE(rwx == (Permission::Read | Permission::Write | Permission::Execute));

	// Remove read and execute
	rwx -= Permission::Read | Permission::Execute;
	REQUIRE(rwx == (Permission::Write));

	// Remove write with traditional way
	rwx &= ~Permission::Write;

	// Add all back traditional way
	rwx |= Permission::Read | Permission::Write | Permission::Execute;
	REQUIRE(rwx == (Permission::Read | Permission::Write | Permission::Execute));


	// Toggle read
	rwx ^= Permission::Read;
	REQUIRE(rwx == (Permission::Write | Permission::Execute));

	// Toggle write and execute
	rwx ^= Permission::Write | Permission::Execute;

	// Reset
	rwx += Permission::Read | Permission::Write | Permission::Execute;

	// Check all flags, with boolean test
	//
	REQUIRE(true == (rwx && Permission::Read));
	REQUIRE(true == (rwx && Permission::Write));
	REQUIRE(true == (rwx && Permission::Execute));

	// Remove read
	rwx -= Permission::Read;
	REQUIRE(false == (rwx && Permission::Read));
	REQUIRE(true == (rwx && Permission::Write));
	REQUIRE(true == (rwx && Permission::Execute));

	// Remove write
	rwx -= Permission::Write;
	REQUIRE(false == (rwx && Permission::Read));
	REQUIRE(false == (rwx && Permission::Write));
	REQUIRE(true == (rwx && Permission::Execute));

	// Remove execute
	rwx -= Permission::Execute;
	REQUIRE(false == (rwx && Permission::Read));
	REQUIRE(false == (rwx && Permission::Write));
	REQUIRE(false == (rwx && Permission::Execute));

	// OR read
	rwx |= Permission::Read;
	REQUIRE(true == (rwx && Permission::Read));
	REQUIRE(false == (rwx && Permission::Write));
	REQUIRE(false == (rwx && Permission::Execute));

	//
	rwx += Permission::Write | Permission::Execute;

	// AND execute, removes all but execute
	rwx &= Permission::Execute;
	REQUIRE(false == (rwx && Permission::Read));
	REQUIRE(false == (rwx && Permission::Write));
	REQUIRE(true == (rwx && Permission::Execute));


	// TODO: add format printing when it is supported
	// TODO: this is just temp
	REQUIRE(std::format("{}", (Permission::Read)) == "001"s);
	REQUIRE(std::format("{}", (Permission::Write)) == "010"s);
	REQUIRE(std::format("{}", (Permission::Execute)) == "100"s);
	REQUIRE(std::format("{}", (Permission::Execute | Permission::Read)) == "101"s);
}
