export module deckard.vec:vec2;


import std;
import deckard.debug;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.utils.hash;
import deckard.math.utils;

namespace deckard::math
{

	export template<arithmetic T>
	struct generic_vec2
	{
		using type     = T;
		using vec_type = generic_vec2<T>;


		T x{0};
		T y{0};

		constexpr generic_vec2() = default;

		constexpr generic_vec2(T s)
			: x(s)
			, y(s)
		{
		}

		constexpr generic_vec2(T sx, T sy)
			: x(sx)
			, y(sy)
		{
		}

		constexpr bool has_zero() const
		requires(std::is_integral_v<T>)
		{
			return x == T{0} or y == T{0};
		}

		constexpr bool is_zero() const
		requires(std::is_integral_v<T>)
		{
			return x == T{0} and y == T{0};
		}

		constexpr bool has_zero() const
		requires(std::is_floating_point_v<T>)
		{
			return math::is_close_enough_zero(x) or math::is_close_enough_zero(y);
		}

		constexpr bool is_zero() const
		requires(std::is_floating_point_v<T>)
		{
			return equals(zero());
		}

		constexpr vec_type& operator++()
		{
			*this += vec_type(1);
			return *this;
		}

		constexpr vec_type operator++(int)
		{
			vec_type tmp = *this;
			*this += vec_type(1);
			return tmp;
		}

		constexpr vec_type& operator--()
		{
			*this -= vec_type(1);
			return *this;
		}

		constexpr vec_type operator--(int)
		{
			vec_type tmp = *this;
			*this -= vec_type(1);
			return tmp;
		}

		// add
		constexpr void operator+=(const T scalar)
		{
			x += scalar;
			y += scalar;
		}

		constexpr void operator+=(const vec_type& other)
		{
			x += other.x;
			y += other.y;
		}

		constexpr vec_type operator+(const vec_type& other) const
		{
			vec_type result = *this;
			result += other;
			return result;
		}

		// sub
		constexpr void operator-=(const vec_type& other)
		{
			x -= other.x;
			y -= other.y;
		}

		constexpr void operator-=(const T scalar)
		{
			x -= scalar;
			y -= scalar;
		}

		constexpr vec_type operator-(const vec_type& other) const
		{
			vec_type result = *this;
			result -= other;
			return result;
		}

		// mul
		constexpr void operator*=(const T scalar)
		{
			x *= scalar;
			y *= scalar;
		}

		constexpr void operator*=(const vec_type& other)
		{
			x *= other.x;
			y *= other.y;
		}

		constexpr vec_type operator*(const vec_type& other) const
		{
			vec_type result = *this;
			result *= other;
			return result;
		}

		// div
		constexpr void operator/=(const vec_type& other)
		{
			if (other.has_zero())
				dbg::panic("divide by zero: {} / {}", *this, other);

			x /= other.x;
			y /= other.y;
		}

		constexpr vec_type operator/(const vec_type& other) const
		{
			if (other.has_zero())
				dbg::panic("divide by zero: {} / {}", *this, other);

			vec_type result = *this;
			result /= other;
			return result;
		}

		void operator/=(const T scalar)
		requires(std::is_floating_point_v<T>)
		{
			if (math::is_close_enough_zero(scalar))
				dbg::panic("divide by zero: {} / {}", *this, scalar);

			x /= scalar;
			y /= scalar;
		}

		void operator/=(const T scalar)
		requires(std::is_integral_v<T>)
		{
			if (scalar == T{0})
				dbg::panic("divide by zero: {} / {}", *this, scalar);

			x /= scalar;
			y /= scalar;
		}

		constexpr vec_type operator/(const T& scalar) const
		requires(std::is_integral_v<T>)
		{
			if (math::is_close_enough_zero(scalar))
				dbg::panic("divide by scalar zero: {} / {}", *this, scalar);

			vec_type result = *this;
			result /= scalar;
			return result;
		}

		// mod
		constexpr void operator%=(const vec_type& other)
		{
			x = mod(x, other.x);
			y = mod(y, other.y);
		}

		constexpr vec_type operator%(const vec_type& other) const
		{
			vec_type result = *this;
			result %= other;
			return result;
		}

		// unary
		constexpr vec_type& operator-()
		{
			*this *= vec_type(T{-1});

			return *this;
		}

		constexpr vec_type operator+() const { return *this; }

		void operator>>(T* v)
		{
			v[0] = x;
			v[1] = y;
		}

		void operator<<(T* v)
		{
			assert::check(v != nullptr, "input buffer null");

			x = v[0];
			y = v[1];
		}

		void operator<<=(T* v)
		{
			assert::check(v != nullptr, "input buffer null");
			x = v[0];
			y = v[1];
		}

		constexpr auto operator<=>(const vec_type& other) const = default;

		constexpr bool operator==(const vec_type& other) const { return equals(other); }

		constexpr bool equals(const vec_type& other, const T epsilon = 0) const { return is_close_enough(other, epsilon); }

		constexpr bool is_close_enough(const vec_type& lhs, T epsilon = 0) const
		requires(std::is_integral_v<T>)
		{
			return std::abs(x - lhs.x) == epsilon and std::abs(y - lhs.y) == epsilon;
		}

		constexpr bool equals(const vec_type& other, const T epsilon = T(0.0001)) const
		requires(std::floating_point<T>)
		{
			return is_close_enough(other, epsilon);
		}

		constexpr bool is_close_enough(const vec_type& lhs, T epsilon = T{0.0001}) const
		requires(std::floating_point<T>)
		{
			return math::is_close_enough(x, lhs.x, epsilon) and math::is_close_enough(y, lhs.y, epsilon);
		}

		[[nodiscard("Use the minimum value")]] constexpr vec_type min(const vec_type& other) const
		{
			return vec_type(std::min(x, other.x), std::min(y, other.y));
		}

		[[nodiscard("Use the maximum value")]] constexpr vec_type max(const vec_type& other) const
		{
			return vec_type(std::max(x, other.x), std::max(y, other.y));
		}

		[[nodiscard("Use the absolute value")]] constexpr vec_type abs() const { return vec_type(std::abs(x), std::abs(y)); }

		// manhattan distance
		[[nodiscard("Use the distance value")]] constexpr T distance(const vec_type& other) const
		requires(std::is_integral_v<T>)
		{
			T result{};

			result = abs_diff(x, other.x);
			result += abs_diff(y, other.y);

			return result;
		}

		[[nodiscard("Use the distance value")]] constexpr T distance(const vec_type& other) const
		requires(std::is_floating_point_v<T>)
		{
			T result{};

			T tmp = abs_diff(x, other.x);
			result += tmp * tmp;

			tmp = abs_diff(y, other.y);
			result += tmp * tmp;

			return std::sqrt(result);
		}

		[[nodiscard("Use the length value")]] constexpr T length() const
		requires(std::is_integral_v<T>)
		{
			T result{};

			result = abs_diff(x, 0);
			result += abs_diff(y, 0);

			return result;
		}

		[[nodiscard("Use the length value")]] constexpr T length() const
		requires(std::is_floating_point_v<T>)
		{
			T result{0};

			result += x * x;
			result += y * y;

			return std::sqrt(result);
		}

		constexpr void normalize()
		requires(std::is_floating_point_v<T>)
		{
			*this /= length();
		}

		[[nodiscard("Use the normalized value")]] constexpr vec_type normalized() const
		requires(std::is_floating_point_v<T>)
		{
			return *this / length();
		}

#if 0
				constexpr bool equals(const vec_type& other, const T epsilon = T(0.000001)) const
		requires(std::floating_point<T>)
		{
			return is_close_enough(other, epsilon);
		}

		constexpr bool is_close_enough(const vec_type& lhs, T epsilon = T{0.000001}) const
		requires(std::floating_point<T>)
		{
			return math::is_close_enough(x, lhs.x, epsilon) and math::is_close_enough(y, lhs.y, epsilon);
		}

#endif
		[[nodiscard("Use the projected vector")]] constexpr vec_type project(const vec_type& other) const
		requires(std::is_floating_point_v<T>)
		{
			if (other.has_zero())
			{
				dbg::trace("cannot project onto a zero vector: {} / {}", *this, other);
				return vec_type::zero();
			}

			auto dot_ab   = dot(other);
			auto b_length = other.length();

			T projection_scalar = dot_ab / (b_length * b_length);
			return other * projection_scalar;
		}

		[[nodiscard("Use the reflected vector")]] constexpr vec_type reflect(const vec_type& other) const
		requires(std::is_floating_point_v<T>)
		{
			const auto v(*this);
			return v - (other * v.dot(other) * 2);
		}

		[[nodiscard("Use the angle value")]] constexpr T angle(const vec_type& other) const
		requires(std::is_floating_point_v<T>)
		{
			if (has_zero() or other.has_zero())
			{
				dbg::trace("cannot take angle between zero vectors: {} / {}", *this, other);
				return T{0};
			}

			T cosTheta = dot(other) / (length() * other.length());

			return std::acos(cosTheta) * T(180.0) / std::numbers::pi_v<T>;
		}

		[[nodiscard("Use the clamped value")]] constexpr vec_type clamp(T low, T hi) const
		{

			if (low > hi)
				std::swap(low, hi);

			vec_type result{std::clamp(x, low, hi), std::clamp(y, low, hi)};
			return result;
		}

		[[nodiscard("Use the dot product value")]] constexpr type dot(const vec_type& other) const
		{
			vec_type result{*this * other};

			return result.x + result.y;
		}

		[[nodiscard("Use the cross product")]] constexpr type cross(const vec_type& other) const { return (x * other.y) - (y * other.x); }

		// static
		static inline vec_type zero() { return vec_type(T{0}); }

		static inline vec_type one() { return vec_type(T{1}); }
	};

	// Free functions

	export template<arithmetic T>
	[[nodiscard("Use the maximum value")]] constexpr generic_vec2<T> min(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.min(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the maximum vector")]] constexpr generic_vec2<T> max(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.max(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the absolute vector")]] constexpr generic_vec2<T> abs(const generic_vec2<T>& lhs)
	{
		return lhs.abs();
	}

	export template<arithmetic T>
	[[nodiscard("Use the distance value")]] constexpr T distance(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.distance(rhs);
	}

	export template<arithmetic T, arithmetic U, arithmetic R>
	[[nodiscard("Use the clamped vector")]] constexpr generic_vec2<T> clamp(const generic_vec2<T>& v, const U cmin, const R cmax)
	{
		return v.clamp(cmin, cmax);
	}

	export template<arithmetic T>
	[[nodiscard("Use the clamped value")]] constexpr T dot(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.dot(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the cross product vector")]] constexpr T cross(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the length value")]] constexpr T length(const generic_vec2<T>& rhs)
	{
		return rhs.length();
	}

	export template<std::floating_point T>
	constexpr void normalize(const generic_vec2<T>& rhs)
	{
		rhs.normalize();
	}

	export template<std::floating_point T>
	[[nodiscard("Use the normalized value")]] auto normalized(const generic_vec2<T>& rhs)
	{
		return rhs.normalized();
	}

	export template<std::floating_point T>
	[[nodiscard("Use the projected vector")]] constexpr generic_vec2<T> project(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.project(rhs);
	}

	export template<std::floating_point T>
	[[nodiscard("Use the angle value")]] constexpr auto angle(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.angle(rhs);
	}

	export template<std::floating_point T>
	[[nodiscard("Use the reflected vector")]] constexpr generic_vec2<T> reflect(const generic_vec2<T>& lhs, const generic_vec2<T>& rhs)
	{
		return lhs.reflect(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the summed vector value")]] constexpr T sum(const generic_vec2<T>& lhs)
	{
		return lhs.x + lhs.y;
	}

} // namespace deckard::math

export namespace std
{
	using namespace deckard;
	using namespace deckard::math;

	template<arithmetic T>
	struct hash<generic_vec2<T>>
	{
		size_t operator()(const generic_vec2<T>& value) const { return deckard::utils::hash_values(value.x, value.y); }
	};

	template<arithmetic T>
	struct formatter<generic_vec2<T>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const generic_vec2<T>& v, std::format_context& ctx) const
		requires(std::is_floating_point_v<T>)
		{
			return std::format_to(ctx.out(), "vec2({:.5f}, {:.5f})", v.x, v.y);
		}

		auto format(const generic_vec2<T>& v, std::format_context& ctx) const
		requires(std::is_integral_v<T>)
		{
			return std::format_to(ctx.out(), "vec2({}, {})", v.x, v.y);
		}
	};
} // namespace std
