export module deckard.bigint;

import deckard.types;
import deckard.assert;
import deckard.math;
import deckard.as;
import deckard.utils.hash;
import deckard.helpers;
import deckard.debug;
import deckard.random;
import deckard.logger;
import std;

namespace deckard
{
	export enum struct Sign : i8 {
		positive = 1,
		zero     = 0,
		negative = -1,
	};

	export class bigint
	{
	private:
		enum struct Compare : i8
		{
			Greater = 1,
			Equal   = 0,
			Less    = -1,
		};

		using type                        = u32;
		static constexpr type base        = 1'000'000'000;
		static constexpr type base_digits = 9;

		std::vector<type> digits;
		Sign              sign{Sign::zero};

		// -------------------------------------------------------------------------
		// Helpers
		// -------------------------------------------------------------------------


		void remove_trailing_zeros() noexcept
		{
			while (!digits.empty() and digits.back() == 0)
				digits.pop_back();

			if (digits.empty() or (digits.size() == 1 and digits[0] == 0))
				sign = Sign::zero;
		}

		[[nodiscard]] static Compare compare_magnitude(const bigint& lhs, const bigint& rhs) noexcept
		{
			if (lhs.digits.size() != rhs.digits.size())
				return lhs.digits.size() > rhs.digits.size() ? Compare::Greater : Compare::Less;

			for (auto i = static_cast<i64>(lhs.digits.size()) - 1; i >= 0; --i)
			{
				if (lhs.digits[i] != rhs.digits[i])
					return lhs.digits[i] > rhs.digits[i] ? Compare::Greater : Compare::Less;
			}
			return Compare::Equal;
		}

		[[nodiscard]] Compare compare(const bigint& lhs, const bigint& rhs) const noexcept
		{
			if (lhs.sign != rhs.sign)
			{
				return static_cast<i8>(lhs.sign) > static_cast<i8>(rhs.sign) ? Compare::Greater : Compare::Less;
			}

			if (lhs.sign == Sign::positive)
				return compare_magnitude(lhs, rhs);
			if (lhs.sign == Sign::negative)
				return compare_magnitude(rhs, lhs);
			return Compare::Equal;
		}

		[[nodiscard]] static bigint largest_divisor(const bigint& a, const bigint& b)
		{
			bigint divisor = b;
			bigint doubled = divisor + divisor;
			while (doubled <= a)
			{
				divisor = std::move(doubled);
				doubled = divisor + divisor;
			}
			return divisor;
		}

		void shift_left(const bigint& lhs, type shift)
		{
			if (shift == 0)
			{
				if (this != &lhs)
					*this = lhs;
				return;
			}

			if (lhs.is_zero())
			{
				*this = 0;
				return;
			}

			bigint power_of_2(1);
			bigint temp;
			for (type i = 0; i < shift; ++i)
			{
				temp.add(power_of_2, power_of_2);
				power_of_2 = std::move(temp);
			}

			const auto input_digits = lhs.digits;
			const auto input_sign   = lhs.sign;

			const size_t lh_size = input_digits.size();
			const size_t rh_size = power_of_2.digits.size();
			digits.assign(lh_size + rh_size, 0);
			sign = input_sign;

			for (size_t j = 0; j < rh_size; ++j)
			{
				type carry = 0;
				for (size_t i = 0; i < lh_size; ++i)
				{
					const u64 prod = static_cast<u64>(input_digits[i]) * power_of_2.digits[j] + digits[i + j] + carry;
					digits[i + j]  = static_cast<type>(prod % base);
					carry          = static_cast<type>(prod / base);
				}
				if (carry)
					digits[lh_size + j] += carry;
			}

			remove_trailing_zeros();
		}

		void shift_right(const bigint& lhs, type shift)
		{
			if (shift == 0)
			{
				if (this != &lhs)
					*this = lhs;
				return;
			}

			if (lhs.is_zero())
			{
				*this = 0;
				return;
			}

			bigint power_of_2(1);
			bigint temp;
			for (type i = 0; i < shift; ++i)
			{
				temp.add(power_of_2, power_of_2);
				power_of_2 = std::move(temp);
			}

			bigint dividend = lhs;
			divide(dividend, power_of_2);
		}

		// Generic bitwise op
		template<typename Op>
		void bitwise(const bigint& lhs, const bigint& rhs, Op op)
		{
			digits.clear();

			auto lhs_val = lhs.to_integer<u64>();
			auto rhs_val = rhs.to_integer<u64>();
			if (lhs_val.has_value() and rhs_val.has_value())
			{
				*this = op(lhs_val.value(), rhs_val.value());
				sign  = is_zero() ? Sign::zero : Sign::positive;
				return;
			}

			static const bigint two32(4'294'967'296ULL);

			bigint lo = lhs.abs();
			bigint ro = rhs.abs();

			std::vector<u32> chunks;
			chunks.reserve(std::max(lo.size(), ro.size()) / 2 + 1);

			while (not lo.is_zero() or not ro.is_zero())
			{
				bigint lrem, rrem;
				bigint lq, rq;
				if (not lo.is_zero())
					lq.divide(lo, two32, &lrem);
				if (not ro.is_zero())
					rq.divide(ro, two32, &rrem);

				const u32 lchunk = lrem.to_integer<u32>().value_or(0u);
				const u32 rchunk = rrem.to_integer<u32>().value_or(0u);
				chunks.push_back(static_cast<u32>(op(lchunk, rchunk)));

				lo = std::move(lq);
				ro = std::move(rq);
			}

			bigint result(0);
			for (auto i = static_cast<i64>(chunks.size()) - 1; i >= 0; --i)
			{
				result = result * bigint(4'294'967'296ULL) + bigint(static_cast<u64>(chunks[i]));
			}

			*this = std::move(result);
			sign  = is_zero() ? Sign::zero : Sign::positive;
		}

		// -------------------------------------------------------------------------
		// Arithmetic primitives
		// -------------------------------------------------------------------------

		void subtract_magnitudes(const bigint& lhs, const bigint& rhs)
		{
			if (this == &lhs or this == &rhs)
			{
				bigint temp_lhs = lhs;
				bigint temp_rhs = rhs;
				subtract_magnitudes(temp_lhs, temp_rhs);
				return;
			}

			std::vector<type> result;
			result.reserve(lhs.digits.size());
			int borrow = 0;
			for (size_t i = 0; i < lhs.digits.size(); ++i)
			{
				int diff =
				  static_cast<int>(lhs.digits[i]) - (i < rhs.digits.size() ? static_cast<int>(rhs.digits[i]) : 0) - borrow;
				if (diff < 0)
				{
					diff += base;
					borrow = 1;
				}
				else
					borrow = 0;
				result.push_back(static_cast<type>(diff));
			}
			digits = std::move(result);
			remove_trailing_zeros();
		}

		void add(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
			{
				if (this != &rhs)
					*this = rhs;
				return;
			}
			if (rhs.is_zero())
			{
				if (this != &lhs)
					*this = lhs;
				return;
			}

			if (lhs.sign == rhs.sign)
			{
				const bigint& lh = lhs.digits.size() >= rhs.digits.size() ? lhs : rhs;
				const bigint& rh = lhs.digits.size() >= rhs.digits.size() ? rhs : lhs;

				std::vector<type> result;
				result.reserve(lh.digits.size() + 1);
				type   carry = 0;
				size_t i = 0, j = 0;
				while (i < lh.digits.size() or j < rh.digits.size() or carry)
				{
					u64 sum = carry;
					if (i < lh.digits.size())
						sum += lh.digits[i++];
					if (j < rh.digits.size())
						sum += rh.digits[j++];
					result.push_back(static_cast<type>(sum % base));
					carry = static_cast<type>(sum / base);
				}
				digits = std::move(result);
				sign   = lhs.sign;
			}
			else
			{
				auto cmp = compare_magnitude(lhs, rhs);
				if (cmp == Compare::Equal)
				{
					*this = 0;
					return;
				}
				const bool    lhs_dominates = (cmp == Compare::Greater);
				const bigint& bigger        = lhs_dominates ? lhs : rhs;
				const bigint& smaller       = lhs_dominates ? rhs : lhs;
				Sign          result_sign   = lhs_dominates ? lhs.sign : rhs.sign;
				subtract_magnitudes(bigger, smaller);
				sign = result_sign;
			}
		}

		void subtract(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
			{
				*this = rhs;
				negate();
				return;
			}
			if (rhs.is_zero())
			{
				if (this != &lhs)
					*this = lhs;
				return;
			}

			const Sign rhs_sign = (rhs.sign == Sign::positive) ? Sign::negative : Sign::positive;

			if (lhs.sign == rhs_sign)
			{
				const bigint& lh = lhs.digits.size() >= rhs.digits.size() ? lhs : rhs;
				const bigint& rh = lhs.digits.size() >= rhs.digits.size() ? rhs : lhs;

				std::vector<type> result;
				result.reserve(lh.digits.size() + 1);
				type   carry = 0;
				size_t i = 0, j = 0;
				while (i < lh.digits.size() or j < rh.digits.size() or carry)
				{
					u64 sum = carry;
					if (i < lh.digits.size())
						sum += lh.digits[i++];
					if (j < rh.digits.size())
						sum += rh.digits[j++];
					result.push_back(static_cast<type>(sum % base));
					carry = static_cast<type>(sum / base);
				}
				digits = std::move(result);
				sign   = lhs.sign;
			}
			else
			{
				auto cmp = compare_magnitude(lhs, rhs);
				if (cmp == Compare::Equal)
				{
					*this = 0;
					return;
				}
				const bool    lhs_dominates = (cmp == Compare::Greater);
				const bigint& bigger        = lhs_dominates ? lhs : rhs;
				const bigint& smaller       = lhs_dominates ? rhs : lhs;
				const Sign    result_sign   = lhs_dominates ? lhs.sign : rhs_sign;
				subtract_magnitudes(bigger, smaller);
				sign = result_sign;
			}
		}

		void multiply(const bigint& lhs, const bigint& rhs)
		{
			assert::check(this != &lhs and this != &rhs, "multiply: output aliases input");

			digits.clear();

			if (lhs.is_zero() or rhs.is_zero())
			{
				sign = Sign::zero;
				return;
			}

			sign = (lhs.sign == rhs.sign) ? Sign::positive : Sign::negative;

			const size_t lh_size = lhs.digits.size();
			const size_t rh_size = rhs.digits.size();
			digits.assign(lh_size + rh_size, 0);

			for (size_t j = 0; j < rh_size; ++j)
			{
				type carry = 0;
				for (size_t i = 0; i < lh_size; ++i)
				{
					const u64 prod = static_cast<u64>(lhs.digits[i]) * rhs.digits[j] + digits[i + j] + carry;
					digits[i + j]  = static_cast<type>(prod % base);
					carry          = static_cast<type>(prod / base);
				}
				if (carry)
					digits[lh_size + j] += carry;
			}

			remove_trailing_zeros();
		}

		// TODO: optional<&> for remainder
#ifdef __cpp_lib_optional_ref
#error ("use optional ref for remainder");
		// std::optional<bigint&> out_remainder = std::nullopt

		if (out_remainder)
			*out_remainder = std::move(rem);
#endif
		void divide(const bigint& dividend, const bigint& divisor, bigint* out_remainder = nullptr)
		{
			if (divisor.is_zero())
				dbg::panic("Division by zero");

			if (dividend.is_zero())
			{
				*this = 0;
				if (out_remainder)
					*out_remainder = 0;
				return;
			}

			const Sign result_sign =
			  (static_cast<i8>(dividend.sign) * static_cast<i8>(divisor.sign)) >= 0 ? Sign::positive : Sign::negative;

			bigint abs_dividend = dividend;
			bigint abs_divisor  = divisor;
			abs_dividend.sign   = Sign::positive;
			abs_divisor.sign    = Sign::positive;

			if (compare_magnitude(abs_dividend, abs_divisor) == Compare::Less)
			{
				*this = 0;
				if (out_remainder)
					*out_remainder = std::move(abs_dividend);
				return;
			}

			digits.clear();
			digits.reserve(abs_dividend.digits.size());
			bigint remainder(0);

			for (auto i = static_cast<i64>(abs_dividend.digits.size()) - 1; i >= 0; --i)
			{
				remainder = remainder.multiply_by_digit(base);
				remainder += bigint(static_cast<u64>(abs_dividend.digits[i]));

				type   quotient_digit = 0;
				bigint last_product;

				if (remainder >= abs_divisor)
				{
					i32 low = 0, high = static_cast<i32>(base - 1);
					while (low <= high)
					{
						i32    mid     = low + (high - low) / 2;
						bigint product = abs_divisor.multiply_by_digit(static_cast<type>(mid));

						if (product <= remainder)
						{
							quotient_digit = static_cast<type>(mid);
							last_product   = std::move(product);
							low            = mid + 1;
						}
						else
						{
							high = mid - 1;
						}
					}
				}

				remainder -= last_product;
				digits.push_back(quotient_digit);
			}

			std::ranges::reverse(digits);
			sign = result_sign;
			remove_trailing_zeros();

			if (out_remainder)
			{
				bigint rem     = std::move(remainder);
				*out_remainder = std::move(rem);
			}
		}

		[[nodiscard]] static std::pair<bigint, bigint> divmod(const bigint& n, const bigint& d)
		{
			bigint q, r;
			q.divide(n, d, &r);
			return {std::move(q), std::move(r)};
		}

		void modulus(const bigint& lhs, const bigint& rhs)
		{
			if (rhs.is_zero())
			{
				dbg::panic("Modulus by zero");
			}

			if (lhs.is_zero())
			{
				*this = 0;
				return;
			}

			if (compare_magnitude(lhs, rhs) == Compare::Less)
			{
				if (this != &lhs)
					*this = lhs;
				return;
			}

			bigint abs_lhs = lhs;
			bigint abs_rhs = rhs;
			abs_lhs.sign   = Sign::positive;
			abs_rhs.sign   = Sign::positive;

			bigint quotient;
			bigint remainder;
			quotient.divide(abs_lhs, abs_rhs, &remainder);

			if (remainder.is_zero())
			{
				*this = 0;
			}
			else
			{
				*this = remainder;
				sign  = lhs.is_negative() ? Sign::negative : Sign::positive;
			}
		}

		void sqrt_op(const bigint& rhs)
		{
			if (rhs.is_zero() or rhs.is_negative())
			{
				*this = 0;
				return;
			}

			if (rhs == 1)
			{
				*this = 1;
				return;
			}

			bigint x = rhs;
			bigint x_prev;

			while (true)
			{
				x_prev = x;
				x      = (x + rhs / x) >> 1;

				if (x >= x_prev)
					break;
			}

			*this = std::move(x_prev);
		}

		[[nodiscard]] bigint multiply_by_digit(type digit) const
		{
			if (digit == 0)
				return bigint(0);
			if (digit == 1)
				return *this;

			bigint result;
			result.digits.clear();
			result.digits.reserve(digits.size() + 1);

			u64 carry = 0;
			for (const auto& d : digits)
			{
				const u64 prod = static_cast<u64>(d) * digit + carry;
				result.digits.push_back(static_cast<type>(prod % base));
				carry = prod / base;
			}

			if (carry)
				result.digits.push_back(static_cast<type>(carry));

			result.sign = sign;
			return result;
		}

		// Helper: flip sign without creating a copy
		void negate() noexcept
		{
			if (sign == Sign::positive)
				sign = Sign::negative;
			else if (sign == Sign::negative)
				sign = Sign::positive;
		}

		[[nodiscard]] bigint from_string(std::string_view input, i32 newbase = 0)
		{
			if (input.empty())
				return {};
			if (input.size() == 1 and (input[0] == '-' or input[0] == '0'))
				return {};

			bool neg = false;
			if (input[0] == '-')
			{
				input.remove_prefix(1);
				neg = true;
			}

			if (newbase == 0)
			{
				if (input.size() >= 2)
				{
					if (input[0] == '0' and (input[1] == 'x' or input[1] == 'X'))
					{
						newbase = 16;
						input.remove_prefix(2);
					}
					else if (input[0] == '0' and (input[1] == 'b' or input[1] == 'B'))
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
				else
					newbase = 10;
			}

			bigint result;
			for (const char c : input)
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

			if (neg)
				result.negate(); // mutate in place — no copy
			return result;
		}

	public:
		// -------------------------------------------------------------------------
		// Construction
		// -------------------------------------------------------------------------

		bigint()
		{
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

		bigint(const bigint&) noexcept       = default;
		bigint(bigint&&) noexcept            = default;
		bigint& operator=(bigint&&) noexcept = default;

		explicit bigint(const char* str) { *this = from_string(str); }

		explicit bigint(std::string_view input)
		{
			if (input.empty())
			{
				digits.push_back(0);
				sign = Sign::zero;
				return;
			}
			*this = from_string(input);
		}

		bigint& operator=(const char* str)
		{
			assert::check(str != nullptr, "bigint: null string pointer");
			return *this = from_string(str);
		}

		bigint& operator=(std::string_view input)
		{
			*this = from_string(input);
			return *this;
		}

		bigint& operator=(const bigint&) = default;

		template<std::unsigned_integral T>
		bigint& operator=(T const rhs)
		{
			digits.clear();
			if (rhs == 0)
			{
				digits.push_back(0);
				sign = Sign::zero;
				return *this;
			}
			sign = Sign::positive;

			u64 temp = rhs;
			while (temp > 0)
			{
				digits.push_back(static_cast<type>(temp % base));
				temp /= base;
			}
			return *this;
		}

		template<std::signed_integral T>
		bigint& operator=(T const rhs)
		{
			digits.clear();
			if (rhs == 0)
			{
				digits.push_back(0);
				sign = Sign::zero;
				return *this;
			}
			sign = (rhs > 0) ? Sign::positive : Sign::negative;

			const i64 as_i64 = static_cast<i64>(rhs);
			u64       temp   = (as_i64 < 0) ? (~static_cast<u64>(as_i64) + 1u) : static_cast<u64>(as_i64);
			while (temp > 0)
			{
				digits.push_back(static_cast<type>(temp % base));
				temp /= base;
			}
			return *this;
		}

		// -------------------------------------------------------------------------
		// Element access
		// -------------------------------------------------------------------------

		[[nodiscard]] type& operator[](size_t index)
		{
			assert::check(index < digits.size(), "Out-of-bound indexing on bigint");
			return digits[index];
		}

		[[nodiscard]] const type& operator[](size_t index) const
		{
			assert::check(index < digits.size(), "Out-of-bound indexing on bigint");
			return digits[index];
		}

		// -------------------------------------------------------------------------
		// Unary operators
		// -------------------------------------------------------------------------

		[[nodiscard]] bigint operator+() const& { return *this; }

		[[nodiscard]] bigint operator+() and { return std::move(*this); }

	

		[[nodiscard]] bigint operator-() const
		{
			bigint result(*this);
			result.negate();
			return result;
		}

		// -------------------------------------------------------------------------
		// Comparison
		// -------------------------------------------------------------------------

		[[nodiscard]] bool operator==(const bigint& rhs) const noexcept { return compare(*this, rhs) == Compare::Equal; }

		[[nodiscard]] bool operator<(const bigint& rhs) const noexcept { return compare(*this, rhs) == Compare::Less; }

		[[nodiscard]] bool operator<=(const bigint& rhs) const noexcept { return compare(*this, rhs) != Compare::Greater; }

		[[nodiscard]] bool operator>(const bigint& rhs) const noexcept { return compare(*this, rhs) == Compare::Greater; }

		[[nodiscard]] bool operator>=(const bigint& rhs) const noexcept { return compare(*this, rhs) != Compare::Less; }

		[[nodiscard]] bool operator==(std::string_view rhs) const { return *this == bigint(rhs); }

		// -------------------------------------------------------------------------
		// Queries
		// -------------------------------------------------------------------------

		[[nodiscard]] Sign signum() const noexcept { return sign; }

		[[nodiscard]] size_t size() const noexcept { return digits.size(); }

		[[nodiscard]] bool empty() const noexcept { return digits.empty(); }

		[[nodiscard]] bool is_zero() const noexcept { return sign == Sign::zero; }

		[[nodiscard]] bool is_negative() const noexcept { return sign == Sign::negative; }

		[[nodiscard]] bool is_positive() const noexcept { return sign == Sign::positive; }

		[[nodiscard]] type bit_length() const
		{
			if (is_zero())
				return 0;

			bigint temp = abs();
			type   bits = 0;
			while (not temp.is_zero())
			{
				temp.divide(temp, bigint(2));
				bits++;
			}
			return bits;
		}

		[[nodiscard]] size_t count() const
		{
			if (sign == Sign::zero)
				return 1;
			if (digits.empty())
				return 0;
			return (digits.size() - 1) * base_digits + count_digits(digits.back());
		}

		[[nodiscard]] size_t popcount() const
		{
			if (is_zero())
				return 0;

			static const bigint two32(4'294'967'296ULL);

			bigint temp   = abs();
			size_t result = 0;
			while (not temp.is_zero())
			{
				bigint rem;
				bigint q;
				q.divide(temp, two32, &rem);
				result += std::popcount(rem.to_integer<u32>().value_or(0u));
				temp = std::move(q);
			}
			return result;
		}

		[[nodiscard]] auto back() const { return digits.back(); }

		[[nodiscard]] auto rbegin() const { return digits.rbegin(); }

		[[nodiscard]] auto rend() const { return digits.rend(); }

		// -------------------------------------------------------------------------
		// Base conversion helpers
		// -------------------------------------------------------------------------

		[[nodiscard]] static std::string
		dc_to_string(const bigint& n, int newbase, std::span<const bigint> powers, size_t level, std::string_view chars)
		{
			auto small = n.to_integer<u64>();
			if (small.has_value())
			{
				u64 val = small.value();
				if (val == 0)
					return std::string(1, chars[0]);
				std::string result;
				while (val > 0)
				{
					result.push_back(chars[val % static_cast<u64>(newbase)]);
					val /= static_cast<u64>(newbase);
				}
				std::ranges::reverse(result);
				return result;
			}

			if (level == 0)
			{
				return dc_to_string(n, newbase, powers, 1, chars);
			}

			auto [q, r]      = divmod(n, powers[level - 1]);
			std::string high = dc_to_string(q, newbase, powers, level - 1, chars);
			std::string low  = dc_to_string(r, newbase, powers, level - 1, chars);

			const size_t low_width = size_t{1} << (level - 1);
			while (low.size() < low_width)
				low.insert(low.begin(), chars[0]);

			return high + low;
		}

		// -------------------------------------------------------------------------
		// Conversion
		// -------------------------------------------------------------------------

		std::string to_string(int newbase = 10, bool uppercase = false) const
		{
			if (digits.empty() or (digits.size() == 1 and digits[0] == 0))
				return "0";

			if (newbase == 10)
			{
				std::string result;
				result.reserve(digits.size() * base_digits + 1);

				if (sign == Sign::negative)
					result += '-';
				result += std::format("{}", digits.back());
				for (auto it = digits.rbegin() + 1; it != digits.rend(); ++it)
					result += std::format("{:0{}}", *it, base_digits);
				return result;
			}

			assert::check(newbase >= 2 && newbase <= 36, "Base must be in the range [2, 36]");

			static constexpr std::string_view lower_chars = "0123456789abcdefghijklmnopqrstuvwxyz";
			static constexpr std::string_view upper_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			const auto&                       chars       = uppercase ? upper_chars : lower_chars;

			std::string result;
			if (sign == Sign::negative)
				result += '-';

			const bigint value = (sign == Sign::negative) ? abs() : *this;

			if (std::has_single_bit(static_cast<unsigned>(newbase)))
			{
				const int bits_per_digit = std::countr_zero(static_cast<unsigned>(newbase));

				static const bigint two32(4'294'967'296ULL);
				std::vector<bigint> pow2_32;
				pow2_32.push_back(two32);
				while (pow2_32.back() <= value)
					pow2_32.push_back(pow2_32.back() * pow2_32.back());

				std::vector<u32> bin_limbs;
				bin_limbs.reserve((value.count() * 10 + 95) / 96); // ceil(decimal_digits * log2(10) / 32)
				auto             collect = [&](this auto& self, const bigint& n, size_t level) -> void
				{
					auto small = n.to_integer<u64>();
					if (small.has_value())
					{
						u64 v = small.value();
						bin_limbs.push_back(static_cast<u32>(v & 0xFFFF'FFFFu));
						if (v > 0xFFFF'FFFFu)
							bin_limbs.push_back(static_cast<u32>(v >> 32));
						return;
					}
					if (level == 0)
					{
						bin_limbs.push_back(n.to_integer<u32>().value_or(0u));
						return;
					}
					auto [q, r]          = divmod(n, pow2_32[level - 1]);
					const size_t r_limbs = size_t{1} << (level - 1);
					self(r, level - 1);

					while (bin_limbs.size() % r_limbs != 0 or bin_limbs.size() < r_limbs)
						bin_limbs.push_back(0u);
					self(q, level - 1);
				};

				collect(value, pow2_32.size());

				while (bin_limbs.size() > 1 and bin_limbs.back() == 0u)
					bin_limbs.pop_back();

				std::string digits_str;
				digits_str.reserve(bin_limbs.size() * 32 / bits_per_digit + 1);
				const u32 mask    = static_cast<u32>((1u << bits_per_digit) - 1u);
				size_t    limb_i  = 0;
				u64       window  = bin_limbs[0];
				int       avail   = 32;

				while (limb_i < bin_limbs.size() or avail >= bits_per_digit)
				{
					if (avail < bits_per_digit)
					{
						++limb_i;
						if (limb_i >= bin_limbs.size())
							break;
						window |= static_cast<u64>(bin_limbs[limb_i]) << avail;
						avail += 32;
					}
					digits_str.push_back(chars[window & mask]);
					window >>= bits_per_digit;
					avail -= bits_per_digit;
				}

				std::ranges::reverse(digits_str);
				auto first_nonzero = digits_str.find_first_not_of(chars[0]);
				result += (first_nonzero == std::string::npos) ? std::string(1, chars[0]) : digits_str.substr(first_nonzero);
				return result;
			}

			std::vector<bigint> powers;
			powers.emplace_back(newbase);
			while (powers.back() <= value)
				powers.push_back(powers.back() * powers.back());

			const size_t start_level = powers.size() - 1;

			std::string body = dc_to_string(value, newbase, std::span<const bigint>(powers), start_level, chars);

			auto first = body.find_first_not_of(chars[0]);
			result += (first == std::string::npos) ? std::string(1, chars[0]) : body.substr(first);
			return result;
		}

		template<std::integral T = i64>
		[[nodiscard]] std::expected<T, std::string> to_integer() const
		{
			if (is_zero() or empty())
				return T{0};
			if (count() > count_digits(std::numeric_limits<T>::max()))
				return std::unexpected("bigint too large for integer conversion");

			u64 result = 0, multiplier = 1;
			for (const auto& digit : digits)
			{
				result += static_cast<u64>(digit) * multiplier;
				multiplier *= base;
			}

			if constexpr (std::is_signed_v<T>)
				if (sign == Sign::negative)
					return static_cast<T>(-static_cast<i64>(result));

			return as<T>(result);
		}

		// -------------------------------------------------------------------------
		// Increment / Decrement
		// -------------------------------------------------------------------------

		bigint& operator++()
		{
			if (is_zero())
			{
				*this = 1;
				return *this;
			}
			if (sign == Sign::negative and digits.size() == 1 and digits[0] == 1)
			{
				*this = 0;
				return *this;
			}

			if (sign == Sign::negative)
				--digits[0];
			else
				++digits[0];

			type carry = digits[0] / base;
			digits[0] %= base;
			for (size_t i = 1; i < digits.size() and carry; ++i)
			{
				digits[i] += carry;
				carry = digits[i] / base;
				digits[i] %= base;
			}
			if (carry)
				digits.push_back(carry);

			if (digits.empty() or (digits.size() == 1 and digits[0] == 0))
				sign = Sign::zero;

			return *this;
		}

		bigint operator++(int)
		{
			bigint tmp(*this);
			++(*this);
			return tmp;
		}

		bigint& operator--()
		{
			if (is_zero())
			{
				*this = -1;
				return *this;
			}
			if (sign == Sign::positive and digits.size() == 1 and digits[0] == 1)
			{
				*this = 0;
				return *this;
			}

			if (digits[0] == 0)
			{
				digits[0]  = base - 1;
				type carry = 1;
				for (size_t i = 1; i < digits.size() and carry; ++i)
				{
					if (digits[i] < carry)
					{
						digits[i] += base - carry;
						carry = 1;
					}
					else
					{
						digits[i] -= carry;
						carry = 0;
					}
				}
			}
			else
			{
				if (sign == Sign::positive)
					--digits[0];
				else
					++digits[0];
			}

			remove_trailing_zeros();
			return *this;
		}

		bigint operator--(int)
		{
			bigint tmp(*this);
			--(*this);
			return tmp;
		}

		// -------------------------------------------------------------------------
		// Shifts
		// -------------------------------------------------------------------------

		[[nodiscard]] bigint operator<<(type shift) const
		{
			bigint r;
			r.shift_left(*this, shift);
			return r;
		}

		[[nodiscard]] bigint operator>>(type shift) const
		{
			bigint r;
			r.shift_right(*this, shift);
			return r;
		}

		bigint& operator<<=(type shift)
		{
			shift_left(*this, shift);
			return *this;
		}

		bigint& operator>>=(type shift)
		{
			shift_right(*this, shift);
			return *this;
		}

		// -------------------------------------------------------------------------
		// Arithmetic
		// -------------------------------------------------------------------------

		[[nodiscard]] bigint operator+(const bigint& rhs) const
		{
			bigint r;
			r.add(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator-(const bigint& rhs) const
		{
			bigint r;
			r.subtract(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator*(const bigint& rhs) const
		{
			bigint r;
			r.multiply(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator/(const bigint& rhs) const
		{
			bigint r;
			r.divide(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator%(const bigint& rhs) const
		{
			bigint r;
			r.modulus(*this, rhs);
			return r;
		}

		bigint& operator+=(const bigint& rhs)
		{
			add(*this, rhs);
			return *this;
		}

		bigint& operator-=(const bigint& rhs)
		{
			subtract(*this, rhs);
			return *this;
		}

		bigint& operator*=(const bigint& rhs)
		{
			*this = *this * rhs;
			return *this;
		}

		bigint& operator/=(const bigint& rhs)
		{
			divide(*this, rhs);
			return *this;
		}

		bigint& operator%=(const bigint& rhs)
		{
			modulus(*this, rhs);
			return *this;
		}

		// -------------------------------------------------------------------------
		// Bitwise
		// -------------------------------------------------------------------------

		[[nodiscard]] bigint operator&(const bigint& rhs) const
		{
			bigint r;
			r.bit_and(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator|(const bigint& rhs) const
		{
			bigint r;
			r.bit_or(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator^(const bigint& rhs) const
		{
			bigint r;
			r.bit_xor(*this, rhs);
			return r;
		}

		[[nodiscard]] bigint operator~() const
		{
			bigint r;
			r.bit_not(*this);
			return r;
		}

		bigint& operator&=(const bigint& rhs)
		{
			operator=(operator&(rhs));
			return *this;
		}

		bigint& operator|=(const bigint& rhs)
		{
			operator=(operator|(rhs));
			return *this;
		}

		bigint& operator^=(const bigint& rhs)
		{
			operator=(operator^(rhs));
			return *this;
		}

		// -------------------------------------------------------------------------
		// Math
		// -------------------------------------------------------------------------

		[[nodiscard]] bigint pow(const bigint& exponent) const
		{
			if (exponent.is_zero())
				return bigint(1);
			if (exponent.is_negative())
				return bigint(0);

			bigint result(1);
			bigint base_copy(*this);
			bigint exp = exponent;

			while (exp > 0)
			{
				if (exp.digits[0] % 2 == 1)
					result *= base_copy;
				base_copy *= base_copy;
				exp >>= 1;
			}

			result.sign = sign;
			return result;
		}

		[[nodiscard]] bigint mod(const bigint& rhs) const { return *this % rhs; }

		[[nodiscard]] bigint sqrt() const
		{
			bigint r;
			r.sqrt_op(*this);
			return r;
		}

		[[nodiscard]] bigint abs() const
		{
			bigint r(*this);
			r.sign = (r.is_zero() ? Sign::zero : Sign::positive);
			return r;
		}

		[[nodiscard]] bigint abs()
		{
			bigint r(*this);
			r.sign = (r.is_zero() ? Sign::zero : Sign::positive);
			return r;
		}

		bigint& abs_in_place() noexcept
		{
			if (!is_zero())
				sign = Sign::positive;
			return *this;
		}

	private:
		void bit_and(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
			{
				*this = lhs;
				return;
			}
			if (rhs.is_zero())
			{
				*this = rhs;
				return;
			}
			bitwise(lhs, rhs, std::bit_and<>{});
		}

		void bit_or(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
			{
				*this = rhs;
				return;
			}
			if (rhs.is_zero())
			{
				*this = lhs;
				return;
			}
			bitwise(lhs, rhs, std::bit_or<>{});
		}

		void bit_xor(const bigint& lhs, const bigint& rhs)
		{
			if (lhs.is_zero())
			{
				*this = rhs;
				return;
			}
			if (rhs.is_zero())
			{
				*this = lhs;
				return;
			}
			if (lhs == rhs)
			{
				*this = 0;
				return;
			}
			bitwise(lhs, rhs, std::bit_xor<>{});
		}

		void bit_not(const bigint& lhs)
		{
			if (lhs.is_zero())
			{
				digits = {1};
				sign   = Sign::positive;
				return;
			}

			type bit_len = lhs.bit_length();
			*this        = lhs ^ ((bigint(1) << bit_len) - 1);
		}
	}; // end class bigint

	export [[nodiscard]] bigint abs(const bigint& a) { return a.abs(); }

	export [[nodiscard]] bigint sqrt(const bigint& a) { return a.sqrt(); }

	export [[nodiscard]] bigint pow(const bigint& a, const bigint& b) { return a.pow(b); }

	export [[nodiscard]] bigint mod(const bigint& a, const bigint& b) { return a.mod(b); }

	export [[nodiscard]] bigint gcd(const bigint& a_orig, const bigint& b_orig)
	{
		bigint a = a_orig;
		bigint b = b_orig;
		a.abs_in_place();
		b.abs_in_place();

		int iteration = 0;
		while (true)
		{
			if (b.is_zero())
			{
				return a;
			}

			a %= b;

			if (a.is_zero())
			{
				return b;
			}

			b %= a;

			iteration++;
			if (iteration > 100)
			{
				break;
			}
		}
		return a;
	}

	export [[nodiscard]] bigint lcm(const bigint& a, const bigint& b)
	{
		if (a.is_zero() or b.is_zero())
			return bigint(0);
		return (a / gcd(a, b)) * b;
	}

	export template<std::integral T>
	[[nodiscard]] bigint operator*(T lhs, const bigint& rhs)
	{
		return bigint{lhs} * rhs;
	}

	export template<std::integral T>
	[[nodiscard]] bigint operator+(T lhs, const bigint& rhs)
	{
		return bigint{lhs} + rhs;
	}

	export template<std::integral T>
	[[nodiscard]] bigint operator-(T lhs, const bigint& rhs)
	{
		return bigint{lhs} - rhs;
	}

	export [[nodiscard]] size_t popcount(const bigint& a) { return a.popcount(); }

	export [[nodiscard]] bigint random_range(const bigint& start, const bigint& end)
	{
		if (end < start)
			dbg::panic("End must be greater than start");

		std::random_device                 rd;
		std::mt19937                       gen(rd());
		std::uniform_int_distribution<u32> dist(0, 10);

		const bigint range     = abs(end - start) + 1;
		const size_t range_cnt = range.count();

		bigint result;
		do
		{
			result = 0;
			for (size_t i = 0; i < range_cnt; ++i)
				result = result * 10 + dist(gen);
		} while (result >= range);

		return start + result;
	}

	export [[nodiscard]] bigint random_bigint(u32 keysize) { return bigint(random::digit(keysize)); }

	export [[nodiscard]] bigint random_bigint(const bigint& /*range*/) { return {}; } // TODO

	export [[nodiscard]] bigint random_prime(size_t /*keysize*/ = 0) { return {}; }   // TODO

	[[nodiscard]] bigint midpoint(const bigint& a, const bigint& b)
	{
		if (a <= b)
			return a + (b - a) / 2;
		else
			return a - (a - b) / 2;
	}

} // namespace deckard

// -------------------------------------------------------------------------
// std specializations
// -------------------------------------------------------------------------

export namespace std
{
	using namespace deckard;

	template<>
	struct hash<bigint>
	{
		[[nodiscard]] size_t operator()(const bigint& value) const { return deckard::utils::hash_values(value.to_string()); }
	};

	template<>
	struct formatter<bigint>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto pos = ctx.begin();
			while (pos != ctx.end() and *pos != '}')
			{
				switch (*pos)
				{
					case 'x': parsed_base = 16; break;
					case 'X':
						parsed_base = 16;
						uppercase   = true;
						break;
					case 'o': parsed_base = 8; break;
					case 'O':
						parsed_base = 8;
						uppercase   = true;
						break;
					case 'd': parsed_base = 10; break;
					case 'D':
						parsed_base = 10;
						uppercase   = true;
						break;
					case 'b':
					case 'B':
					{
						++pos;
						parsed_base = 0;
						while (pos != ctx.end() and *pos >= '0' and *pos <= '9')
						{
							parsed_base = parsed_base * 10 + (*pos - '0');
							++pos;
						}
						if (parsed_base < 2 or parsed_base > 36)
							dbg::panic("Base invalid");
						continue;
					}
					default: break;
				}
				++pos;
			}
			if (pos != ctx.end() and *pos != '}')
				throw std::format_error("Invalid format");
			return pos;
		}

		auto format(const bigint& v, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "{}", v.to_string(parsed_base, uppercase));
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};
} // namespace std
