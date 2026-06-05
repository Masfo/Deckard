
#include <catch2/catch_test_macros.hpp>

import deckard.enums;
import deckard.types;
import std;

using namespace deckard;
using namespace std::string_literals;

namespace EnumFlag__Test
{
	namespace Filesystem
	{
		enum class Permission : u8
		{
			Read    = BIT(0),
			Write   = BIT(1),
			Execute = BIT(2),


			Count = 3,
		};
#ifdef __cpp_impl_reflection
#error "Reflection is supported, use it to count enums"

		// template<typename E>
		// requires std::is_enum_v<E>
		// consteval auto enum_count() -> std::size_t
		// {
		// 	return std::meta::members_of(^^E).size();
		// }
		// 
		// 
		// width = temp > 0 ? temp : enum_count<Permission>();
		// width = temp > 0 ? temp : std::meta::members_of(^^Permission).size();
		

#endif
		consteval void enable_bitmask_operations(Permission);

		template<EnumFlagType T>
		struct std::formatter<T> : formatter<int>
		{
			u32 width{std::to_underlying(T::Count)};

			constexpr auto parse(std::format_parse_context& ctx)
			{
				auto pos  = ctx.begin();
				u32  temp = 0;
				while (pos != ctx.end() and *pos != '}')
				{
					temp *= 10;
					temp += (*pos - '0');
					++pos;
				}

				width = temp > 0 ? temp : std::to_underlying(T::Count);

				return pos;
			}

			auto format(T f, format_context& ctx) const
			{
				return std::format_to(ctx.out(), "{:0{}b}", std::to_underlying(f), width);
			}
		};

	}; // namespace Filesystem
} // namespace EnumFlag__Test

TEST_CASE("enumflags", "[enum]")
{

	using EnumFlag__Test::Filesystem::Permission;

	Permission rwx{};


	// Add read
	rwx += Permission::Read;
	CHECK(rwx == (Permission::Read));

	// Add write + execute
	rwx += Permission::Write | Permission::Execute;
	CHECK(rwx == (Permission::Read | Permission::Write | Permission::Execute));

	// Remove read and execute
	rwx -= Permission::Read | Permission::Execute;
	CHECK(rwx == (Permission::Write));

	// Remove write with traditional way
	rwx &= ~Permission::Write;

	// Add all back traditional way
	rwx |= Permission::Read | Permission::Write | Permission::Execute;
	CHECK(rwx == (Permission::Read | Permission::Write | Permission::Execute));


	// Toggle read
	rwx ^= Permission::Read;
	CHECK(rwx == (Permission::Write | Permission::Execute));

	// Toggle write and execute
	rwx ^= Permission::Write | Permission::Execute;

	// Reset
	rwx = Permission::Read | Permission::Write | Permission::Execute;

	// Check all flags, with boolean test
	//
	CHECK(true == has(rwx, Permission::Read));
	CHECK(true == has(rwx, Permission::Write));
	CHECK(true == has(rwx, Permission::Execute));

	// Remove read
	rwx -= Permission::Read;
	CHECK(false == has(rwx, Permission::Read));
	CHECK(true == has(rwx, Permission::Write));
	CHECK(true == has(rwx, Permission::Execute));

	// Remove write
	rwx -= Permission::Write;
	CHECK(false == has(rwx, Permission::Read));
	CHECK(false == has(rwx, Permission::Write));
	CHECK(true == has(rwx, Permission::Execute));

	// Remove execute
	rwx -= Permission::Execute;
	CHECK(false == has(rwx, Permission::Read));
	CHECK(false == has(rwx, Permission::Write));
	CHECK(false == has(rwx, Permission::Execute));

	// OR read
	rwx |= Permission::Read;
	CHECK(true == has(rwx, Permission::Read));
	CHECK(false == has(rwx, Permission::Write));
	CHECK(false == has(rwx, Permission::Execute));

	//
	rwx += Permission::Write | Permission::Execute;

	// AND execute, removes all but execute
	rwx &= Permission::Execute;
	CHECK(false == has(rwx, Permission::Read));
	CHECK(false == has(rwx, Permission::Write));
	CHECK(true == has(rwx, Permission::Execute));


	CHECK(std::format("{}", (Permission::Read)) == "001"s);
	CHECK(std::format("{}", (Permission::Write)) == "010"s);
	CHECK(std::format("{}", (Permission::Execute)) == "100"s);
	CHECK(std::format("{}", (Permission::Execute | Permission::Read)) == "101"s);


	CHECK(std::format("{:8}", (Permission::Read)) == "00000001"s);
	CHECK(std::format("{:8}", (Permission::Read | Permission::Execute)) == "00000101"s);
}
