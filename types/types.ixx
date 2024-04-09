module;

export module deckard.types;

import std;
import deckard.assert;

export namespace deckard
{

	using DefaultErrorType = std::string;

	template<typename T, typename E = DefaultErrorType>
	using Result = std::expected<T, E>;

	auto Ok  = []<typename T>(const T value) -> Result<T> { return value; };
	auto Err = [](std::string_view fmt, auto... args)
	{
		using namespace std::string_view_literals;
		if constexpr (sizeof...(args) > 0)
			return std::unexpected<DefaultErrorType>(std::format("{}"sv, std::vformat(fmt, std::make_format_args(args...))));
		else
			return std::unexpected<DefaultErrorType>(fmt);
	};

	using byte = std::byte;
	using u8   = std::uint8_t;
	using i8   = std::int8_t;
	using u16  = std::uint16_t;
	using i16  = std::int16_t;
	using u32  = std::uint32_t;
	using i32  = std::int32_t;
	using u64  = std::uint64_t;
	using i64  = std::int64_t;
	using f32  = float;
	using f64  = double;

	using kibi = std::ratio<1ULL << 10>;
	using mebi = std::ratio<1ULL << 20>;
	using gibi = std::ratio<1ULL << 30>;
	using tebi = std::ratio<1ULL << 40>;

	constexpr u64 operator"" _KB(const u64 value) noexcept { return value * std::kilo::num; }

	constexpr u64 operator"" _MB(const u64 value) noexcept { return value * std::mega::num; }

	constexpr u64 operator"" _GB(const u64 value) noexcept { return value * std::giga::num; }

	constexpr u64 operator"" _TB(const u64 value) noexcept { return value * std::tera::num; }

	constexpr u64 operator"" _KiB(const u64 value) noexcept { return value * kibi::num; }

	constexpr u64 operator"" _MiB(const u64 value) noexcept { return value * mebi::num; }

	constexpr u64 operator"" _GiB(const u64 value) noexcept { return value * gibi::num; }

	constexpr u64 operator"" _TiB(const u64 value) noexcept { return value * tebi::num; }

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

	constexpr u8 operator"" _u8(const u64 value) noexcept { return static_cast<u8>(value & 0xFF); }

	constexpr i8 operator"" _i8(const u64 value) noexcept { return static_cast<i8>(value & 0xFF); }

	constexpr u16 operator"" _u16(const u64 value) noexcept { return static_cast<u16>(value & 0xFFFF); }

	constexpr i16 operator"" _i16(const u64 value) noexcept { return static_cast<i16>(value & 0xFFFF); }

	constexpr u32 operator"" _u32(const u64 value) noexcept { return static_cast<u32>(value & 0xFFFF'FFFF); }

	constexpr i32 operator"" _i32(const u64 value) noexcept { return static_cast<i32>(value & 0xFFFF'FFFF); }

	constexpr u64 operator"" _u64(const u64 value) noexcept { return static_cast<u64>(value); }

	constexpr i64 operator"" _i64(const u64 value) noexcept { return static_cast<i64>(value); }

	// sink
	struct sink_t
	{
		template<typename T>
		constexpr void operator=(T&&) const noexcept
		{
		}
	};

	inline constexpr sink_t _;

	// *************************************
	// Enum flags **************************
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



		enum class FunEnum : u8
		{
			Read  = 0x01,
			Write = 0x02,
		};

		consteval void enable_bitmask_operator_or(FunEnum);

	*/

} // namespace deckard
