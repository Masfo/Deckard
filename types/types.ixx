module;

export module deckard.types;

import std;
import deckard.assert;

export namespace deckard
{
	using u8  = std::uint8_t;
	using i8  = std::int8_t;
	using u16 = std::uint16_t;
	using i16 = std::int16_t;
	using u32 = std::uint32_t;
	using i32 = std::int32_t;
	using u64 = std::uint64_t;
	using i64 = std::int64_t;
	using f32 = float;
	using f64 = double;

	template<typename T, typename U>
	T as(U u)
	{
		if constexpr (std::is_same_v<T, U>)
			return u;
		else
			return static_cast<T>(u);
	}

	template<typename I, typename F>
	requires std::integral<I> && std::floating_point<F>
	I as(F value) noexcept
	{
		if (((value <= static_cast<F>(std::numeric_limits<I>::max())) && (value >= static_cast<F>(std::numeric_limits<I>::min()))))
		{
			return static_cast<I>(value);
		}
		else
		{
			assert_msg(false,
					   std::format("Could not convert value '{}' safely. Target too small: {} < {} < {}",
								   value,
								   std::numeric_limits<I>::min(),
								   value,
								   std::numeric_limits<I>::max()));

			return std::numeric_limits<I>::max();
		}
	}

	namespace literals
	{

		constexpr u8 operator"" _u8(const u64 value) noexcept { return static_cast<u8>(value & 0xFF); }

		constexpr i8 operator"" _i8(const u64 value) noexcept { return static_cast<i8>(value & 0xFF); }

		constexpr u16 operator"" _u16(const u64 value) noexcept { return static_cast<u16>(value & 0xFFFF); }

		constexpr i16 operator"" _i16(const u64 value) noexcept { return static_cast<i16>(value & 0xFFFF); }

		constexpr u32 operator"" _u32(const u64 value) noexcept { return static_cast<u32>(value & 0xFFFF'FFFF); }

		constexpr i32 operator"" _i32(const u64 value) noexcept { return static_cast<i32>(value & 0xFFFF'FFFF); }

		constexpr u64 operator"" _u64(const u64 value) noexcept { return static_cast<u64>(value); }

		constexpr i64 operator"" _i64(const u64 value) noexcept { return static_cast<i64>(value); }
	} // namespace literals

	// Enum flags
	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator|(const T lhs, const T rhs)
	{
		return static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator&(const T lhs, const T rhs)
	{
		return static_cast<T>(std::to_underlying(lhs) & std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator^(const T lhs, const T rhs)
	{
		return static_cast<T>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
	}

	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator~(const T lhs)
	{
		return static_cast<T>(~std::to_underlying(lhs));
	}

	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator|=(T &lhs, const T rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator&=(T &lhs, const T rhs)
	{
		lhs = lhs & rhs;
		return lhs;
	}

	template<typename T>
	requires(std::is_enum_v<T> and requires(T e) { enable_bitmask_operations(e); })
	constexpr auto operator^=(T &lhs, const T rhs)
	{
		lhs = lhs ^ rhs;
		return lhs;
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

		} // namespace Filesystem



		enum class FunEnum : u8
		{
			Read  = 0x01,
			Write = 0x02,
		};

		consteval void enable_bitmask_operator_or(FunEnum);

	*/

} // namespace deckard
