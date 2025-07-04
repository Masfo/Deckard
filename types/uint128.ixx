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
		uint128() = default;

		uint128(u64 h, u64 l)
			: high(h)
			, low(l)
		{
		}

		uint128(const uint128& other)                = default;
		uint128(uint128&& other) noexcept            = default;
		uint128& operator=(const uint128& other)     = default;
		uint128& operator=(uint128&& other) noexcept = default;

		constexpr bool operator==(const uint128& other) const { return high == other.high && low == other.low; }

		constexpr bool operator!=(const uint128& other) const { return !(*this == other); }

		constexpr bool operator<(const uint128& other) const { return (high < other.high) || (high == other.high && low < other.low); }

		constexpr bool operator<=(const uint128& other) const { return (*this < other) || (*this == other); }

		constexpr bool operator>(const uint128& other) const { return !(*this <= other); }

		constexpr bool operator>=(const uint128& other) const { return !(*this < other); }

		uint128 operator+(const uint128& other) const
		{
			uint128 result;
			result.low  = low + other.low;
			result.high = high + other.high + (result.low < low);
			return result;
		}

		uint128 operator-(const uint128& other) const
		{ 
			uint128 result;
			result.low = low - other.low;
			result.high = high - other.high - (result.low > low);
			return result;

		}

		std::string to_string() const
		{
			if (high == 0)
			{
				if (low == 0)
					return "0";
				return std::to_string(low);
			}

			std::string result;
			uint128     quotient = *this;
			uint128     remainder;
			uint128     divisor(0, 10);

			while (quotient != uint128(0, 0))
			{
				remainder     = quotient;
				quotient.high = 0;
				for (int i = 127; i >= 0; i--)
				{
					remainder.high = (remainder.high << 1) | ((remainder.low & (1ULL << 63)) >> 63);
					remainder.low  = remainder.low << 1;
					quotient.low   = quotient.low << 1;
					if (remainder >= divisor)
					{
						remainder = remainder - divisor;
						quotient.low |= 1;
					}
				}
				result.insert(0, 1, char('0' + static_cast<char>(remainder.low)));
			}

			return result;
			

		}
	};

} // namespace deckard::uint128
