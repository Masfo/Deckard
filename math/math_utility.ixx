module;
#include <cmath>
#include <concepts>
#include <numbers>
#include <pmmintrin.h>
#include <xmmintrin.h>

export module deckard.math:utils;

export namespace deckard::math
{

	template<typename T>
	concept arithmetic = std::integral<T> or std::floating_point<T>;


	template<std::floating_point T = float>
	inline constexpr T pi180 = std::numbers::pi_v<T> / T{180};

	// is_close_enough
	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = T{1e-7})
	{
		return std::abs(A - B) <= error;
	}

	// is_close_enough
	template<std::integral T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = T{0})
	{
		return std::abs(A - B) == error;
	}

	// lerp
	template<arithmetic T, arithmetic U>
	[[nodiscard]] constexpr T lerp(const T& A, const T& B, const U& Alpha) noexcept
	{
		return static_cast<T>(A + Alpha * (B - A));
	}

	template<typename T>
	[[nodiscard]] constexpr T to_radians(const T& angle) noexcept
	{
		return angle * pi180<T>;
	}

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

	} // namespace sse
} // namespace deckard::math
