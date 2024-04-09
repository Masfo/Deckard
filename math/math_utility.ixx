export module deckard.math.utility;

import std;

export namespace deckard::math
{
	template<class T>
	concept number = std::integral<T> && std::floating_point<T>;

	template<std::floating_point T>
	inline constexpr T default_float_tolerance = T{0.0000001};

	// is_close_enough
	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = default_float_tolerance)
	{
		return std::fabs(A - B) <= error;
	}

	// lerp
	template<number T, number U>
	[[nodiscard]] constexpr T lerp(const T& A, const T& B, const U& Alpha) noexcept
	{
		return static_cast<T>(A + Alpha * (B - A));
	}
} // namespace deckard::math
