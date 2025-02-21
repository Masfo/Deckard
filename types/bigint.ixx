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
		using type = u32;

		static constexpr u32 base        = 10000;
		static constexpr u32 base_digits = 4;

		std::vector<type> digits;
		Sign              sign;

		void remove_trailing_zeros()
		{
			//
			if (digits.empty())
				return;

			while (not digits.empty() and digits.back() == 0)
				digits.pop_back();

			if (digits.empty())
				sign = Sign::zero;
			digits.shrink_to_fit();
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

				type carry = 0, buffer;

				for (int i = 0; i < rh->digits.size(); ++i)
				{

					buffer = lh->digits[i] + rh->digits[i] + carry;

					carry = buffer / bigint::base;
					buffer %= bigint::base;

					digits[i] = buffer;
				}

				if (carry > 0)
					digits.push_back(carry);
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

				type carry      = 0, buffer;
				bool need_carry = false;

				for (int i = 0; i < rh->digits.size() or carry != 0; ++i)
				{
					buffer     = lh->digits[i];
					need_carry = false;

					if (i < rh->digits.size())
					{
						if (lh->digits[i] < rh->digits[i] + carry)
						{
							buffer += bigint::base;
							need_carry = true;
						}

						buffer -= rh->digits[i] + carry;
					}
					else
					{
						if (lh->digits[i] == 0)
						{
							buffer += bigint::base;
							need_carry = true;
						}

						buffer -= carry;
					}
					digits[i] = buffer;
					carry     = need_carry ? 1 : 0;
				}
			}
			else
			{
				//
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

		void pow_op(const bigint& newbase, const bigint& exponent)
		{
			if (exponent.is_zero())
			{
				operator=(1);
				return;
			}

			bigint result(1);
			bigint base_copy(newbase);
			bigint exp_copy(exponent);

			while (exp_copy > 0)
			{
				if (exp_copy[0] % 2 == 1)
					result *= base_copy;
				base_copy *= base_copy;
				exp_copy >>= 1;
			}

			operator=(result);
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

			i32 sign = 1;
			if (input.size() >= 1 and input[0] == '-')
			{
				input.remove_prefix(1);
				sign = -1;
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

			if (sign < 0)
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

		bigint(const char* str) 
		{ 
			operator=(from_string(str)); }

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
			assert::check(not empty(), "bigint empty");

			return ((digits.size() - 1) * bigint::base_digits) + count_digits(digits.back());
		}

		auto back() const { return digits.back(); }

		auto rbegin() const { return digits.rbegin(); }

		auto rend() const { return digits.rend(); }

		bool empty() const { return digits.empty(); }

		std::string to_string(int newbase = 10, bool uppercase = false) const
		{
			if (digits.empty() or (digits.size() == 1 and digits[0] == 0))
				return "0";

			if (newbase == 10)
			{
				std::string buffer;
				buffer.reserve(digits.size() * bigint::base_digits);


				switch (signum())
				{
					case Sign::negative: buffer.append("-"); break;
					case Sign::zero: return "0";
					default: break;
				}

				buffer.append(std::format("{}", digits.back()));
				auto it = digits.rbegin();
				for (it++; it != digits.rend(); ++it)
					buffer.append(std::format("{:04}", *it));

				return buffer;
			}

			std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
			if (uppercase)
				chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

			assert::check(newbase >= 2 or newbase <= 36, "Base must be in the range [2, 36]");


			bigint      value(*this);
			std::string result;

			bigint carry = 0;
			while (!value.is_zero())
			{
				bigint remainder = value % newbase;
				value /= newbase;
				u32 index = remainder.to_integer<u32>().value_or(99);
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

		template<std::integral T = i32>
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

		// pow
		bigint operator^(const bigint& rhs) const
		{
			bigint result(*this);
			result.pow_op(*this, rhs);
			return result;
		}

		bigint& operator^=(const bigint& rhs)
		{
			operator=(operator^(rhs));
			return *this;
		}

		bigint pow(const bigint& rhs) const { return *this ^ rhs; }

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
			result.digits.clear();
			result.sign = Sign::positive;

			size_t size = std::max(digits.size(), rhs.digits.size());
			result.digits.reserve(size);

			for (size_t i = 0; i < size; ++i)
			{
				type digit1 = (i < digits.size()) ? digits[i] : 0;
				type digit2 = (i < rhs.digits.size()) ? rhs.digits[i] : 0;
				result.digits.push_back(digit1 & digit2);
			}
			// std::ranges::reverse(result.digits);
			result.remove_trailing_zeros();
			return result;
		}

		bigint& operator&=(const bigint& rhs)
		{
			operator=(operator&(rhs));
			return *this;
		}

		bool is_zero() const { return sign == Sign::zero; }
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

	export bigint random_bigint(size_t digits = 0)
	{
		std::random_device            rd;
		std::uniform_int_distribution ints(48, 57); // '0'...'9'

		std::string str;

		if (digits == 0)
		{
			std::uniform_int_distribution digits_dist(1, 32);
			digits = digits_dist(rd);
		}

		str.resize(digits);

		for (auto& i : str)
			i = ints(rd);

		while (str[0] == '0')
			str[0] = ints(rd);

		return bigint(str);
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
			auto pos = ctx.begin();
			while (pos != ctx.end() && *pos != '}')
			{
				if (*pos == 'h' or *pos == 'x')
					parsed_base = 16;

				if (*pos == 'H' or *pos == 'X')
				{
					parsed_base = 16;
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
								throw std::format_error("Base invalid");

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
