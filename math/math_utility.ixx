module;
#include <cmath>
#include <concepts>
#include <numbers>

#include <xmmintrin.h>

export module deckard.math.utils;

export namespace deckard::math
{
	template<class T>
	concept number = std::integral<T> && std::floating_point<T>;


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
	template<number T, number U>
	[[nodiscard]] constexpr T lerp(const T& A, const T& B, const U& Alpha) noexcept
	{
		return static_cast<T>(A + Alpha * (B - A));
	}

	template<typename T>
	[[nodiscard]] constexpr T to_radians(const T& angle) noexcept
	{
		return angle * pi180<T>;
	}

	// sse
	__m128 horizontal_add(const __m128& lhs) noexcept
	{
		__m128 shuf = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
		__m128 sums = _mm_add_ps(lhs, shuf);                             // sums = [ D+C C+D | B+A A+B ]
		shuf        = _mm_movehl_ps(shuf, sums); //  [   C   D | D+C C+D ]  // let the compiler avoid a mov by reusing shuf
		sums        = _mm_add_ss(sums, shuf);
		return sums;
	};

	float horizontal_addf(const __m128& lhs) noexcept { return _mm_cvtss_f32(horizontal_add(lhs)); };

	export float sse_sqrt(float f) noexcept { return _mm_cvtss_f32(_mm_sqrt_ps(_mm_set_ps1(f))); }


} // namespace deckard::math
