export module deckard.uint128;

import std;
import deckard.types;

namespace deckard::uint128
{
	export class uint128
	{
	private:
		u64 high{0};
		u64 low{0};

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

			uint128 result(0);
			for (char c : str)
			{
				if (c < '0' or c > '9')
				{
					high = 0;
					low  = 0;
					return;
				}
				result = (result * 10u) + uint128(static_cast<u64>(c - '0'));
			}
			high = result.high;
			low  = result.low;
		}

		explicit constexpr operator bool() const { return high != 0 or low != 0; }

		constexpr bool operator==(const uint128& other) const { return high == other.high and low == other.low; }
		constexpr bool operator!=(const uint128& other) const { return not(*this == other); }
		constexpr bool operator<(const uint128& other) const { return (high < other.high) or (high == other.high and low < other.low); }
		constexpr bool operator<=(const uint128& other) const { return (*this < other) or (*this == other); }
		constexpr bool operator>(const uint128& other) const { return not(*this <= other); }
		constexpr bool operator>=(const uint128& other) const { return not(*this < other); }

		constexpr uint128 operator+(const uint128& other) const
		{
			u64 r_low  = low + other.low;
			u64 r_high = high + other.high + (r_low < low ? 1u : 0u);
			return {r_high, r_low};
		}

		constexpr uint128 operator-(const uint128& other) const
		{
			u64 r_low  = low - other.low;
			u64 r_high = high - other.high - (low < other.low ? 1u : 0u);
			return {r_high, r_low};
		}

		constexpr uint128 operator*(const uint128& rhs) const
		{
			u64 a32[4] = {low & 0xFFFFFFFF, low >> 32, high & 0xFFFFFFFF, high >> 32};
			u64 b32[4] = {rhs.low & 0xFFFFFFFF, rhs.low >> 32, rhs.high & 0xFFFFFFFF, rhs.high >> 32};
			u64 res[4] = {0, 0, 0, 0};

			for (int i = 0; i < 4; i++)
			{
				u64 carry = 0;
				for (int j = 0; i + j < 4; j++)
				{
					u64 val    = res[i + j] + (a32[i] * b32[j]) + carry;
					res[i + j] = val & 0xFFFFFFFF;
					carry      = val >> 32;
				}
			}

			return {res[3] << 32 | res[2], res[1] << 32 | res[0]};
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

		constexpr uint128& operator+=(const uint128& other) { return *this = *this + other; }
		constexpr uint128& operator-=(const uint128& other) { return *this = *this - other; }
		constexpr uint128& operator*=(const uint128& other) { return *this = *this * other; }

		// Bitwise
		constexpr uint128 operator&(const uint128& other) const { return {high & other.high, low & other.low}; }
		constexpr uint128 operator|(const uint128& other) const { return {high | other.high, low | other.low}; }
		constexpr uint128 operator^(const uint128& other) const { return {high ^ other.high, low ^ other.low}; }
		constexpr uint128 operator~() const { return {~high, ~low}; }

		constexpr uint128 operator<<(u32 shift) const
		{
			if (shift == 0) return *this;
			if (shift >= 128) return {0, 0};
			if (shift >= 64) return {low << (shift - 64), 0};
			return {(high << shift) | (low >> (64 - shift)), low << shift};
		}

		constexpr uint128 operator>>(u32 shift) const
		{
			if (shift == 0) return *this;
			if (shift >= 128) return {0, 0};
			if (shift >= 64) return {0, high >> (shift - 64)};
			return {high >> shift, (low >> shift) | (high << (64 - shift))};
		}

		constexpr uint128& operator<<=(u32 shift) { return *this = *this << shift; }
		constexpr uint128& operator>>=(u32 shift) { return *this = *this >> shift; }

		// Division and modulo
		constexpr std::pair<uint128, uint128> div_mod(const uint128& divisor) const
		{
			if (divisor == 0u)
				return {0u, 0u}; // Or throw/panic

			if (*this < divisor)
				return {0u, *this};

			if (divisor == *this)
				return {1u, 0u};

			uint128 quotient  = 0;
			uint128 remainder = 0;

			for (int i = 127; i >= 0; i--)
			{
				remainder <<= 1;
				if (is_bit_set(static_cast<u32>(i)))
					remainder.low |= 1u;

				if (remainder >= divisor)
				{
					remainder -= divisor;
					quotient.set_bit(static_cast<u32>(i));
				}
			}

			return {quotient, remainder};
		}

		constexpr uint128 operator/(const uint128& other) const { return div_mod(other).first; }
		constexpr uint128 operator%(const uint128& other) const { return div_mod(other).second; }

		constexpr uint128& operator/=(const uint128& other) { return *this = *this / other; }
		constexpr uint128& operator%=(const uint128& other) { return *this = *this % other; }

		std::string to_string() const
		{
			if (*this == 0u)
				return "0";

			std::string   s;
			uint128       temp = *this;
			const uint128 ten(10);

			while (temp != 0u)
			{
				auto [q, r] = temp.div_mod(ten);
				s += static_cast<char>(r.low + '0');
				temp = q;
			}
			std::reverse(s.begin(), s.end());
			return s;
		}

	private:
		constexpr bool is_bit_set(u32 bit) const
		{
			if (bit >= 64)
				return (high >> (bit - 64)) & 1;
			return (low >> bit) & 1;
		}

		constexpr void set_bit(u32 bit)
		{
			if (bit >= 64)
				high |= (1ULL << (bit - 64));
			else
				low |= (1ULL << bit);
		}
	};
} // namespace deckard::uint128
