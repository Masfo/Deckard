export module deckard.bigint;

import deckard.types;
import deckard.assert;
import deckard.math;
import deckard.as;
import deckard.utils.hash;
import deckard.helpers;
import deckard.debug;

import std;

namespace deckard
{
	export enum struct Sign {
		positive = 1,
		zero     = 0,
		negative = -1,
	};

	export class bigint
	{
	private:
		enum struct Compare
		{
			Greater,
			Equal,
			Less
		};

		// TODO: try base 10 and bas_digits 1, type to u8
		#if 0
		using type = u32;
		static constexpr type base        = 10000;
		static constexpr type base_digits = 4;
		#else
		using type = u8;
		static constexpr type base        = 10;
		static constexpr type base_digits = 1;
		#endif
		static constexpr type mask        = limits::max<type>;

		std::vector<type> digits;
		Sign              sign;

		void remove_trailing_zeros()
		{
			//
			if (digits.empty())
				return;

			while (not digits.empty() and digits.back() == 0)
				digits.pop_back();


			fix_sign();

			digits.shrink_to_fit();
		}

		void fix_sign()
		{
			if (digits.empty() or (digits.size() == 1 and digits[0] == 0))
			{
				sign = Sign::zero;
				return;
			}

			if (*this < 0)
				sign = Sign::negative;
			else
				sign = Sign::positive;
		}

		Compare compare(const bigint& lhs, const bigint& rhs) const
		{
			if (lhs.sign > rhs.sign)
				return Compare::Greater;
			else if (lhs.sign < rhs.sign)
				return Compare::Less;

			if (lhs.sign == Sign::positive)
				return compare_magnitude(lhs, rhs);
			else
				return compare_magnitude(rhs, lhs);
		}

		Compare compare_magnitude(const bigint& lhs, const bigint& rhs) const
		{
			if (lhs.size() > rhs.size())
				return Compare::Greater;
			else if (lhs.size() < rhs.size())
				return Compare::Less;
			else
			{
				for (i64 i = lhs.size() - 1; i >= 0 or i == 0; i--)
				{
					if (lhs[i] > rhs[i])
						return Compare::Greater;
					else if (lhs[i] < rhs[i])
						return Compare::Less;
				}
				return Compare::Equal;
			}
		}

		bigint largest_divisor(const bigint& a, const bigint& b)
		{
			bigint divisor(b);
			while ((a - divisor) >= divisor)
				divisor = divisor + divisor;
			return divisor;
		}

		void shift_left(const bigint& lhs, type shift)
		{
			digits.clear();
			digits.reserve(lhs.digits.size() + shift / bigint::base_digits + 1);

			if (shift == 0)
			{
				operator=(lhs);
			}
			else
			{
				u64 carry = 0;
				for (const u64 digit : lhs.digits)
				{
					u64 new_digit = (digit << shift) + carry;
					carry         = new_digit / bigint::base;
					digits.push_back(new_digit % bigint::base);
				}

				while (carry > 0)
				{
					digits.push_back(carry % bigint::base);
					carry /= bigint::base;
				}
			}

			sign = lhs.sign;
			remove_trailing_zeros();
		}

		void shift_right(const bigint& lhs, type shift)
		{
			digits.clear();
			digits.reserve(lhs.digits.size());

			u64 carry = 0;

			for (i64 i = as<i64>(lhs.digits.size() - 1); i >= 0; --i)
			{
				u64 new_digit = as<u64>(lhs.digits[i] + carry * bigint::base);
				carry         = new_digit % (1ull << shift);
				new_digit /= (1ull << shift);
				digits.push_back(as<type>(new_digit));
			}

			std::reverse(digits.begin(), digits.end());
			sign = lhs.sign;
			remove_trailing_zeros();
		}

		template<typename Op>
		void bitwise(const bigint& lhs, const bigint& rhs, Op op)
		{
			digits.clear();

			if (lhs.is_zero() or rhs.is_zero())
			{
				sign = Sign::zero;
				return;
			}
			else if ((lhs.sign == Sign::positive and rhs.sign == Sign::negative) or //
					 (lhs.sign == Sign::negative and rhs.sign == Sign::positive))
				sign = Sign::negative;
			else
				sign = Sign::positive;

			auto smaller = lhs;
			auto larger  = rhs;
			if (smaller.size() > larger.size())
				std::swap(smaller, larger);

			std::string result;
			result.reserve(2 + smaller.size() * 8);

			while (smaller > 0)
			{
				const auto digit1 = smaller % 2;
				const auto digit2 = larger % 2;
				smaller /= 2;
				larger /= 2;


				type binary1 = digit1.to_integer<type>().value();
				type binary2 = digit2.to_integer<type>().value();

				result.push_back('0' + ((char)(op(binary1, binary2))));
			}
			result += "b0";
			std::ranges::reverse(result);

			operator=(result);

			remove_trailing_zeros();
		}

		// bit ops
		void bit_and(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
				operator=(lhs);
			else if (rhs.is_zero())
				operator=(rhs);
			else
			{
				bitwise(lhs, rhs, std::bit_and<type>());
			}
		}

		void bit_or(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
				operator=(rhs);
			else if (rhs.is_zero())
				operator=(lhs);
			else
			{
				bitwise(lhs, rhs, std::bit_or<type>());
			}
		}

		void bit_xor(const bigint& lhs, const bigint& rhs)
		{

			if (lhs.is_zero())
				operator=(rhs);
			else if (rhs.is_zero())
				operator=(lhs);
			else if (lhs == rhs)
			{
				operator=(0);
			}
			else
			{
				bitwise(lhs, rhs, std::bit_xor<type>());
			}
		}

		void bit_not(const bigint& lhs)
		{
			digits.clear();

			if (lhs.is_zero())
			{
				digits.push_back(1);
				sign = Sign::positive;
				return;
			}

			bigint      count(lhs);
			std::string result;
			result.reserve(2 + lhs.size() * 8);

			while (count > 0)
			{
				const auto digit = count % 2;
				count >>= 1;

				bool binary = (bool)digit.to_integer<u8>().value();

				result.push_back('0' + (!binary));
			}
			result += "b0";
			std::ranges::reverse(result);

			operator=(result);

			remove_trailing_zeros();
		}

		// bin op
		void add(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.sign == Sign::zero)
			{
				operator=(rhs);
				return;
			}
			if (rhs.sign == Sign::zero)
			{
				operator=(lhs);
				return;
			}


			const bigint *lh = &lhs, *rh = &rhs;

			if (compare_magnitude(lhs, rhs) == Compare::Less)
			{
				lh = &rhs;
				rh = &lhs;
			}

			if (lhs.sign == rhs.sign)
			{
				if (lh != this)
					operator=(*lh);

				std::vector<type> result;
				type              carry = 0;
				size_t            i = 0, j = 0;

				while (i < lh->digits.size() or j < rh->digits.size() or carry != 0)
				{
					unsigned sum = carry;
					if (i < lh->digits.size())
						sum += lh->digits[i++];

					if (j < rh->digits.size())
						sum += rh->digits[j++];

					result.push_back(sum % bigint::base);
					carry = sum / bigint::base;
				}
				digits = result;
			}
			else
			{
				if (lh->sign == Sign::positive)
				{
					subtract(*lh, -(*rh));
				}
				else
				{
					subtract(-(*lh), *rh);
					operator-();
				}
			}
		}

		void subtract(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.sign == Sign::zero)
			{
				operator=(rhs);
				operator-();
				return;
			}
			if (rhs.sign == Sign::zero)
			{
				operator=(lhs);
				return;
			}


			const bigint *lh = &lhs, *rh = &rhs;

			if (compare_magnitude(lhs, rhs) == Compare::Less)
			{
				lh = &rhs;
				rh = &lhs;
			}

			if (lhs.sign == rhs.sign)
			{
				if (lh != this)
					operator=(*lh);

				std::vector<type> result;

				i32    borrow = 0;
				size_t i = 0, j = 0;

				while (i < lh->digits.size() or j < rh->digits.size())
				{
					int diff = (i < lh->digits.size() ? lh->digits[i] : 0) - (j < rh->digits.size() ? rh->digits[j] : 0) - borrow;
					if (diff < 0)
					{
						diff += bigint::base;
						borrow = 1;
					}
					else
					{
						borrow = 0;
					}

					result.push_back(diff);
					i++;
					j++;
				}

				while (result.size() > 1 && result.back() == 0)
					result.pop_back();

				digits = result;
			}
			else
			{

				if (lh->sign == Sign::positive)
				{
					add(*lh, -(*rh));
				}
				else
				{
					add(-(*lh), *rh);
					operator-();
				}
			}

			if (lh != &lhs)
				operator-();

			remove_trailing_zeros();
		}

		void karatsuba_multiply(const bigint& lhs, const bigint& rhs)
		{
			size_t n = std::max(lhs.size(), rhs.size());
			size_t m = n / 2;

			bigint high1(lhs.digits.begin() + m, lhs.digits.end());
			bigint low1(lhs.digits.begin(), lhs.digits.begin() + m);
			bigint high2(rhs.digits.begin() + m, rhs.digits.end());
			bigint low2(rhs.digits.begin(), rhs.digits.begin() + m);

			bigint z0 = low1 * low2;
			bigint z1 = (low1 + high1) * (low2 + high2);
			bigint z2 = high1 * high2;

			digits = z2.digits;
			digits.insert(digits.begin(), m * 2, 0);
			bigint temp = z1 - z2 - z0;
			temp.digits.insert(temp.digits.begin(), m, 0);
			*this += temp;
			*this += z0;
		}

		// mul
		void multiply(const bigint& lhs, const bigint& rhs)
		{
			// if (lhs.size() >= 32 and rhs.size() >= 32)
			//{
			//	karatsuba_multiply(lhs, rhs);
			//	return;
			// }


			digits.clear();

			if (lhs.is_zero() or rhs.is_zero())
			{
				sign = Sign::zero;
				return;
			}
			else if ((lhs.sign == Sign::positive and rhs.sign == Sign::negative) or //
					 (lhs.sign == Sign::negative and rhs.sign == Sign::positive))
				sign = Sign::negative;
			else
				sign = Sign::positive;

			if (rhs == 2)
			{
				operator=(lhs << 1);
				return;
			}

			const bigint *lh = &lhs, *rh = &rhs;

			if (compare_magnitude(lhs, rhs) == Compare::Less)
			{
				lh = &rhs;
				rh = &lhs;
			}

			digits.reserve(lh->digits.size() + rh->digits.size());

			size_t index = 0, lh_size = lh->digits.size(), rh_size = rh->digits.size();
			type   carry = 0, buffer = 0;


			for (size_t lower = 0; lower < rh_size; lower++)
			{
				index = lower;

				for (int upper = 0; upper < lh_size; upper++)
				{
					buffer = lh->digits[upper] * rh->digits[lower] + carry;
					if (index < digits.size())
						buffer += digits[index];

					carry = buffer / bigint::base;
					buffer %= bigint::base;

					if (index < digits.size())
						digits[index] = buffer;
					else
						digits.push_back(buffer);

					index++;
				}
				if (carry != 0)
				{
					digits.push_back(carry);
					carry = 0;
				}
			}
		}

		// divide


		void divide(const bigint& dividend, const bigint& divisor)
		{
			if (divisor.is_zero())
			{
				dbg::panic("divide zero");
			}

			if (dividend.is_zero() || compare_magnitude(dividend, divisor) == Compare::Less)
			{
				operator=(0);
				return;
			}

			bigint a(dividend), b(divisor);
			if (a.sign == b.sign)
			{
				if (a.sign == Sign::negative)
				{
					-a;
					-b;
				}

				sign = Sign::positive;
			}
			else
			{
				if (a.sign == Sign::negative)
					-a;
				else
					-b;

				sign = Sign::negative;
			}

			if (divisor == 2)
			{
				operator=(dividend >> 1);
				return;
			}

			bigint c(largest_divisor(a, b));
			bigint n(1);
			a = a - c;

			while (c != b)
			{
				c = c >> 1;
				n = n + n;
				if (c <= a)
				{
					a = a - c;
					n = n + 1;
				}
			}
			if (sign == Sign::negative)
				-n;
			operator=(n);
		}

		// modulus
		void modulus(const bigint& lhs, const bigint& rhs)
		{
			if (rhs.is_zero())
				operator=(0);

			if (compare_magnitude(lhs, rhs) == Compare::Less)
			{
				operator=(lhs);
				return;
			}

			operator=(lhs - rhs * (lhs / rhs));
		}

		void sqrt_op(const bigint& rhs)
		{
			if (rhs.is_zero() || rhs.signum() == Sign::negative)
			{
				operator=(0);
				return;
			}

			bigint left(1), right(rhs), mid, result;
			while (left <= right)
			{
				mid                = (left + right) / 2;
				bigint mid_squared = mid * mid;
				if (mid_squared == rhs)
				{
					operator=(mid);
					return;
				}
				else if (mid_squared < rhs)
				{
					left   = mid + 1;
					result = mid;
				}
				else
				{
					right = mid - 1;
				}
			}
			operator=(result);
		}

		bigint from_string(std::string_view input, i32 newbase = 0)
		{
			if (input.empty())
				return {};

			if (input.size() == 1 and (input[0] == '-' or input[0] == '0'))
				return {};

			i32 newsign = 1;
			if (input.size() >= 1 and input[0] == '-')
			{
				input.remove_prefix(1);
				newsign = -1;
			}


			if (newbase == 0 and input.size() >= 2)
			{
				if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X'))
				{
					newbase = 16;
					input.remove_prefix(2);
				}
				else if (input[0] == '0' && (input[1] == 'b' || input[1] == 'B'))
				{
					newbase = 2;
					input.remove_prefix(2);
				}
				else if (input[0] == '#')
				{
					newbase = 16;
					input.remove_prefix(1);
				}
				else if (input[0] == '0')
				{
					newbase = 8;
					input.remove_prefix(1);
				}
				else
				{
					newbase = 10;
				}
			}
			else if (newbase == 0 and input.size() < 2)
			{
				newbase = 10;
			}

			//
			bigint result;

			for (const auto& c : input)
			{
				i32 value = 0;
				if (c >= '0' and c <= '9')
					value = c - '0';
				else if (c >= 'a' and c <= 'z')
					value = c - 'a' + 10;
				else if (c >= 'A' and c <= 'Z')
					value = c - 'A' + 10;
				else
					value = static_cast<i32>(c);

				if (value >= newbase)
					dbg::panic("invalid character '{:c}' in string", c);

				result = result * newbase + value;
			}

			if (newsign < 0)
				-result;


			return result;
		}

	public:
		bigint()
		{
			digits.clear();
			digits.push_back(0);
			sign = Sign::zero;
		}

		template<typename Iterator>
		bigint(const Iterator begin, const Iterator end)
		{
			digits.assign(begin, end);
			remove_trailing_zeros();
			sign = digits.empty() ? Sign::zero : Sign::positive;
		}

		bigint(std::integral auto v) { operator=(v); }

		bigint(const bigint& bi) { operator=(bi); }

		bigint(const char* str) { operator=(from_string(str)); }

		bigint& operator=(const char* str) { return operator=(from_string(str)); }

		bigint(std::string_view input)
		{
			if (input.empty())
			{
				digits.clear();
				digits.push_back(0);
				sign = Sign::positive;
				return;
			}

			operator=(from_string(input));

			remove_trailing_zeros();
		}

		bigint& operator=(const std::string_view input)
		{
			bigint r(input);
			sign = r.sign;
			operator=(r);
			return *this;
		}

		bigint& operator=(const bigint& rhs)
		{
			sign   = rhs.sign;
			digits = rhs.digits;

			return *this;
		}

		template<std::unsigned_integral T>
		bigint& operator=(T const rhs)
		{
			u64 temp(rhs);

			digits.clear();
			digits.reserve(3);

			if (rhs == 0)
				sign = Sign::zero;
			else
				sign = Sign::positive;

			while (temp > 0)
			{
				digits.push_back(temp % bigint::base);
				temp /= bigint::base;
			}

			return *this;
		}

		template<std::signed_integral T>
		bigint& operator=(T const rhs)
		{
			u64 temp(std::abs(as<i64>(rhs)));

			digits.clear();
			digits.reserve(3);

			if (rhs == 0)
				sign = Sign::zero;
			else if (rhs > 0)
				sign = Sign::positive;
			else
				sign = Sign::negative;

			while (temp > 0)
			{
				digits.push_back(temp % bigint::base);
				temp /= bigint::base;
			}

			return *this;
		}

		type& operator[](size_t index)
		{
			assert::check(index < digits.size(), "Out-of-bound indexing on bigint");
			return digits[index];
		}

		const type& operator[](size_t index) const
		{
			assert::check(index < digits.size(), "Out-of-bound indexing on bigint");
			return digits[index];
		}

		bigint operator+() { return *this; }

		bigint operator+() const { return *this; }

		bigint operator-()
		{
			if (sign == Sign::positive)
				sign = Sign::negative;
			else if (sign == Sign::negative)
				sign = Sign::positive;

			return *this;
		}

		bigint operator-() const
		{
			bigint result(*this);
			result.operator-();
			return result;
		}

		// compares
		bool operator==(const bigint& rhs) const { return compare(*this, rhs) == Compare::Equal; }

		bool operator<(const bigint& rhs) const { return compare(*this, rhs) == Compare::Less; }

		bool operator<=(const bigint& rhs) const { return operator<(rhs) or operator==(rhs); }

		bool operator>(const bigint& rhs) const { return compare(*this, rhs) == Compare::Greater; }

		bool operator>=(const bigint& rhs) const { return operator>(rhs) or operator==(rhs); }

		//
		Sign signum() const { return sign; }

		size_t size() const { return digits.size(); }

		size_t count() const
		{
			if (sign == Sign::zero)
				return 1;
			if (digits.empty())
				return 0;

			return ((digits.size() - 1) * bigint::base_digits) + count_digits(digits.back());
		}

		size_t popcount() const
		{
			size_t result = 0;
			bigint count(*this);
			bigint digit;
			while (count > 0)
			{
				digit = count % 2;
				count >>= 1;

				result += digit.to_integer<size_t>().value();
			}
			return result;
		}

		auto back() const { return digits.back(); }

		auto rbegin() const { return digits.rbegin(); }

		auto rend() const { return digits.rend(); }

		bool empty() const { return digits.empty(); }

		std::string to_string(int newbase = 10, bool uppercase = false) const
		{
			if (digits.empty() or (digits.size() == 1 and digits[0] == 0))
				return "0";

			std::string result;
			u32         len  = digits.size() * bigint::base_digits + 1;
			result.reserve(len);

			if (newbase == 10)
			{

				switch (signum())
				{
					case Sign::negative: result.append("-"); break;
					case Sign::zero: return "0";
					default: break;
				}

				result.append(std::format("{}", digits.back()));
				auto it = digits.rbegin();
				for (it++; it != digits.rend(); ++it)
					result.append(std::format("{}", *it));

				return result;
			}


			static constexpr char chars[]      = "0123456789abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			u32                   upper_offset = uppercase ? 36 : 0;

			assert::check(newbase >= 2 or newbase <= 36, "Base must be in the range [2, 36]");


			bigint value(*this);


			bigint carry = 0;
			while (!value.is_zero())
			{
				bigint remainder = value % newbase;
				value /= newbase;
				u32 index = upper_offset + remainder.to_integer<u32>().value_or(99);
				assert::check(index < 99, "Base index is wrong");
				result.push_back(chars[index]);
			}

			if (sign == Sign::negative)
				result.push_back('-');

			while (not result.empty() and result.back() == '0')
				result.pop_back();

			std::reverse(result.begin(), result.end());

			return result;
		}

		template<std::integral T = i64>
		std::expected<T, std::string> to_integer() const
		{
			if (signum() == Sign::zero or empty())
				return T{0};

			if (count() > count_digits(std::numeric_limits<T>::max()))
				return std::unexpected("bigint too large for integer conversion");

			u64 result     = 0;
			u64 multiplier = 1;
			for (const auto& digit : digits)
			{
				result += (T)(digit)*multiplier;
				multiplier *= bigint::base;
			}

			if constexpr (std::is_signed_v<T>)
			{
				if (signum() == Sign::negative)
				{
					i64 iresult = result;
					return as<T>(-iresult);
				}
			}

			return as<T>(result);
		}

		void operator++()
		{
			if (digits.empty())
				operator=(0);

			if (is_zero())
			{
				operator=(1);
				return;
			}
			else if (sign == Sign::negative and digits.size() == 1 and digits[0] == 1)
			{
				sign = Sign::zero;
				operator=(0);
				return;
			}

			if (sign == Sign::negative)
				digits[0] -= 1;
			else
				digits[0] += 1;

			type carry = 0;
			for (int index = 0; index < digits.size(); index++)
			{
				digits[index] += carry;
				carry = digits[index] / bigint::base;
				digits[index] %= bigint::base;
			}
			if (carry != 0)
				digits.push_back(carry);
		}

		void operator++(int) { operator++(); }

		void operator--()
		{
			if (is_zero())
			{
				operator=(-1);
				return;
			}
			else if (sign == Sign::positive and digits.size() == 1 and digits[0] == 1)
			{
				sign = Sign::zero;
				operator=(0);
				return;
			}

			type carry = 0;
			if (digits[0] == 0)
			{
				carry     = 1;
				digits[0] = bigint::base - 1;
			}
			else
			{
				if (sign == Sign::positive)
					digits[0]--;
				else
					digits[0]++;
			}


			for (int index = 1; index < digits.size(); index++)
			{

				if (digits[index] < carry)
				{
					digits[index] += bigint::base;
					digits[index] -= carry;
					carry = 1;
				}
				else
				{
					digits[index] -= carry;
					carry = 0;
				}
			}

			remove_trailing_zeros();
		}

		void operator--(int) { operator--(); }

		// shift left
		auto operator<<(const type shift) const
		{
			bigint result;
			result.shift_left(*this, shift);
			return result;
		}

		bigint& operator<<=(const type shift)
		{
			operator=(operator<<(shift));
			return *this;
		}

		// shift right
		auto operator>>(const type shift) const
		{
			bigint result;
			result.shift_right(*this, shift);
			return result;
		}

		bigint& operator>>=(const type shift)
		{
			operator=(operator>>(shift));
			return *this;
		}

		// add
		bigint& operator+=(const bigint& rhs)
		{
			operator=(operator+(rhs));
			return *this;
		}

		bigint operator+(const bigint& rhs) const
		{
			bigint result;
			result.add(*this, rhs);
			return result;
		}

		// sub
		bigint& operator-=(const bigint& rhs)
		{
			operator=(operator-(rhs));
			return *this;
		}

		bigint operator-(const bigint& rhs) const

		{
			bigint result;
			result.subtract(*this, rhs);
			return result;
		}

		// mul

		bigint& operator*=(const bigint& rhs)
		{
			operator=(operator*(rhs));
			return *this;
		}

		bigint operator*(const bigint& rhs) const
		{
			bigint result;
			result.multiply(*this, rhs);
			return result;
		}

		// div
		auto operator/(const bigint& rhs) const
		{
			bigint result;
			result.divide(*this, rhs);
			return result;
		}

		bigint& operator/=(const bigint& rhs)
		{
			operator=(operator/(rhs));
			return *this;
		}

		// modulus
		auto operator%(const bigint& rhs) const
		{
			bigint result;
			result.modulus(*this, rhs);
			return result;
		}

		bigint& operator%=(const bigint& rhs)
		{
			operator=(operator%(rhs));
			return *this;
		}

		bigint pow(const bigint& exponent) const
		{
			bigint result(1);

			if (exponent.is_zero())
				return result;
			if (exponent.is_negative())
				return 0;

			bigint base_copy(*this);

			bigint exp_copy(exponent);

			while (exp_copy > 0)
			{
				if (exp_copy[0] % 2 == 1)
					result *= base_copy;
				base_copy *= base_copy;
				exp_copy >>= 1;
			}

			result.sign = sign;

			return result;
		}

		bigint mod(const bigint& rhs) const { return *this % rhs; }

		// sqrt
		bigint sqrt() const
		{
			bigint result;
			result.sqrt_op(*this);
			return result;
		}

		// abs
		bigint& abs()
		{
			sign = Sign::positive;
			return *this;
		}

		bigint abs() const
		{
			bigint result(*this);
			if (result < 0)
				return -result;
			return result;
		}

		bigint abs(const bigint& rhs) const
		{
			if (rhs.signum() == Sign::negative)
				return -rhs;

			return rhs;
		}

		// bitwise and
		bigint operator&(const bigint& rhs) const
		{
			bigint result;
			result.bit_and(*this, rhs);
			return result;
		}

		bigint& operator&=(const bigint& rhs)
		{
			operator=(operator&(rhs));
			return *this;
		}

		// bitwise or
		bigint operator|(const bigint& rhs) const
		{
			bigint result;
			result.bit_or(*this, rhs);
			return result;
		}

		bigint& operator|=(const bigint& rhs)
		{
			operator=(operator&(rhs));
			return *this;
		}

		// bitwise xor
		bigint operator^(const bigint& rhs) const
		{
			bigint result;
			result.bit_xor(*this, rhs);
			return result;
		}

		bigint& operator^=(const bigint& rhs)
		{
			operator=(operator&(rhs));
			return *this;
		}

		// bitwise not
		bigint operator~() const
		{
			bigint result;
			result.bit_not(*this);
			return result;
		}

		bool is_zero() const { return sign == Sign::zero; }

		bool is_negative() const { return sign == Sign::negative; }

		bool is_positive() const { return sign == Sign::positive; }
	};

	export bigint abs(const bigint& a) { return a.abs(); }

	export bigint sqrt(const bigint& a) { return a.sqrt(); }

	export bigint pow(const bigint& a, const bigint& b) { return a.pow(b); }

	export bigint mod(const bigint& a, const bigint& b) { return a.mod(b); }

	export bigint gcd(bigint a, bigint b)
	{
		a = a.abs();
		b = b.abs();

		while (true)
		{
			if (b.is_zero())
				return a;
			a %= b;
			if (a.is_zero())
				return b;
			b %= a;
		}
	}

	export bigint lcm(const bigint& a, const bigint& b)
	{
		if (a.is_zero() or b.is_zero())
			return bigint(0);
		return (a / gcd(a, b)) * b;
	}

	export template<std::integral T>
	bigint operator*(const T lhs, const bigint& rhs)
	{
		return bigint{lhs} * rhs;
	}

	export template<std::integral T>
	bigint operator+(const T lhs, const bigint& rhs)
	{
		return bigint{lhs} + rhs;
	}

	export template<std::integral T>
	bigint operator-(const T lhs, const bigint& rhs)
	{
		return bigint{lhs} - rhs;
	}

	export size_t popcount(const bigint& a) { return a.popcount(); }

	export bigint random_range(const bigint& start, const bigint& end)
	{

		if (end < start)
			dbg::panic("End must be greater than start. Swap order");


		std::random_device                 rd;
		std::mt19937                       gen(rd());
		std::uniform_int_distribution<u32> dist(0, 10);

		bigint range = (abs(end - start)) + 1;
		bigint result;
		size_t range_count = range.count();
		do
		{

			result = 0;

			for (size_t i = 0; i < range_count; ++i)
				result = (result * 10) + dist(gen);


		} while (result >= range);

		return start + result;
	}

	export bigint random_bigint(const bigint& range)
	{
		//
		return range;
	}

	export bigint random_prime(size_t keysize = 0)
	{
		//


		return {};
	}

} // namespace deckard

export namespace std

{
	using namespace deckard;

	template<>
	struct hash<bigint>
	{
		size_t operator()(const bigint& value) const { return deckard::utils::hash_values(value.to_string()); }
	};

	template<>
	struct formatter<bigint>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			// TODO: Width + leading zeros, 0x,0b,0o
			auto pos = ctx.begin();
			while (pos != ctx.end() && *pos != '}')
			{
				if (*pos == 'x')
					parsed_base = 16;

				if (*pos == 'X')
				{
					parsed_base = 16;
					uppercase   = true;
				}

				if (*pos == 'o')
					parsed_base = 8;

				if (*pos == 'O')
				{
					parsed_base = 8;
					uppercase   = true;
				}


				if (*pos == 'd')
				{
					parsed_base = 10;
				}

				if (*pos == 'D')
				{
					parsed_base = 10;
					uppercase   = true;
				}

				if (*pos == 'b' or *pos == 'B')
				{
					++pos;
					parsed_base = 0;

					if (*pos >= '0' and *pos <= '9')
					{
						parsed_base += *pos - '0';
						++pos;
						if (*pos >= '0' and *pos <= '9')
						{
							parsed_base *= 10;
							parsed_base += *pos - '0';
							++pos;

							if (parsed_base < 2 or parsed_base > 36)
								dbg::panic("Base invalid");

							continue;
						}
					}
					continue;
				}

				++pos;
			}

			if (pos != ctx.end() and *pos != '}')
				throw std::format_error("Invalid format");

			return pos;
		}

		auto format(const bigint& v, std::format_context& ctx) const
		{
			//
			return std::format_to(ctx.out(), "{}", v.to_string(parsed_base, uppercase));
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};
} // namespace std
