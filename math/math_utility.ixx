export module deckard.math.utility;

import std;

export namespace deckard::math
{
	template<class T>
	concept number = std::integral<T> && std::floating_point<T>;

	template<std::floating_point T = float>
	inline constexpr T epsilon = T{1e-7};


	template<std::floating_point T = float>
	inline constexpr T pi180 = std::numbers::pi_v<T> / T{180};

	// is_close_enough
	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = epsilon)
	{
		return std::fabs(A - B) <= error;
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

} // namespace deckard::math
