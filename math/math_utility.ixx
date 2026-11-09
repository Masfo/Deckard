module;
// #include <algorithm>
// #include <functional>
#include <immintrin.h>
// #include <pmmintrin.h>
// #include <xmmintrin.h>


export module deckard.math.utils;
import std;
import deckard.types;
import deckard.assert;
import deckard.debug;

export namespace deckard::math
{

	template<typename T>
	concept MathOp = std::same_as<T, std::plus<>> or std::same_as<T, std::minus<>> or std::same_as<T, std::multiplies<>>
					 or std::same_as<T, std::divides<>>;

	template<std::integral T, MathOp Op>
	requires(not std::same_as<T, bool>)
	constexpr std::expected<T, std::string> checked_op(T a, T b, Op op)
	{
		if constexpr (std::same_as<Op, std::multiplies<>>)
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (a == 0 or b == 0)
					return T{};
				if (a > 0)
				{
					if (b > 0)
					{
						if (a > std::numeric_limits<T>::max() / b)
							return std::unexpected("integer overflow in multiplication");
					}
					else if (b < std::numeric_limits<T>::min() / a)
						return std::unexpected("integer underflow in multiplication");
				}
				else
				{
					if (b > 0)
					{
						if (a < std::numeric_limits<T>::min() / b)
							return std::unexpected("integer underflow in multiplication");
					}
					else if (a < std::numeric_limits<T>::max() / b)
						return std::unexpected("integer overflow in multiplication");
				}
			}
			else if (b != 0 and a > std::numeric_limits<T>::max() / b)
				return std::unexpected("integer overflow in multiplication");
		}
		else if constexpr (std::same_as<Op, std::plus<>>)
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (b > 0 and a > std::numeric_limits<T>::max() - b)
					return std::unexpected("integer overflow in addition");
				if (b < 0 and a < std::numeric_limits<T>::min() - b)
					return std::unexpected("integer underflow in addition");
			}
			else if (a > std::numeric_limits<T>::max() - b)
				return std::unexpected("integer overflow in addition");
		}
		else if constexpr (std::same_as<Op, std::minus<>>)
		{
			if constexpr (std::is_signed_v<T>)
			{
				if (b < 0 and a > std::numeric_limits<T>::max() + b)
					return std::unexpected("integer overflow in subtraction");
				if (b > 0 and a < std::numeric_limits<T>::min() + b)
					return std::unexpected("integer underflow in subtraction");
			}
			else if (a < b)
				return std::unexpected("integer underflow in subtraction");
		}
		else
		{
			if (b == 0)
				return std::unexpected("division by zero");
			if constexpr (std::is_signed_v<T>)
			{
				if (a == std::numeric_limits<T>::min() and b == -1)
					return std::unexpected("integer overflow in division");
			}
		}

		return op(a, b);
	}

	template<std::integral T>
	requires(not std::same_as<T, bool>)
	constexpr std::expected<T, std::string> safe_mul(T a, T b)
	{
		return checked_op(a, b, std::multiplies<>{});
	}

	template<std::integral T>
	requires(not std::same_as<T, bool>)
	constexpr std::expected<T, std::string> safe_add(T a, T b)
	{
		return checked_op(a, b, std::plus<>{});
	}

	template<std::integral T>
	requires(not std::same_as<T, bool>)
	constexpr std::expected<T, std::string> safe_sub(T a, T b)
	{
		return checked_op(a, b, std::minus<>{});
	}

	template<std::integral T>
	requires(not std::same_as<T, bool>)
	constexpr std::expected<T, std::string> safe_div(T a, T b)
	{
		return checked_op(a, b, std::divides<>{});
	}

	// Floating point safe math


	template<std::floating_point T, MathOp Op>
	[[nodiscard]] auto checked_op(T a, T b, std::string_view name, Op op) noexcept -> std::expected<T, std::string>
	{
		if (std::isnan(a) or std::isnan(b))
			return std::unexpected(std::format("NaN in input: a = {}, b = {}", a, b));
		if (std::isinf(a) or std::isinf(b))
			return std::unexpected(std::format("infinity in input: a = {}, b = {}", a, b));

		const T result = op(a, b);
		if (not std::isfinite(result))
			return std::unexpected(std::format("overflow in {}: a = {}, b = {}", name, a, b));

		return result;
	}

	template<std::floating_point T>
	[[nodiscard]] auto safe_mul(T a, T b) noexcept -> std::expected<T, std::string>
	{
		return checked_op(a, b, "multiplication", std::multiplies<>{});
	}

	template<std::floating_point T>
	[[nodiscard]] auto safe_div(T a, T b) noexcept -> std::expected<T, std::string>
	{
		if (b == T{0})
			return std::unexpected(std::format("division by zero: a = {}", a));

		return checked_op(a, b, "division", std::divides<>{});
	}

	template<std::floating_point T>
	[[nodiscard]] auto safe_add(T a, T b) noexcept -> std::expected<T, std::string>
	{
		return checked_op(a, b, "addition", std::plus<>{});
	}

	template<std::floating_point T>
	[[nodiscard]] auto safe_sub(T a, T b) noexcept -> std::expected<T, std::string>
	{
		return checked_op(a, b, "subtraction", std::minus<>{});
	}

	//

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

	template<std::unsigned_integral T>
	[[nodiscard]] constexpr T isqrt(T value) noexcept
	{
		if (value == 0)
			return 0;

		// Highest power of 4 <= value
		T bit = static_cast<T>(T{1} << ((std::bit_width(value) - 1) & ~1));
		T res = 0;

		while (bit != 0)
		{
			if (value >= res + bit)
			{
				value = static_cast<T>(value - (res + bit));
				res   = static_cast<T>((res >> 1) + bit);
			}
			else
			{
				res >>= 1;
			}
			bit >>= 2;
		}
		return res;
	}

	template<std::unsigned_integral T>
	[[nodiscard]] constexpr T ceil_isqrt(T value) noexcept
	{
		const T r = isqrt(value);
		return static_cast<T>(r + (r * r != value));
	}

	static_assert(ceil_isqrt<u32>(16) == 4); // 4 * 4 = 16 >= 16
	static_assert(ceil_isqrt<u32>(24) == 5); // 5 * 5 = 25 >= 24

	template<std::unsigned_integral T>
	[[nodiscard]] constexpr T floor_isqrt(T value) noexcept
	{
		return isqrt(value);
	}

	static_assert(floor_isqrt<u32>(24) == 4); // 4 * 4 = 16 <= 24
	static_assert(floor_isqrt<u32>(25) == 5); // 5 * 5 = 25 <= 25

	template<arithmetic T>
	[[nodiscard]] constexpr T clamp01(const T x, const T lowerlimit = T{0}, const T upperlimit = T{1})
	{
		if (x < lowerlimit)
			return lowerlimit;
		if (x > upperlimit)
			return upperlimit;
		return x;
	}

	// normalize

	template<arithmetic T>
	T normalize(T value, T start, T end)
	{
		assert::check(not is_close_enough_zero(end - start), "Lets not divide by zero");

		T result = (value - start) / (end - start);
		return result;
	}

	// remap
	template<arithmetic T>
	T remap(const T value, const T minimum, const T maximum, const T newminimum, const T newmaximum)
	{
		return newminimum + (value - minimum) * (newmaximum - newminimum) / (maximum - minimum);
	}

	// mod
	template<std::integral T, std::integral U>
	[[nodiscard]] constexpr T mod(T x, U n) noexcept
	{
		// Precondition: n != 0
		if constexpr (std::is_unsigned_v<T> and std::is_unsigned_v<U>)
		{
			return static_cast<T>(x % n);
		}
		else
		{
			using wide   = std::intmax_t;
			const wide a = static_cast<wide>(x);
			const wide b = static_cast<wide>(n);

			if (b == -1)
				return 0;                        // avoids INT_MIN % -1 UB

			wide r = a % b;
			if (r != 0 and ((r < 0) != (b < 0))) // result takes the sign of the divisor
				r += b;
			return static_cast<T>(r);
		}
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr T mod(T x, T n) noexcept
	{
		const T r = std::fmod(x, n);
		return (r != T{0} and ((r < T{0}) != (n < T{0}))) ? r + n : r;
	}

	// mix
	template<std::floating_point T>
	[[nodiscard]] constexpr T mix(T x, T y, T t) noexcept
	{
		return x * (T{1} - t) + y * t;
	}

	// absolute difference
	template<arithmetic T, arithmetic U>
	T abs_diff(T x, U y)
	{
		return std::abs(x - y);
	}

	// between values
	[[nodiscard]] constexpr auto is_between(arithmetic auto value, arithmetic auto lo, arithmetic auto hi) noexcept -> bool
	{
		using common_t = std::common_type_t<decltype(value), decltype(lo), decltype(hi)>;

		const auto v = static_cast<common_t>(value);
		const auto l = static_cast<common_t>(lo);
		const auto h = static_cast<common_t>(hi);

		return v >= l and v <= h;
	}

	// default epsilon
	template<std::floating_point T>
	constexpr T default_epsilon = T{1e-6};

	// zero_clamp
	template<std::floating_point T>
	[[nodiscard]] constexpr T clamp_zero(T value, T epsilon = default_epsilon<T>) noexcept
	{
		return std::abs(value) < epsilon ? T{0} : value;
	}

	// is_close_enough
	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough(T a, T b, T epsilon = default_epsilon<T>) noexcept
	{
		if (a == b)
			return true;

		if constexpr (is_debug_build)
		{
			if (std::isnan(a) or std::isnan(b) or std::isinf(a) or std::isinf(b))
				return false;
		}

		const T diff = std::abs(a - b);

		if (a == T{0.0} or b == T{0.0})
			return diff < epsilon;

		return diff <= epsilon * std::max(std::abs(a), std::abs(b));
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough_zero(const T& A, const T epsilon = default_epsilon<T>) noexcept
	{
		return is_close_enough(A, T{0}, epsilon);
	}

	template<std::floating_point T>
	[[nodiscard]] constexpr bool is_close_enough_one(const T& A, const T epsilon = default_epsilon<T>) noexcept
	{
		return is_close_enough(A, T{1}, epsilon);
	}

	// is_close_enough
	template<std::integral T>
	[[nodiscard]] constexpr bool is_close_enough(const T A, const T B, const T error = T{0})
	{
		return (A > B ? A - B : B - A) == error;
	}

	template<std::integral T>
	[[nodiscard]] constexpr bool is_close_enough_zero(const T& A)
	{
		return is_close_enough<T>(A, T{0});
	}

	//
	template<arithmetic T>
	[[nodiscard]] constexpr T to_radians(const T& v) noexcept
	{
		return static_cast<T>(v * deg_to_radians<T>);
	}

	template<arithmetic T>
	[[nodiscard]] constexpr T to_degrees(const T& v) noexcept
	{
		return static_cast<T>(v * rad_to_degrees<T>);
	}

	f32 operator""_rad(long double deg) { return to_radians<f32>(static_cast<f32>(deg)); }

	f32 operator""_deg(long double rad) { return to_degrees<f32>(static_cast<f32>(rad)); }

	f32 normalize_radians(f32 radian)
	{
		constexpr f32 pi    = std::numbers::pi_v<f32>;
		constexpr f32 twoPi = pi * 2.0f;

		radian = std::fmod(radian + pi, twoPi);
		if (radian < 0.0f)
			radian += twoPi;
		return radian - pi;
	}

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

		f32 dotf(const m128& lhs) { return _mm_cvtss_f32(dot(lhs)); };

		export f32 sqrt(f32 f) { return _mm_cvtss_f32(_mm_sqrt_ps(_mm_set_ps1(f))); }

		// ~2x faster than std::sin
		export f32 sin(f32 f)
		{
			// return _mm_cvtss_f32(_mm_sin_ps(_mm_set_ps1(f)));
			return _mm_cvtss_f32(_mm_set_ps1(std::sin(f)));
		}

		export f32 cos(f32 f)
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

	// template<std::unsigned_integral T>
	//[[nodiscard]] constexpr u64 index_from_2d(T x, T y, T width) noexcept
	//{
	//	return u64{y} * width + x;
	// }

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
