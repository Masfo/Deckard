module;

export module deckard.types;

import std;
import deckard.assert;

export namespace deckard
{
	using u8  = uint8_t;
	using i8  = int8_t;
	using u16 = uint16_t;
	using i16 = int16_t;
	using u32 = uint32_t;
	using i32 = int32_t;
	using u64 = uint64_t;
	using i64 = int64_t;
	using f32 = float;
	using f64 = double;


	export enum class Uppercase : u8 {
		No,
		Yes,
	};

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
					   std::format("Could not convert float '{}' safely. Too small target: {} < {} < {}",
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


} // namespace deckard
