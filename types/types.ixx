module;

export module deckard.types;

import std;
export import deckard.as;
import deckard.assert;
import deckard.debug;

export namespace deckard
{

	using byte = unsigned char;
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


	// Result
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

	constexpr u8 operator"" _u8(const u64 value) noexcept { return static_cast<u8>(value & 0xFF); }

	constexpr i8 operator"" _i8(const u64 value) noexcept { return static_cast<i8>(value & 0xFF); }

	constexpr u16 operator"" _u16(const u64 value) noexcept { return static_cast<u16>(value & 0xFFFF); }

	constexpr i16 operator"" _i16(const u64 value) noexcept { return static_cast<i16>(value & 0xFFFF); }

	constexpr u32 operator"" _u32(const u64 value) noexcept { return static_cast<u32>(value & 0xFFFF'FFFF); }

	constexpr i32 operator"" _i32(const u64 value) noexcept { return static_cast<i32>(value & 0xFFFF'FFFF); }

	constexpr u64 operator"" _u64(const u64 value) noexcept { return static_cast<u64>(value); }

	constexpr i64 operator"" _i64(const u64 value) noexcept { return static_cast<i64>(value); }

	// Sizes
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

	// sink
	struct sink_t
	{
		template<typename T>
		constexpr void operator=(T&&) const noexcept
		{
		}
	};

	inline constexpr sink_t _;


} // namespace deckard
