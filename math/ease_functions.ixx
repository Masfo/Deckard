export module deckard.math:easing;

import deckard.types;
import deckard.as;

export namespace deckard::math
{

	// lerp
	template<arithmetic T>
	[[nodiscard]] constexpr T lerp(T A, T B, T Alpha)
	{
		return as<T>(A + Alpha * (B - A));
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T smoothstep(T x)
	{
		return x * x * (T{3} - T{2} * x);
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr T inverse_smoothstep(T x)
	{
		return 0.5 - std::sin(std::asin(T{1} - T{2} * x) / T{3});
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T smootherstep(T x)
	{
		return x * x * x * (x * (T{6} * x - T{15}) + T{10});
	}

	// quadratic
	template<arithmetic T>
	[[nodiscard]] constexpr T quadratic_ease_in(T p)
	{
		return p * p;
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T quadratic_ease_out(T p)
	{
		return -(p * (p - 2));
	}

	namespace cubic
	{
		// cubic
		template<arithmetic T>
		[[nodiscard]] constexpr T in(T p)
		{
			return p * p * p;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p)
		{
			T f = (p - 1);
			return f * f * f + 1;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T inout(T p)
		{
			if (p < 0.5)
			{
				return 4 * p * p * p;
			}
			else
			{
				T f = ((2 * p) - 2);
				return 0.5 * f * f * f + 1;
			}
		}

	} // namespace cubic

	namespace quartic
	{
		// quartic
		template<arithmetic T>
		[[nodiscard]] constexpr T in(T p)
		{
			return p * p * p * p;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p)
		{
			T f = (p - 1);
			return f * f * f * (1 - p) + 1;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T inout(T p)
		{
			if (p < 0.5)
			{
				return 8 * p * p * p * p;
			}
			else
			{
				T f = (p - 1);
				return -8 * f * f * f * f + 1;
			}
		}
	} // namespace quartic

	namespace sine
	{
		// sine
		template<arithmetic T>
		[[nodiscard]] constexpr T in(T p)
		{
			return std::sin((p - 1) * std::numbers::pi_v<T>) + 1;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p)
		{
			return std::sin(p * std::numbers::pi_v<T>);
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T inout(T p)
		{
			return as<T>(0.5 * (1 - std::cos(p * std::numbers::pi_v<T>)));
		}
	} // namespace sine

	namespace bounce
	{

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p)
		{
			if (p < 4 / 11.0)
			{
				return as<T>((121 * p * p) / 16.0);
			}
			else if (p < 8 / 11.0)
			{
				return as<T>((363 / 40.0 * p * p) - (99 / 10.0 * p) + 17 / 5.0);
			}
			else if (p < 9 / 10.0)
			{
				return as<T>((4356 / 361.0 * p * p) - (35442 / 1805.0 * p) + 16061 / 1805.0);
			}
			else
			{
				return as<T>((54 / 5.0 * p * p) - (513 / 25.0 * p) + 268 / 25.0);
			}
		}

		// bounce
		template<arithmetic T>
		[[nodiscard]] constexpr T in(T p)
		{
			return 1 - out(1 - p);
		}

	} // namespace bounce

	// damp
	template<std::floating_point T>
	[[nodiscard]] T damp(T lambda, T dt)
	{
		return 1 - std::exp(-lambda * dt);
	}


} // namespace deckard::math
