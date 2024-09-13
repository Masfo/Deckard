module;
#include <algorithm>
#include <functional>
#include <immintrin.h>
#include <pmmintrin.h>
#include <xmmintrin.h>


export module deckard.math:utils;
import std;
import deckard.types;

export namespace deckard::math
{

	template<typename T>
	concept arithmetic = std::integral<T> or std::floating_point<T>;


	template<std::floating_point T = float>
	inline constexpr T pi180 = std::numbers::pi_v<T> / T{180};

	template<typename T>
	T round_to_even(f32 num)
	{
		T rounded = static_cast<T>(std::round(num));
		return rounded + (rounded % 2);
	}

	template<std::unsigned_integral T>
	T floor_pow2(T v)
	{
		return std::bit_floor(v);
	}

	template<std::unsigned_integral T>
	T ceil_pow2(T v)
	{
		return std::bit_ceil(v);
	}

	template<arithmetic T>
	[[nodiscard]] constexpr bool clamp01(const T x, const T lowerlimit = T{0}, const T upperlimit = T{1})
	{
		if (x < lowerlimit)
			return lowerlimit;
		if (x > upperlimit)
			return upperlimit;
		return x;
	}

	// mod
	template<std::integral T = i64>
	T mod(T x, T N) noexcept
	{
		if (N == T{0})
			return 0;
		return (x % N + N) % N;
	}

	// is_close_enough
	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = T{1e-7})
	{
		return std::abs(A - B) <= error;
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T error = T{1e-7})
	{
		return std::abs(A) <= error;
	}

	// is_close_enough
	template<arithmetic T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = T{0})
	{
		return std::abs(A - B) == error;
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T radians(const T& v) noexcept
	{
		return static_cast<T>(v * pi180<T>);
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T degrees(const T& v) noexcept
	{
		return static_cast<T>((v * T{180}) / std::numbers::pi_v<T>);
	}

	constexpr float operator""_rad(long double deg) { return radians<float>((float)deg); }

	constexpr float operator""_deg(long double rad) { return degrees<float>((float)rad); }

	namespace sse
	{
		// sse
		__m128 dot(const __m128& rhs) noexcept
		{
#if 0
		// SSE3
		__m128 tmp0 = _mm_hadd_ps(rhs, rhs);
		__m128 tmp1 = _mm_hadd_ps(tmp0, tmp0);
		return tmp1;
#else
			__m128 tmp0 = _mm_add_ps(rhs, _mm_movehl_ps(rhs, rhs));
			__m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));
			return tmp1;
#endif
		};

		float dotf(const __m128& lhs) noexcept
		{
			//
			return _mm_cvtss_f32(dot(lhs));
		};

		export float sqrt(float f) noexcept
		{
			//
			return _mm_cvtss_f32(_mm_sqrt_ps(_mm_set_ps1(f)));
		}

		// ~2x faster than std::sin
		export float sin(float f) noexcept { return _mm_cvtss_f32(_mm_sin_ps(_mm_set_ps1(f))); }

		//
		export float cos(float f) noexcept { return _mm_cvtss_f32(_mm_cos_ps(_mm_set_ps1(f))); }

	} // namespace sse
} // namespace deckard::math

namespace deckard::math
{

	export class bignum
	{
		using Type = u32;

	public:
		bignum() = default;

		bignum(std::string_view input) { init(input); }

		void init(std::string_view input) noexcept
		{
			//
			const i32 len        = input.size();
			const i32 num_digits = (len + 8) / 9;
			digits.resize(num_digits);

			for (i32 i = 0; i < len; i++)
			{
				i32 pos    = i / 9;
				i32 offset = i % 9;

				Type digit      = input[len - 1 - i] - '0';
				Type multiplier = 1;

				for (int j = 0; j < offset; j++)
					multiplier *= 10;

				digits[pos] += digit * multiplier;
			}
		}

		bignum add(const bignum& a, const bignum& b)
		{
			//
			bignum result;
			i32    max_size = std::max(a.size(), b.size());
			result.resize(max_size + 1);

			i32 i     = 0;
			u32 carry = 0;
			for (i = 0; i < max_size || carry; i++)
			{
				u32 sum = carry;
				if (i < a.size())
					sum += a[i];
				if (i < b.size())
					sum += b[i];
				result[i] = sum % 1000'000000;
				carry     = sum / 1000'000000;
			}
			result.resize(carry ? i + 1 : i);
			return result;
		}

		bignum sub(const bignum& a, const bignum& b)
		{
			//
			bignum result;

			return result;
		}

		std::string print() const noexcept
		{
			//
			std::string buffer;
			buffer.reserve(9 * size());
			bool leading = true;

			for (i32 i = size() - 1; i >= 0; i--)
			{
				if (leading)
				{
					std::format_to(std::back_inserter(buffer), "{}", digits[i]);
					leading = 0;
				}
				else
				{
					std::format_to(std::back_inserter(buffer), "{:09}", digits[i]);
				}
			}
			if (leading)
			{
				std::format_to(std::back_inserter(buffer), "0");
			}
			return buffer;
		}

		size_t size() const noexcept { return digits.size(); }

		Type operator[](size_t index) const noexcept { return digits[index]; }

		Type& operator[](size_t index) noexcept { return digits[index]; }


	private:
		void resize(size_t size) noexcept { digits.resize(size); };

		std::vector<Type> digits;
	};

} // namespace deckard::math
