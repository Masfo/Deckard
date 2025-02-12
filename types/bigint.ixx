export module deckard.bigint;

import deckard.types;
import deckard.assert;
import deckard.math;
import deckard.helpers;
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


#if 1
		static constexpr u32 base        = 1000000;
		static constexpr u32 base_digits = 6;
#else
		static constexpr u32 base        = 1'000'000'000;
		static constexpr u32 base_digits = 9;
#endif
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
			if (lhs.digits.size() > rhs.digits.size())
				return Compare::Greater;
			else if (lhs.digits.size() < rhs.digits.size())
				return Compare::Less;
			else
			{
				for (size_t i = lhs.digits.size() - 1; i >= 0; i--)
				{
					if (lhs.digits[i] > rhs.digits[i])
						return Compare::Greater;
					else if (lhs.digits[i] < rhs.digits[i])
						return Compare::Less;
				}
				return Compare::Equal;
			}
		}


	public:
		bigint() = default;

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

		template<std::integral T>
		bigint& operator=(T const rhs)
		{
			T temp(rhs);

			digits.clear();
			digits.reserve(3);

			if (temp == 0)
				sign = Sign::zero;
			else if (temp > 0)
				sign = Sign::positive;
			else
			{
				sign = Sign::negative;
				temp *= -1;
			}

			while (temp > 0)
			{
				digits.push_back(math::mod<T>(temp, bigint::base));
				temp /= bigint::base;
			}

			return *this;
		}

		void operator-()
		{
			if (sign == Sign::positive)
				sign = Sign::negative;
			else if (sign == Sign::negative)
				sign = Sign::positive;
		}

		Sign signum() const { return sign; }

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
	};
} // namespace deckard

export namespace std
{
	using namespace deckard;

	//	template<>
	//	struct hash<bigint>
	//	{
	//		size_t operator()(const generic_vec2<T>& value) const { return deckard::utils::hash_values(value.x, value.y); }
	//	};
	//
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

			assert::check(v.empty() == false, "big_int empty");

			std::format_to(ctx.out(), "{}", v.back());

			auto it = v.rbegin();
			for (it++; it != v.rend(); ++it)
				std::format_to(ctx.out(), "{:04}", *it);

			return std::format_to(ctx.out(), "");
		}
	};
} // namespace std
