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
				auto digit = digit_value(c);
				if (not digit or *digit >= base)
				{
					high = 0;
					low  = 0;
					return;
				}
				add_digit(result, base, *digit);
			}
			high = result.high;
			low  = result.low;
		}

		explicit constexpr operator bool() const { return high != 0 or low != 0; }

		constexpr bool operator==(const uint128& other) const  = default;
		constexpr auto operator<=>(const uint128& other) const = default;

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
			constexpr u64      mask = 0xFFFF'FFFFu;
			std::array<u64, 4> a    = {low & mask, low >> 32, high & mask, high >> 32};
			std::array<u64, 4> b    = {rhs.low & mask, rhs.low >> 32, rhs.high & mask, rhs.high >> 32};
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
			if (shift == 0)
				return *this;
			if (shift >= 128)
				return {0, 0};
			if (shift >= 64)
				return {low << (shift - 64), 0};
			return {(high << shift) | (low >> (64 - shift)), low << shift};
		}

		constexpr uint128 operator>>(u32 shift) const
		{
			if (shift == 0)
				return *this;
			if (shift >= 128)
				return {0, 0};
			if (shift >= 64)
				return {0, high >> (shift - 64)};
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
			return {hi, lo};
		}

		constexpr void add_digit(uint128& result, u32 base, u32 digit)
		{
			auto [low_carry, new_low]                    = mul_u64_u32(result.low, base);
			[[maybe_unused]] auto [high_carry, new_high] = mul_u64_u32(result.high, base);
			new_high += low_carry;
			new_low += digit;
			if (new_low < digit)
				new_high += 1u;
			result.high = new_high;
			result.low  = new_low;
		}
	};
} // namespace deckard::uint128
