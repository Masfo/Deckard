
#include <catch2/catch_test_macros.hpp>

import deckard.enums;
import deckard.types;
import std;

using namespace deckard;

namespace EnumFlagTest
{
	namespace Filesystem
	{
		enum class Permission : u8
		{
			No      = BIT<u8>(0),
			Read    = BIT<u8>(1),
			Write   = BIT<u8>(2),
			Execute = BIT<u8>(3),
		};
		consteval void enable_bitmask_operations(Permission);
	} // namespace Filesystem
} // namespace EnumFlagTest

TEST_CASE("enumflags", "[enum]")
{

	using EnumFlagTest::Filesystem::Permission;

	Permission rwx{};

	// Default flag
	REQUIRE(rwx == (Permission::No));

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
	REQUIRE(rwx == (Permission::No));

	// Add all back traditional way
	rwx |= Permission::Read | Permission::Write | Permission::Execute;
	REQUIRE(rwx == (Permission::Read | Permission::Write | Permission::Execute));


	// Toggle read
	rwx ^= Permission::Read;
	REQUIRE(rwx == (Permission::Write | Permission::Execute));

	// Toggle write and execute
	rwx ^= Permission::Write | Permission::Execute;
	REQUIRE(rwx == (Permission::No));

	// TODO: add format printing when it is supported
}
