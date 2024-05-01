export module deckard.enums;

import std;
import deckard.types;

export namespace deckard
{

	export template<typename T>
	concept EnumFlagType = requires {
		// typename std::underlying_type<T>::type;
		requires std::is_scoped_enum_v<T>;
		// requires std::is_same_v<decltype(T::Width), T>;
		{ enable_bitmask_operations(std::declval<T>()) } -> std::same_as<void>; //
	};

	// Bit indexes
	template<typename T = u8>
	consteval T BIT(size_t index)
	{
		return static_cast<T>(index == 0 ? 0 : 1 << (index - 1));
	}

	template<EnumFlagType T>
	constexpr auto operator|(const T lhs, const T rhs) noexcept
	{
		return static_cast<T>(std::to_underlying(lhs) bitor std::to_underlying(rhs));
	}

	template<EnumFlagType T>
	constexpr T operator&(const T lhs, const T rhs) noexcept
	{
		return static_cast<T>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
	}

	template<EnumFlagType T>
	constexpr bool operator&&(const T lhs, const T rhs) noexcept
	{
		if (std::to_underlying(lhs) == std::to_underlying(rhs))
			return true;

		return static_cast<bool>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
	}

	template<EnumFlagType T>
	constexpr auto operator^(const T lhs, const T rhs) noexcept
	{
		return static_cast<T>(std::to_underlying(lhs) xor std::to_underlying(rhs));
	}

	template<EnumFlagType T>
	constexpr auto operator~(const T lhs) noexcept
	{
		return static_cast<T>(~std::to_underlying(lhs));
	}

	template<EnumFlagType T>
	constexpr auto operator|=(T& lhs, const T rhs) noexcept
	{
		lhs = lhs bitor rhs;
		return lhs;
	}

	template<EnumFlagType T>
	constexpr auto operator&=(T& lhs, const T rhs) noexcept
	{
		lhs = static_cast<T>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
		return lhs;
	}

	template<EnumFlagType T>
	constexpr auto operator^=(T& lhs, const T rhs) noexcept
	{
		lhs = lhs xor rhs;
		return lhs;
	}

	// Helpers for removing and setting flags
	template<EnumFlagType T>
	constexpr auto operator-=(T& lhs, const T rhs) noexcept
	{
		lhs &= ~rhs;
		return lhs;
	}

	template<EnumFlagType T>
	constexpr auto operator+=(T& lhs, const T rhs) noexcept
	{
		lhs |= rhs;
		return lhs;
	}

	// P3070 - https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p3070r0.html
	template<EnumFlagType T>
	auto format_as(T f) noexcept
	{
		return std::to_underlying(f);
	}

	/*
		namespace Filesystem
		{
			enum class Permission : u8
			{
				No      = BIT(0),
				Read    = BIT(1),
				Write   = BIT(2),
				Execute = BIT(3),
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

		} // namespace Filesystem

		// Usage:

		using Filesystem::Permission;
		Permission readAndWrite{Permission::Read | Permission::Write | Permission::Execute};

		readAndWrite &= ~Permission::Write; // Read | Execute
		readAndWrite |= Permission::Write;  // Read | Execute | Write

		// or alternatively using += and -=
		readAndWrite -= Permission::Execute; // Read | Write
		readAndWrite += Permission::Execute; // Read | Write | Execute

	*/

} // namespace deckard
