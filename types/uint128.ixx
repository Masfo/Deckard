export module deckard.uint128;

import std;
import deckard.types;
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

		uint128(u64 value)
			: high(0)
			, low(value)
		{
		}


		uint128(const uint128& other)                = default;
		uint128(uint128&& other) noexcept            = default;
		uint128& operator=(const uint128& other)     = default;
		uint128& operator=(uint128&& other) noexcept = default;

		uint128(std::string_view str) 
		{
			high = 0;
			low  = 0;

			if (str.empty())
				return;

			uint128       result;
			const uint128 ten(10);

			for (char c : str)
			{
				if (c < '0' || c > '9')
				{
					high = 0;
					low  = 0;
					return;
				}

				result = (*this) * ten + uint128(c - '0');

				high = result.high;
				low  = result.low;
			}
		}

		constexpr bool operator==(const uint128& other) const { return high == other.high && low == other.low; }

		constexpr bool operator!=(const uint128& other) const { return !(*this == other); }

		constexpr bool operator<(const uint128& other) const { return (high < other.high) || (high == other.high && low < other.low); }

		constexpr bool operator<=(const uint128& other) const { return (*this < other) || (*this == other); }

		constexpr bool operator>(const uint128& other) const { return !(*this <= other); }

		constexpr bool operator>=(const uint128& other) const { return !(*this < other); }

		template<std::integral T>
		uint128 operator+(const T value) const
		{
			uint128 result;
			result.low  = low + static_cast<u64>(value);
			result.high = high + (result.low < low ? 1 : 0);
			return result;
		}

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
			result.low  = low - other.low;
			result.high = high - other.high - (result.low > low);
			return result;
		}

		uint128 operator*(const uint128& other) const
		{
			uint128 result;
			result.low = low * other.low;
			result.high = high * other.low + low * other.high + (result.low < low);
			return result;
		}

#if 0
		std::string to_string() const
		{
			if (*this == ZERO)
			{
				return "0";
			}

			std::string   s;
			uint128       temp = *this;
			const uint128 ten(10);

			while (temp != ZERO)
			{
				s += static_cast<char>((temp % ten).low + '0');
				temp /= ten;
			}
			std::reverse(s.begin(), s.end());
			return s;
		}
#endif
	};


} // namespace deckard::uint128
