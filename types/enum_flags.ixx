export module deckard.enums;

import std;

namespace deckard
{
	// consteval void enable_bitmask_operations( <enum> );

	export template<typename T>
	concept EnumFlagType = requires {
		requires std::is_scoped_enum_v<T>;
		{ enable_bitmask_operations(std::declval<T>()) } -> std::same_as<void>; //

		{ T::Count } -> std::convertible_to<T>;
	};

	// Bit indexes
	export template<std::unsigned_integral T = unsigned char>
	[[nodiscard]] consteval T BIT(size_t index) noexcept
	{
		return static_cast<T>(T{1} << index);
	}

	export template<EnumFlagType T>
	[[nodiscard]] constexpr T operator|(const T lhs, const T rhs)
	{
		return static_cast<T>(std::to_underlying(lhs) bitor std::to_underlying(rhs));
	}

	export template<EnumFlagType T>
	[[nodiscard]] constexpr T operator&(const T lhs, const T rhs)
	{
		return static_cast<T>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
	}

	export template<EnumFlagType T>
	[[nodiscard]] constexpr T operator^(const T lhs, const T rhs)
	{
		return static_cast<T>(std::to_underlying(lhs) xor std::to_underlying(rhs));
	}

	export template<EnumFlagType T>
	[[nodiscard]] constexpr T operator~(const T lhs)
	{
		return static_cast<T>(~std::to_underlying(lhs));
	}

	export template<EnumFlagType T>
	constexpr T operator|=(T& lhs, const T rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	export template<EnumFlagType T>
	constexpr T operator&=(T& lhs, const T rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	export template<EnumFlagType T>
	constexpr T operator^=(T& lhs, const T rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
	}

	export template<EnumFlagType T>
	constexpr T operator-=(T& lhs, const T rhs)
	{
		lhs &= ~rhs;
		return lhs;
	}

	export template<EnumFlagType T>
	constexpr T operator+=(T& lhs, const T rhs)
	{
		lhs |= rhs;
		return lhs;
	}

	export template<EnumFlagType T>
	[[nodiscard]] constexpr bool has(T flags, T flag) noexcept
	{
		if (std::to_underlying(flags) == std::to_underlying(flag))
			return true;
		return static_cast<bool>(std::to_underlying(flags) & std::to_underlying(flag));
	}

	// P3070r0 - https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p3070r0.html
	//      r1 - https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3070r1.html
	//		r2 - https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3070r2.html
	//      r3 - https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3070r3.html

	export template<EnumFlagType T>
	auto format_as(T f)
	{
		return std::to_underlying(f);
	}


	/*
		namespace Filesystem
		{
			enum class Permission : u8
			{
				Read    = BIT(0),
				Write   = BIT(1),
				Execute = BIT(2),

				Count = 3,
			};
			consteval void enable_bitmask_operations(Permission);

			template<EnumFlagType T>
			struct std::formatter<T> : formatter<int>
			{
				// parse is optional
				constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

				auto format(T f, format_context& ctx) const
				{
					return std::format_to(ctx.out(), "{}", std::to_underlying(f));
				}
			};

		} // namespace Filesystem

		// Usage:

		using Filesystem::Permission;
		Permission readAndWrite{Permission::Read | Permission::Write | Permission::Execute};

		readAndWrite &= ~Permission::Write; // Read | Execute
		readAndWrite |= Permission::Write;  // Read | Execute | Write

		// or alternatively using += and -=
		readAndWrite -= Permission::Execute; // Read | Write
		readAndWrite += Permission::Execute; // Read | Write | Execute

		// Check
		bool has_read = has(readAndWrite, Permission::Read);

	*/

} // namespace deckard
