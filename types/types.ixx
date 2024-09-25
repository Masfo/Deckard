module;
#include <Windows.h>

export module deckard.types;

import std;

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

	using char32 = char32_t;
	using byte   = unsigned char;
	using usize  = std::uintmax_t;
	using isize  = std::intmax_t;

	using f32 = float;
	using f64 = double;

	constexpr std::byte operator"" _byte(const u64 value) noexcept { return static_cast<std::byte>(value & 0xFF); }

	constexpr u8 operator"" _u8(const u64 value) noexcept { return static_cast<u8>(value & 0xFF); }

	constexpr i8 operator"" _i8(const u64 value) noexcept { return static_cast<i8>(value & 0xFF); }

	constexpr u16 operator"" _u16(const u64 value) noexcept { return static_cast<u16>(value & 0xFFFF); }

	constexpr i16 operator"" _i16(const u64 value) noexcept { return static_cast<i16>(value & 0xFFFF); }

	constexpr u32 operator"" _u32(const u64 value) noexcept { return static_cast<u32>(value & 0xFFFF'FFFF); }

	constexpr i32 operator"" _i32(const u64 value) noexcept { return static_cast<i32>(value & 0xFFFF'FFFF); }

	constexpr u64 operator"" _u64(const u64 value) noexcept { return static_cast<u64>(value); }

	constexpr i64 operator"" _i64(const u64 value) noexcept { return static_cast<i64>(value); }

	constexpr f32 operator"" _f32(const u64 value) noexcept { return static_cast<f32>(value); }

	constexpr f64 operator"" _f64(const u64 value) noexcept { return static_cast<f64>(value); }

	// limits
	template<std::integral T>
	constexpr T max_value = std::numeric_limits<T>::max();

	template<std::integral T>
	constexpr T min_value = std::numeric_limits<T>::min();


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
	struct sink_t final
	{
		template<typename T>
		constexpr void operator=(T&&) const noexcept
		{
		}
	};

	inline constexpr sink_t _;

	template<std::unsigned_integral T = u16>
	struct extent
	{
		extent() = default;

		extent(T w, T h)
			: width(w)
			, height(h)
		{
		}

		T width{T{0}};
		T height{T{0}};
	};

	auto to_extent(const RECT& r) -> extent<u16> { return extent{static_cast<u16>(r.right - r.left), static_cast<u16>(r.bottom - r.top)}; }

	export template<typename T>
	concept basic_container = requires(T cont) { requires std::ranges::range<T>; };

	export template<typename T>
	concept non_string_container = requires(T) {
		requires std::ranges::range<T>;

		requires !std::is_same_v<T, std::string> && !std::is_same_v<T, std::wstring> && !std::is_same_v<T, std::string_view> &&
				   !std::is_same_v<T, std::wstring_view>;
	};

	/* Formatter
		template <>
		struct std::formatter<Color> {
			constexpr auto parse(std::format_parse_context& ctx){
				auto pos = ctx.begin();
				while (pos != ctx.end() && *pos != '}') {
					if (*pos == 'h' || *pos == 'H')
						isHex_ = true;
					++pos;
				}
				return pos;  // expect `}` at this position, otherwise,
							  // it's error! exception!
			}

			auto format(const Color& col, std::format_context& ctx) const {
				if (isHex_) {
					uint32_t val = col.r << 16 | col.g << 8 | col.b;
					return std::format_to(ctx.out(), "#{:x}", val);
				}

				return std::format_to(ctx.out(), "({}, {}, {})", col.r, col.g, col.b);
			}

			bool isHex_{ false };
		};

		template<>
		class std::formatter<YourType>
		{
		public:
			// parse is optional
			constexpr auto parse( std::format_parse_context& ctx)
			{
				 return ctx.begin();
			}

			auto format( const YourType& type, std::format_context& ctx ) const
			{
				 return std::format_to(ctx.out(), "{}", type.value);
			}
		};
	*/

} // namespace deckard
