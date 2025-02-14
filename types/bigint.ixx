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

		bool is_zero() const { return sign == Sign::zero; }

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
					type new_digit = (digit << shift) + carry;
					carry          = new_digit / bigint::base;
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

			for (int i = lhs.digits.size() - 1; i >= 0; --i)
			{
				type new_digit = lhs.digits[i] + carry * bigint::base;
				carry          = new_digit % (1 << shift);
				new_digit /= (1 << shift);
				digits.push_back(new_digit);
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

		// mul
		void multiply(const bigint& lhs, const bigint& rhs)
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

	public:
		bigint() { operator=(0); }

		bigint(std::integral auto v) { operator=(v); }

		bigint(const bigint& bi) { operator=(bi); }

		bigint(std::string_view input)
		{
			auto lastPosition = input.rend();

			switch (input[0])
			{
				case '-':
					sign = Sign::negative;
					lastPosition -= 1;
					break;
				case '0':
				{
					if (input.length() == 1)
						sign = Sign::zero;
					return;
				}

				default: sign = Sign::positive;
			}

			int counter = 0, newgroup = 0, buffer = 0;
			for (auto it = input.rbegin(); it != lastPosition; ++it)
			{
				if (*it == 0)
					continue;
				if (counter == bigint::base_digits)
				{
					digits.push_back(newgroup);
					counter = newgroup = 0;
				}

				buffer = (int)(*it - '0');
				for (int i = 0; i < counter; i++)
					buffer *= 10;
				newgroup += buffer;

				counter++;
			}
			if (newgroup != 0)
				digits.push_back(newgroup);
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


			return (digits.size() - 1) * bigint::base_digits + count_digits(digits.back());
		}

		auto back() const { return digits.back(); }

		auto rbegin() const { return digits.rbegin(); }

		auto rend() const { return digits.rend(); }

		bool empty() const { return digits.empty(); }

		std::string to_string() const
		{
			std::string buffer;
			buffer.reserve(digits.size() * bigint::base_digits);

			switch (signum())
			{
				case Sign::negative: buffer.append("-"); break;
				case Sign::zero: return "0";
				default: break;
			}

			assert::check(not empty(), "bigint empty");

			buffer.append(std::format("{}", digits.back()));

			auto it = digits.rbegin();
			for (it++; it != digits.rend(); ++it)
				buffer.append(std::format("{:04}", *it));

			return buffer;
		}

		template<std::integral T = i32>
		std::expected<T, std::string> to_integer() const
		{
			if (signum() == Sign::zero or empty())
				return T{0};

			if (count() > count_digits(std::numeric_limits<T>::max()))
				return std::unexpected("bigint too large for integer conversion");

			i64 result     = 0;
			i64 multiplier = 1;
			for (const auto& digit : digits)
			{
				result += digit * multiplier;
				multiplier *= bigint::base;
			}
			if (signum() == Sign::negative)
				return result = -result;

			return result;
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

		auto operator*(const bigint& rhs) const
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
	};


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
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const bigint& v, std::format_context& ctx) const
		{

			switch (v.signum())
			{
				case Sign::negative: std::format_to(ctx.out(), "-"); break;
				case Sign::zero: return std::format_to(ctx.out(), "0");
				default: break;
			}

			assert::check(v.empty() == false, "bigint empty");

			std::format_to(ctx.out(), "{}", v.back());

			auto it = v.rbegin();
			for (it++; it != v.rend(); ++it)
				std::format_to(ctx.out(), "{:04}", *it);

			return std::format_to(ctx.out(), "");
		}
	};
} // namespace std
