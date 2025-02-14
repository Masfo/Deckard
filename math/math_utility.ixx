module;
// #include <algorithm>
// #include <functional>
#include <immintrin.h>
// #include <pmmintrin.h>
// #include <xmmintrin.h>


export module deckard.math.utils;
import std;
import deckard.types;
import deckard.as;
import deckard.assert;

export namespace deckard::math
{


	template<std::floating_point T = float>
	constexpr T deg_to_radians = std::numbers::pi_v<T> / T{180};

	template<std::floating_point T = float>
	constexpr T rad_to_degrees = T{180} / std::numbers::pi_v<T>;

	std::integral auto align_integer(std::integral auto value, std::integral auto alignment = 4)
	{
		assert::check(alignment > 0, "Alignment must be greater that zero");

		auto remainder = value % alignment;

		if (remainder == 0)
			return value;

		auto offset = alignment - remainder;
		return value + offset;
	}

	template<std::integral T, arithmetic U>
	T round_to_even(U num)
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

	// remap
	template<arithmetic T>
	T remap(const T& X, const T& minimum, const T& maximum, const T& newminimum, const T& newmaximum)
	{
		return newminimum + (X - minimum) * (newmaximum - newminimum) / (maximum - minimum);
	}

	// mod
	template<arithmetic T, arithmetic U>
	T mod(T x, U N)
	{
		if (N == U{0})
			return 0;
		return as<T>((x % N + N) % N);
	}

	// absolute difference
	template<arithmetic T, arithmetic U>
	T abs_diff(T x, U y)
	{
		return std::abs(x - y);
	}

	// is_close_enough


	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = T{1e-5})
	{
		const T diff    = A - B;
		const T absdiff = std::abs(diff);
		return absdiff <= error;
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough_zero(const T& A, const T error = T{1e-5})
	{
		return std::abs(A) <= error;
	}

	template<std::integral T>
	[[nodiscard]] constexpr bool is_close_enough_zero(const T& A)
	{
		return std::abs<T>(A) == T{0};
	}

	// is_close_enough
	template<std::integral T>
	[[nodiscard]] constexpr bool is_close_enough(const T& A, const T& B, const T error = T{0})
	{
		return std::abs(A - B) == error;
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T to_radians(const T& v)
	{
		return static_cast<T>(v * deg_to_radians<T>);
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T to_degrees(const T& v)
	{
		return static_cast<T>(v * rad_to_degrees<T>);
	}

	constexpr f32 operator""_rad(long double deg) { return to_radians<f32>(as<f32>(deg)); }

	constexpr f32 operator""_deg(long double rad) { return to_degrees<f32>(as<f32>(rad)); }

	namespace sse
	{
		using m128 = __m128;

		// sse
		__m128 dot(const m128& rhs)
		{
#if 0
		// SSE3
		m128 tmp0 = _mm_hadd_ps(rhs, rhs);
		m128 tmp1 = _mm_hadd_ps(tmp0, tmp0);
		return tmp1;
#else
			m128 tmp0 = _mm_add_ps(rhs, _mm_movehl_ps(rhs, rhs));
			m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));
			return tmp1;
#endif
		};

		float dotf(const m128& lhs) { return _mm_cvtss_f32(dot(lhs)); };

		export float sqrt(float f) { return _mm_cvtss_f32(_mm_sqrt_ps(_mm_set_ps1(f))); }

		// ~2x faster than std::sin
		export float sin(float f)
		{
			// return _mm_cvtss_f32(_mm_sin_ps(_mm_set_ps1(f)));
			return _mm_cvtss_f32(_mm_set_ps1(std::sin(f)));
		}

		export float cos(float f)
		{
			// return _mm_cvtss_f32(_mm_cos_ps(_mm_set_ps1(f)));
			return _mm_cvtss_f32(_mm_set_ps1(std::cos(f)));
		}

	} // namespace sse
} // namespace deckard::math

export namespace deckard::math
{
	template<std::unsigned_integral T>
	auto egcd(T a, T b) -> std::tuple<T, T, T>
	{
		T x1 = 1, y1 = 0, x2 = 0, y2 = 1;
		T temp{};

		while (b != 0)
		{
			T q = a / b;
			T r = a % b;

			temp = x1;
			x1   = x2;
			x2   = temp - q * x2;

			temp = y1;
			y1   = y2;
			y2   = temp - q * y2;

			a = b;
			b = r;
		}


		return {a, x1, y1};
	}

	i32 sqrt(i32 v)
	{
		u32 b = 1 << 30, q = 0, r = v;
		while (b > r)
			b >>= 2;
		while (b > 0)
		{
			u32 t = q + b;
			q >>= 1;
			if (r >= t)
			{
				r -= t;
				q += b;
			}
			b >>= 2;
		}
		return q;
	}

	i64 sqrt(i64 v)
	{
		u64 b = ((u64)1) << 62, q = 0, r = v;
		while (b > r)
			b >>= 2;
		while (b > 0)
		{
			u64 t = q + b;
			q >>= 1;
			if (r >= t)
			{
				r -= t;
				q += b;
			}
			b >>= 2;
		}
		return q;
	}

	template<std::integral T, std::integral U = T>
	constexpr T index_from_2d(T x, T y, U width)
	{
		return y * width + x;
	}

	template<std::integral T, std::integral U = T, std::integral I = T>
	constexpr T index_from_3d(T x, T y, T z, U height, I depth)
	{
		return (z * height * depth) + (y * depth) + x;
	}

	template<std::integral T>
	constexpr bool is_prime(T n)
	{

		if (n <= 1)
			return false;

		if (n <= 3)
			return true;

		if (n % 2 == 0 || n % 3 == 0)
			return false;

		T i = 5;
		while (i * i <= n)
		{
			if (n % i == 0 || n % (i + 2) == 0)
				return false;

			i += 6;
		}

		return true;
	}

} // namespace deckard::math

namespace deckard::math
{

	
} // namespace deckard::math
