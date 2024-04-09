export module deckard.enums;

import std;

export namespace deckard
{
	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator|(const T lhs, const T rhs) noexcept
	{
		return static_cast<T>(std::to_underlying(lhs) bitor std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr T operator&(const T lhs, const T rhs) noexcept
	{
		return static_cast<T>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr bool operator&&(const T lhs, const T rhs) noexcept
	{
		if (std::to_underlying(lhs) == std::to_underlying(rhs))
			return true;

		return static_cast<bool>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator^(const T lhs, const T rhs) noexcept
	{
		return static_cast<T>(std::to_underlying(lhs) xor std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator~(const T lhs) noexcept
	{
		return static_cast<T>(~std::to_underlying(lhs));
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator|=(T& lhs, const T rhs) noexcept
	{
		lhs = lhs bitor rhs;
		return lhs;
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator&=(T& lhs, const T rhs) noexcept
	{
		lhs = static_cast<T>(std::to_underlying(lhs) bitand std::to_underlying(rhs));
		return lhs;
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator^=(T& lhs, const T rhs) noexcept
	{
		lhs = lhs xor rhs;
		return lhs;
	}

	// Helpers for removing and setting flags
	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator-=(T& lhs, const T rhs) noexcept
	{
		lhs &= ~rhs;
		return lhs;
	}

	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator+=(T& lhs, const T rhs) noexcept
	{
		lhs |= rhs;
		return lhs;
	}

	// P3070 - https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p3070r0.html
	template<typename T>
	requires(std::is_scoped_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	auto format_as(T f) noexcept
	{
		return std::to_underlying(f);
	}

	/*
		namespace Filesystem
		{
			enum class Permission : u8
			{
				No      = 0x00,
				Read    = 0x01,
				Write   = 0x02,
				Execute = 0x04,
			};
			consteval void enable_bitmask_operator_or(Permission);


			//		using Filesystem::Permission;
			//		Permission readAndWrite{Permission::Read | Permission::Write | Permission::Execute};
			// 		readAndWrite &= ~Permission::Write;	 // Read | Execute
			//      readAndWrite -= Permission::Write    // Read | Execute

		} // namespace Filesystem


	*/

} // namespace deckard
