module;
#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_addcarry_u64, _subborrow_u64, _umul128, _div128, _udiv128)
#endif
export module deckard.int128;

import std;
import deckard.types;
import deckard.utils.hash;
import deckard.assert;

namespace deckard
{
	namespace detail
	{
		constexpr std::optional<u32> digit_value(char c)
		{
			if (c >= '0' and c <= '9')
				return static_cast<u32>(c - '0');
			if (c >= 'a' and c <= 'f')
				return static_cast<u32>(c - 'a' + 10);
			if (c >= 'A' and c <= 'F')
				return static_cast<u32>(c - 'A' + 10);
			return {};
		}

		constexpr std::pair<u64, u64> mul_u64_u32(u64 value, u32 mul)
		{
			constexpr u64 mask = 0xFFFF'FFFFu;
			const u64     a0   = value & mask;
			const u64     a1   = value >> 32;
			const u64     p0   = a0 * mul;
			const u64     p1   = a1 * mul;
			const u64     mid  = (p0 >> 32) + p1;
			const u64     lo   = (p0 & mask) | ((mid & mask) << 32);
			const u64     hi   = mid >> 32;
			return std::pair<u64, u64>{hi, lo};
		}

		struct uint128_result
		{
			u64 high;
			u64 low;
		};

		inline uint128_result mul_portable_u64(u64 a_low, u64 a_high, u64 b_low, u64 b_high)
		{
			constexpr u64      mask = 0xFFFF'FFFFu;
			std::array<u64, 4> a    = {a_low & mask, a_low >> 32, a_high & mask, a_high >> 32};
			std::array<u64, 4> b    = {b_low & mask, b_low >> 32, b_high & mask, b_high >> 32};
			std::array<u64, 8> res{};

			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					const int k    = i + j;
					const u64 prod = a[i] * b[j];
					u64       sum  = res[k] + (prod & mask);
					res[k]         = sum & mask;
					u64 carry      = sum >> 32;

					sum        = res[k + 1] + (prod >> 32) + carry;
					res[k + 1] = sum & mask;
					carry      = sum >> 32;

					for (int kk = k + 2; carry != 0 and kk < 8; ++kk)
					{
						sum     = res[kk] + carry;
						res[kk] = sum & mask;
						carry   = sum >> 32;
					}
				}
			}

			return {(res[3] << 32) | res[2], (res[1] << 32) | res[0]};
		}

		inline bool is_bit_set_u64(u64 value, u32 bit) { return (value >> bit) & 1; }

		inline void set_bit_u64(u64& value, u32 bit) { value |= (1ULL << bit); }

		inline void clear_bit_u64(u64& value, u32 bit) { value &= ~(1ULL << bit); }

	} // namespace detail

	export class int128
	{
		i64 high{0};
		i64 low{0};

		template<typename>
		friend struct std::hash;
		friend class uint128;
		friend std::ostream& operator<<(std::ostream&, const int128&);

	public:
		constexpr int128() = default;

		constexpr int128(i64 h, i64 l)
			: high(h)
			, low(l)
		{
		}

		constexpr int128(i64 value)
			: high(value < 0 ? -1 : 0)
			, low(value)
		{
		}

		constexpr int128(const int128& other)                = default;
		constexpr int128(int128&& other) noexcept            = default;
		constexpr int128& operator=(const int128& other)     = default;
		constexpr int128& operator=(int128&& other) noexcept = default;

		constexpr int128(std::string_view str)
		{
			if (str.empty())
				return;

			bool negative = false;
			if (str[0] == '-')
			{
				negative = true;
				str.remove_prefix(1);
			}
			else if (str[0] == '+')
			{
				str.remove_prefix(1);
			}

			u32 base = 10;
			if (str.size() >= 2 and str[0] == '0' and (str[1] == 'x' or str[1] == 'X'))
			{
				base = 16;
				str.remove_prefix(2);
			}
			if (str.empty())
			{
				if (negative)
					*this = int128(-1);
				return;
			}

			int128 result(0);
			for (char c : str)
			{
				auto digit = detail::digit_value(c);
				if (not digit or *digit >= base)
				{
					high = 0;
					low  = 0;
					return;
				}
				auto [low_carry, new_low]                    = detail::mul_u64_u32(static_cast<u64>(result.low), base);
				[[maybe_unused]] auto [high_carry, new_high] = detail::mul_u64_u32(static_cast<u64>(result.high), base);
				new_high += low_carry;
				new_low += *digit;
				if (new_low < *digit)
					new_high += 1u;
				result.high = static_cast<i64>(new_high);
				result.low  = static_cast<i64>(new_low);
			}
			high = result.high;
			low  = result.low;

			if (negative)
				*this = -*this;
		}

		explicit constexpr operator bool() const { return high != 0 or low != 0; }

		explicit constexpr operator f64() const noexcept
		{
			constexpr f64 two_pow_64 = 18446744073709551616.0;
			return static_cast<f64>(high) * two_pow_64 + static_cast<f64>(static_cast<u64>(low));
		}

		explicit constexpr operator f32() const noexcept
		{
			constexpr f32 two_pow_64 = 18446744073709551616.0f;
			return static_cast<f32>(high) * two_pow_64 + static_cast<f32>(static_cast<u64>(low));
		}

		constexpr bool operator==(const int128& other) const = default;

		constexpr std::strong_ordering operator<=>(const int128& rhs) const
		{
			if (const auto cmp = high <=> rhs.high; cmp != 0)
				return cmp;
			return static_cast<u64>(low) <=> static_cast<u64>(rhs.low);
		}

		int128 operator+(const int128& other) const
		{
#ifdef _MSC_VER
			u64 result_low, result_high;
			u8  carry = _addcarry_u64(0, static_cast<u64>(low), static_cast<u64>(other.low), &result_low);
			_addcarry_u64(carry, static_cast<u64>(high), static_cast<u64>(other.high), &result_high);
			return {static_cast<i64>(result_high), static_cast<i64>(result_low)};
#else
			u64 r_low  = static_cast<u64>(low) + static_cast<u64>(other.low);
			u64 r_high = static_cast<u64>(high) + static_cast<u64>(other.high) + (static_cast<u64>(low) > r_low ? 1u : 0u);
			return {static_cast<i64>(r_high), static_cast<i64>(r_low)};
#endif
		}

		int128 operator-(const int128& other) const
		{
#ifdef _MSC_VER
			u64 result_low, result_high;
			u8  borrow = _subborrow_u64(0, static_cast<u64>(low), static_cast<u64>(other.low), &result_low);
			_subborrow_u64(borrow, static_cast<u64>(high), static_cast<u64>(other.high), &result_high);
			return {static_cast<i64>(result_high), static_cast<i64>(result_low)};
#else
			u64 r_low  = static_cast<u64>(low) - static_cast<u64>(other.low);
			u64 r_high = static_cast<u64>(high) - static_cast<u64>(other.high) -
						 (static_cast<u64>(low) < static_cast<u64>(other.low) ? 1u : 0u);
			return {static_cast<i64>(r_high), static_cast<i64>(r_low)};
#endif
		}

		constexpr int128 operator-() const
		{
			u64 low_bits  = static_cast<u64>(low);
			u64 high_bits = static_cast<u64>(high);
			low_bits      = ~low_bits + 1u;
			high_bits     = ~high_bits + (low_bits == 0u ? 1u : 0u);
			return {static_cast<i64>(high_bits), static_cast<i64>(low_bits)};
		}

		int128 operator*(const int128& rhs) const
		{
			bool   neg = (high < 0) != (rhs.high < 0);
			int128 a   = (high < 0) ? -(*this) : *this;
			int128 b   = (rhs.high < 0) ? -rhs : rhs;

#ifdef _MSC_VER
			u64 a_low  = static_cast<u64>(a.low);
			u64 a_high = static_cast<u64>(a.high);
			u64 b_low  = static_cast<u64>(b.low);
			u64 b_high = static_cast<u64>(b.high);

			u64    hi    = 0;
			u64    lo_lo = _umul128(a_low, b_low, &hi);
			u64    lo_hi = a_high * b_low + a_low * b_high + hi;
			int128 result{static_cast<i64>(lo_hi), static_cast<i64>(lo_lo)};
#else
			auto mul_result = detail::mul_portable_u64(
			  static_cast<u64>(a.low), static_cast<u64>(a.high), static_cast<u64>(b.low), static_cast<u64>(b.high));
			int128 result{static_cast<i64>(mul_result.high), static_cast<i64>(mul_result.low)};
#endif
			return neg ? -result : result;
		}

		int128& operator+=(const int128& other) { return *this = *this + other; }

		int128& operator-=(const int128& other) { return *this = *this - other; }

		int128& operator*=(const int128& other) { return *this = *this * other; }

		std::pair<int128, int128> div_mod(const int128& divisor) const;

		int128 operator/(const int128& rhs) const { return div_mod(rhs).first; }

		int128 operator%(const int128& rhs) const { return div_mod(rhs).second; }

		int128& operator/=(const int128& other) { return *this = *this / other; }

		int128& operator%=(const int128& other) { return *this = *this % other; }

		// Bitwise
		constexpr int128 operator&(const int128& other) const { return {high & other.high, low & other.low}; }

		constexpr int128 operator|(const int128& other) const { return {high | other.high, low | other.low}; }

		constexpr int128 operator^(const int128& other) const { return {high ^ other.high, low ^ other.low}; }

		constexpr int128 operator~() const { return {~high, ~low}; }

		int128 operator<<(u32 shift) const
		{
			if (shift == 0)
				return *this;
			if (shift >= 128)
				return {0, 0};
			if (shift >= 64)
				return {low << (shift - 64), 0};
#ifdef _MSC_VER
			return {static_cast<i64>(__shiftleft128(static_cast<u64>(low), static_cast<u64>(high), static_cast<u8>(shift))),
					low << shift};
#else
			return {(high << shift) | (low >> (64 - shift)), low << shift};
#endif
		}

		int128 operator>>(u32 shift) const
		{
			if (shift == 0)
				return *this;
			if (shift >= 128)
				return high < 0 ? int128(-1, -1) : int128(0, 0);
			if (shift >= 64)
				return {high >> 63, high >> (shift - 64)};
#ifdef _MSC_VER
			return {
			  high >> shift,
			  static_cast<i64>(__shiftright128(static_cast<u64>(low), static_cast<u64>(high), static_cast<u8>(shift)))};
#else
			return {high >> shift,
					static_cast<i64>((static_cast<u64>(low) >> shift) | (static_cast<u64>(high) << (64 - shift)))};
#endif
		}

		int128& operator<<=(u32 shift) { return *this = *this << shift; }

		int128& operator>>=(u32 shift) { return *this = *this >> shift; }

		std::string to_string(int base = 10) const
		{
			assert::check(base >= 2 and base <= 36, "Base must be between 2 and 36");

			if (*this == int128(0))
				return "0";

			bool   is_negative = high < 0;
			int128 value       = is_negative ? -*this : *this;

			const char* prefix     = "";
			int         prefix_len = 0;
			if (base == 16)
			{
				prefix     = "0x";
				prefix_len = 2;
			}
			else if (base == 2)
			{
				prefix     = "0b";
				prefix_len = 2;
			}

			std::string s;
			s.reserve(40); // 128 bits max ~40 chars

			while (value != int128(0))
			{
				int128 quotient  = value;
				i64    remainder = 0;

				for (int bit = 127; bit >= 0; --bit)
				{
					remainder = (remainder << 1) | static_cast<int>(value.is_bit_set(bit));
					if (remainder >= base)
					{
						remainder -= base;
						quotient.set_bit(bit);
					}
					else
					{
						quotient.clear_bit(bit);
					}
				}

				const u32 digit = static_cast<u32>(remainder);
				s += static_cast<char>(digit < 10 ? '0' + digit : 'a' + (digit - 10u));

				value = quotient;
			}

			std::reverse(s.begin(), s.end());
			if (is_negative)
				s.insert(0, "-");
			if (prefix_len > 0)
				s.insert(is_negative ? 1 : 0, prefix, prefix_len);
			return s;
		}

	private:
		constexpr bool is_bit_set(u32 bit) const
		{
			if (bit >= 64)
				return detail::is_bit_set_u64(static_cast<u64>(high), bit - 64);
			return detail::is_bit_set_u64(static_cast<u64>(low), bit);
		}

		constexpr void set_bit(u32 bit)
		{
			if (bit >= 64)
			{
				u64 h = static_cast<u64>(high);
				detail::set_bit_u64(h, bit - 64);
				high = static_cast<i64>(h);
			}
			else
			{
				u64 l = static_cast<u64>(low);
				detail::set_bit_u64(l, bit);
				low = static_cast<i64>(l);
			}
		}

		constexpr void clear_bit(u32 bit)
		{
			if (bit >= 64)
			{
				u64 h = static_cast<u64>(high);
				detail::clear_bit_u64(h, bit - 64);
				high = static_cast<i64>(h);
			}
			else
			{
				u64 l = static_cast<u64>(low);
				detail::clear_bit_u64(l, bit);
				low = static_cast<i64>(l);
			}
		}
	};

	export class uint128
	{
	private:
		u64 high{0};
		u64 low{0};

		template<typename>
		friend struct std::hash;
		friend class int128;
		friend std::ostream& operator<<(std::ostream&, const uint128&);

	public:
		constexpr uint128() = default;

		constexpr uint128(u64 h, u64 l)
			: high(h)
			, low(l)
		{
		}

		constexpr uint128(u64 value)
			: high(0)
			, low(value)
		{
		}

		constexpr uint128(const uint128& other)                = default;
		constexpr uint128(uint128&& other) noexcept            = default;
		constexpr uint128& operator=(const uint128& other)     = default;
		constexpr uint128& operator=(uint128&& other) noexcept = default;

		constexpr uint128(std::string_view str)
		{
			if (str.empty())
				return;

			u32 base = 10;
			if (str.size() >= 2 and str[0] == '0' and (str[1] == 'x' or str[1] == 'X'))
			{
				base = 16;
				str.remove_prefix(2);
			}
			if (str.empty())
				return;

			uint128 result(0);
			for (char c : str)
			{
				auto digit = detail::digit_value(c);
				if (not digit or *digit >= base)
				{
					high = 0;
					low  = 0;
					return;
				}
				auto [low_carry, new_low]                    = detail::mul_u64_u32(result.low, base);
				[[maybe_unused]] auto [high_carry, new_high] = detail::mul_u64_u32(result.high, base);
				new_high += low_carry;
				new_low += *digit;
				if (new_low < *digit)
					new_high += 1u;
				result.high = new_high;
				result.low  = new_low;
			}
			high = result.high;
			low  = result.low;
		}

		explicit constexpr operator bool() const { return high != 0 or low != 0; }

		explicit constexpr operator f64() const noexcept
		{
			constexpr f64 two_pow_64 = 18446744073709551616.0;
			return static_cast<f64>(high) * two_pow_64 + static_cast<f64>(low);
		}

		explicit constexpr operator f32() const noexcept
		{
			constexpr f32 two_pow_64 = 18446744073709551616.0f;
			return static_cast<f32>(high) * two_pow_64 + static_cast<f32>(low);
		}

		constexpr bool operator==(std::integral auto value) const
		{
			if constexpr (std::is_signed_v<decltype(value)>)
			{
				if (value < 0)
					return false;
			}
			return high == 0 && low == static_cast<u64>(value);
		}

		constexpr bool                 operator==(const uint128& other) const = default;
		constexpr std::strong_ordering operator<=>(const uint128& rhs) const  = default;

		uint128 operator+(const uint128& other) const
		{
#ifdef _MSC_VER
			u64 result_low, result_high;
			u8  carry = _addcarry_u64(0, low, other.low, &result_low);
			_addcarry_u64(carry, high, other.high, &result_high);
			return {result_high, result_low};
#else
			u64 r_low  = low + other.low;
			u64 r_high = high + other.high + (r_low < low ? 1u : 0u);
			return {r_high, r_low};
#endif
		}

		uint128 operator-(const uint128& other) const
		{
#ifdef _MSC_VER
			u64 result_low, result_high;
			u8  borrow = _subborrow_u64(0, low, other.low, &result_low);
			_subborrow_u64(borrow, high, other.high, &result_high);
			return {result_high, result_low};
#else
			u64 r_low  = low - other.low;
			u64 r_high = high - other.high - (low < other.low ? 1u : 0u);
			return {r_high, r_low};
#endif
		}

		uint128 operator*(const uint128& rhs) const
		{
#ifdef _MSC_VER
			u64 hi    = 0;
			u64 lo_lo = _umul128(low, rhs.low, &hi);
			u64 lo_hi = high * rhs.low + low * rhs.high + hi;
			return {lo_hi, lo_lo};
#else
			auto result = detail::mul_portable_u64(low, high, rhs.low, rhs.high);
			return {result.high, result.low};
#endif
		}

		template<std::integral T>
		constexpr uint128 operator+(T value) const
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (value < 0)
					return *this - uint128(static_cast<u64>(-(value + 1))) - 1u;
			}
			return *this + uint128(static_cast<u64>(value));
		}

		template<std::integral T>
		constexpr uint128 operator-(T value) const
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (value < 0)
					return *this + uint128(static_cast<u64>(-(value + 1))) + 1u;
			}
			return *this - uint128(static_cast<u64>(value));
		}

		template<std::integral T>
		constexpr uint128 operator*(T value) const
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (value < 0)
				{
					assert::check(false, "uint128 does not support multiply by negative values");
					return uint128(0u);
				}
			}
			return *this * uint128(static_cast<u64>(value));
		}

		template<std::integral T>
		uint128 operator/(T value) const
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (value < 0)
				{
					assert::check(false, "uint128 does not support divide by negative values");
					return uint128(0u);
				}
			}
			return *this / uint128(static_cast<u64>(value));
		}

		uint128& operator+=(const uint128& other) { return *this = *this + other; }

		uint128& operator-=(const uint128& other) { return *this = *this - other; }

		uint128& operator*=(const uint128& other) { return *this = *this * other; }

		constexpr bool operator==(const int128& other) const
		{
			if (other.high < 0)
				return false;
			return high == static_cast<u64>(other.high) and low == static_cast<u64>(other.low);
		}

		std::strong_ordering operator<=>(const int128& rhs) const
		{
			if (rhs.high < 0)
				return std::strong_ordering::greater;
			if (const auto cmp = high <=> static_cast<u64>(rhs.high); cmp != 0)
				return cmp;
			return low <=> static_cast<u64>(rhs.low);
		}

		// Bitwise
		constexpr uint128 operator&(const uint128& other) const { return {high & other.high, low & other.low}; }

		constexpr uint128 operator|(const uint128& other) const { return {high | other.high, low | other.low}; }

		constexpr uint128 operator^(const uint128& other) const { return {high ^ other.high, low ^ other.low}; }

		constexpr uint128 operator~() const { return {~high, ~low}; }

		uint128 operator<<(u32 shift) const
		{
			if (shift == 0)
				return *this;
			if (shift >= 128)
				return {0, 0};
			if (shift >= 64)
				return {low << (shift - 64), 0};
#ifdef _MSC_VER
			return {__shiftleft128(low, high, static_cast<u8>(shift)), low << shift};
#else
			return {(high << shift) | (low >> (64 - shift)), low << shift};
#endif
		}

		template<std::integral T>
		constexpr uint128 operator<<(T shift) const
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (shift < 0)
					return *this >> static_cast<u32>(-(shift + 1)) + 1u;
			}
			return *this << static_cast<u32>(shift);
		}

		uint128 operator>>(u32 shift) const
		{
			if (shift == 0)
				return *this;
			if (shift >= 128)
				return {0, 0};
			if (shift >= 64)
				return {0, high >> (shift - 64)};
#ifdef _MSC_VER
			return {high >> shift, __shiftright128(low, high, static_cast<u8>(shift))};
#else
			return {high >> shift, (low >> shift) | (high << (64 - shift))};
#endif
		}

		template<std::integral T>
		constexpr uint128 operator>>(T shift) const
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (shift < 0)
					return *this << static_cast<u32>(-(shift + 1)) + 1u;
			}
			return *this >> static_cast<u32>(shift);
		}

		uint128& operator<<=(u32 shift) { return *this = *this << shift; }

		uint128& operator>>=(u32 shift) { return *this = *this >> shift; }

		static inline int get_msb_pos(const uint128& val)
		{
			if (val.high != 0)
			{
				return 127 - static_cast<int>(std::countl_zero(val.high));
			}
			if (val.low != 0)
			{
				return 63 - static_cast<int>(std::countl_zero(val.low));
			}
			return -1; // Should not happen if checked before
		}

		std::pair<uint128, uint128> div_mod(const uint128& divisor) const
		{
			assert::check(divisor != uint128(0u), "Division by zero");

			if (*this < divisor)
				return {0u, *this};

			if (divisor == *this)
				return {1u, 0u};

			// fast path <= 64 bit divisor
			if (high == 0u and divisor.high == 0u)
				return {{0u, low / divisor.low}, {0u, low % divisor.low}};

#ifdef _MSC_VER
			// fast path intrinsics: 128 bit dividend, <= 64 bit divisor
			if (divisor.high == 0u)
			{
				const u64 d = divisor.low;
				u64       rem{};
				if (high < d)
				{
					const u64 q = _udiv128(high, low, d, &rem);
					return {{0u, q}, {0u, rem}};
				}
				else
				{
					const u64 q_high = high / d;
					const u64 q_low  = _udiv128(high % d, low, d, &rem);
					return {{q_high, q_low}, {0u, rem}};
				}
			}
#endif

			// Fallback: Restoring Division Algorithm
			int msb_dividend = get_msb_pos(*this);
			int msb_divisor  = get_msb_pos(divisor);

			// Since *this >= divisor, msb_dividend >= msb_divisor
			int shift = msb_dividend - msb_divisor;

			uint128 quotient  = 0;
			uint128 remainder = *this;
			uint128 d         = divisor << shift;

			for (int i = shift; i >= 0; --i)
			{
				if (remainder >= d)
				{
					remainder -= d;
					quotient = quotient | (uint128(1u) << i);
				}
				d >>= 1;
			}

			return {quotient, remainder};
		}

		uint128 operator/(const uint128& rhs) const { return div_mod(rhs).first; }

		uint128 operator%(const uint128& rhs) const { return div_mod(rhs).second; }

		uint128& operator/=(const uint128& rhs) { return *this = *this / rhs; }

		uint128& operator%=(const uint128& rhs) { return *this = *this % rhs; }

		std::string to_string(int base = 10) const
		{
			assert::check(base >= 2 and base <= 36, "Base must be between 2 and 36");

			if (*this == 0u)
			{
				if (base == 16)
					return "0x0";
				else if (base == 2)
					return "0b0";
				return "0";
			}

			std::string s;
			s.reserve(40); // 128 bits max ~40 chars
			uint128       temp = *this;
			const uint128 radix(static_cast<u64>(base));

			while (temp != 0u)
			{
				auto [q, r]     = temp.div_mod(radix);
				const u32 digit = static_cast<u32>(r.low);
				s += static_cast<char>(digit < 10 ? '0' + digit : 'a' + (digit - 10u));
				temp = q;
			}
			std::reverse(s.begin(), s.end());

			if (base == 16)
				s.insert(0, "0x");
			else if (base == 2)
				s.insert(0, "0b");
			return s;
		}


	private:
		constexpr bool is_bit_set(u32 bit) const
		{
			if (bit >= 64)
				return detail::is_bit_set_u64(high, bit - 64);
			return detail::is_bit_set_u64(low, bit);
		}

		constexpr void set_bit(u32 bit)
		{
			if (bit >= 64)
				detail::set_bit_u64(high, bit - 64);
			else
				detail::set_bit_u64(low, bit);
		}
	};

	inline std::pair<int128, int128> int128::div_mod(const int128& divisor) const
	{
		assert::check(divisor != int128(0), "Division by zero");

		const bool neg_q = (high < 0) != (divisor.high < 0);
		const bool neg_r = high < 0;

		int128 abs_a = (high < 0) ? -*this : *this;
		int128 abs_b = (divisor.high < 0) ? -divisor : divisor;

		uint128 ua(static_cast<u64>(abs_a.high), static_cast<u64>(abs_a.low));
		uint128 ub(static_cast<u64>(abs_b.high), static_cast<u64>(abs_b.low));

		auto [uq, ur] = ua.div_mod(ub);

		int128 q(static_cast<i64>(uq.high), static_cast<i64>(uq.low));
		int128 r(static_cast<i64>(ur.high), static_cast<i64>(ur.low));
		return {neg_q ? -q : q, neg_r ? -r : r};
	}

	export inline std::ostream& operator<<(std::ostream& os, const int128& val)
	{
		return os << val.to_string();
	}

	export inline std::ostream& operator<<(std::ostream& os, const uint128& val)
	{
		return os << val.to_string();
	}

} // namespace deckard

export namespace std
{

	template<>
	struct hash<deckard::int128>
	{
		size_t operator()(const deckard::int128& value) const { return deckard::utils::hash_values(value.low, value.high); }
	};

	template<>
	struct formatter<deckard::int128>
	{
		int base = 10;

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto it = ctx.begin();
			if (it != ctx.end() and (*it == 'x' or *it == 'b'))
			{
				if (*it == 'x')
					base = 16;
				else if (*it == 'b')
					base = 2;
				++it;
			}
			return it;
		}

		auto format(const deckard::int128& value, std::format_context& ctx) const
		{
			std::string str = value.to_string(base);
			return std::ranges::copy(str, ctx.out()).out;
		}
	};

	template<>
	struct hash<deckard::uint128>
	{
		size_t operator()(const deckard::uint128& value) const { return deckard::utils::hash_values(value.low, value.high); }
	};

	template<>
	struct formatter<deckard::uint128>
	{
		int base = 10;

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto it = ctx.begin();
			if (it != ctx.end() and (*it == 'x' or *it == 'b'))
			{
				if (*it == 'x')
					base = 16;
				else if (*it == 'b')
					base = 2;
				++it;
			}
			return it;
		}

		auto format(const deckard::uint128& value, std::format_context& ctx) const
		{
			std::string str = value.to_string(base);
			return std::ranges::copy(str, ctx.out()).out;
		}
	};

	template<>
	class numeric_limits<deckard::int128>
	{
	public:
		static constexpr bool                   is_specialized    = true;
		static constexpr bool                   is_signed         = true;
		static constexpr bool                   is_integer        = true;
		static constexpr bool                   is_exact          = true;
		static constexpr bool                   has_infinity      = false;
		static constexpr bool                   has_quiet_NaN     = false;
		static constexpr bool                   has_signaling_NaN = false;
		static constexpr std::float_round_style round_style       = std::round_toward_zero;
		static constexpr bool                   is_iec559         = false;
		static constexpr bool                   is_bounded        = true;
		static constexpr bool                   is_modulo         = true;
		static constexpr int                    digits            = 127;
		static constexpr int                    digits10          = 38;
		static constexpr int                    max_digits10      = 0;
		static constexpr int                    radix             = 2;
		static constexpr int                    min_exponent      = 0;
		static constexpr int                    min_exponent10    = 0;
		static constexpr int                    max_exponent      = 0;
		static constexpr int                    max_exponent10    = 0;
		static constexpr bool                   traps             = false;
		static constexpr bool                   tinyness_before   = false;

		static constexpr deckard::int128 min() noexcept
		{
			return deckard::int128(static_cast<deckard::i64>(0x8000'0000'0000'0000), 0);
		}

		static constexpr deckard::int128 max() noexcept
		{
			return deckard::int128(
			  static_cast<deckard::i64>(0x7FFF'FFFF'FFFF'FFFF), static_cast<deckard::i64>(0xFFFF'FFFF'FFFF'FFFF));
		}

		static constexpr deckard::int128 lowest() noexcept { return min(); }

		static constexpr deckard::int128 epsilon() noexcept { return deckard::int128(0); }

		static constexpr deckard::int128 round_error() noexcept { return deckard::int128(0); }

		static constexpr deckard::int128 infinity() noexcept { return deckard::int128(0); }

		static constexpr deckard::int128 quiet_NaN() noexcept { return deckard::int128(0); }

		static constexpr deckard::int128 signaling_NaN() noexcept { return deckard::int128(0); }

		static constexpr deckard::int128 denorm_min() noexcept { return deckard::int128(0); }
	};

	template<>
	class numeric_limits<deckard::uint128>
	{
	public:
		static constexpr bool                   is_specialized    = true;
		static constexpr bool                   is_signed         = false;
		static constexpr bool                   is_integer        = true;
		static constexpr bool                   is_exact          = true;
		static constexpr bool                   has_infinity      = false;
		static constexpr bool                   has_quiet_NaN     = false;
		static constexpr bool                   has_signaling_NaN = false;
		static constexpr std::float_round_style round_style       = std::round_toward_zero;
		static constexpr bool                   is_iec559         = false;
		static constexpr bool                   is_bounded        = true;
		static constexpr bool                   is_modulo         = true;
		static constexpr int                    digits            = 128;
		static constexpr int                    digits10          = 38;
		static constexpr int                    max_digits10      = 0;
		static constexpr int                    radix             = 2;
		static constexpr int                    min_exponent      = 0;
		static constexpr int                    min_exponent10    = 0;
		static constexpr int                    max_exponent      = 0;
		static constexpr int                    max_exponent10    = 0;
		static constexpr bool                   traps             = false;
		static constexpr bool                   tinyness_before   = false;

		static constexpr deckard::uint128 min() noexcept { return deckard::uint128(0); }

		static constexpr deckard::uint128 max() noexcept
		{
			return deckard::uint128(0xFFFF'FFFF'FFFF'FFFF, 0xFFFF'FFFF'FFFF'FFFF);
		}

		static constexpr deckard::uint128 lowest() noexcept { return min(); }

		static constexpr deckard::uint128 epsilon() noexcept { return deckard::uint128(0); }

		static constexpr deckard::uint128 round_error() noexcept { return deckard::uint128(0); }

		static constexpr deckard::uint128 infinity() noexcept { return deckard::uint128(0); }

		static constexpr deckard::uint128 quiet_NaN() noexcept { return deckard::uint128(0); }

		static constexpr deckard::uint128 signaling_NaN() noexcept { return deckard::uint128(0); }

		static constexpr deckard::uint128 denorm_min() noexcept { return deckard::uint128(0); }
	};
} // namespace std
