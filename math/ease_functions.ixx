export module deckard.math:easing;

import deckard.types;
import deckard.as;
import :utils;

export namespace deckard::math
{

	// lerp
	template<arithmetic T>
	[[nodiscard]] constexpr T lerp(T A, T B, T Alpha) noexcept
	{
		return as<T>(A + Alpha * (B - A));
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T smoothstep(T x) noexcept
	{
		return x * x * (3.0f - 2.0f * x);
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr T inverse_smoothstep(T x) noexcept
	{
		return 0.5 - sin(asin(1.0 - 2.0 * x) / 3.0);
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T smootherstep(T x) noexcept
	{
		return x * x * x * (x * (6.0f * x - 15.0f) + 10.0f);
	}

	// quadratic
	template<arithmetic T>
	[[nodiscard]] constexpr T quadratic_ease_in(T p) noexcept
	{
		return p * p;
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T quadratic_ease_out(T p) noexcept
	{
		return -(p * (p - 2));
	}

	namespace cubic
	{
		// cubic
		template<arithmetic T>
		[[nodiscard]] constexpr T in(T p) noexcept
		{
			return p * p * p;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p) noexcept
		{
			T f = (p - 1);
			return f * f * f + 1;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T inout(T p) noexcept
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
		[[nodiscard]] constexpr T in(T p) noexcept
		{
			return p * p * p * p;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p) noexcept
		{
			T f = (p - 1);
			return f * f * f * (1 - p) + 1;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T inout(T p) noexcept
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
		[[nodiscard]] constexpr T in(T p) noexcept
		{
			return std::sin((p - 1) * std::numbers::pi_v<T>) + 1;
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p) noexcept
		{
			return std::sin(p * std::numbers::pi_v<T>);
		}

		template<arithmetic T>
		[[nodiscard]] constexpr T inout(T p) noexcept
		{
			return as<T>(0.5 * (1 - std::cos(p * std::numbers::pi_v<T>)));
		}
	} // namespace sine

	namespace bounce
	{

		template<arithmetic T>
		[[nodiscard]] constexpr T out(T p) noexcept
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
		[[nodiscard]] constexpr T in(T p) noexcept
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
