export module deckard.vec:vec4;
import :vec3;

import std;
import deckard.debug;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.utils.hash;

namespace deckard::math
{

	export template<arithmetic T>
	struct alignas(alignof(T)) generic_vec4
	{
		using type     = T;
		using vec_type = generic_vec4<T>;


		T x{0};
		T y{0};
		T z{0};
		T w{0};


		constexpr generic_vec4() = default;

		constexpr generic_vec4(T s)
			: x(s)
			, y(s)
			, z(s)
			, w(s)
		{
		}

		constexpr generic_vec4(const generic_vec3<T>& v, T sw)
			: x(v.x)
			, y(v.y)
			, z(v.z)
			, w(sw)
		{
		}

		constexpr generic_vec4(T sx, T sy)
			: x(sx)
			, y(sy)
			, z(T{0})
			, w(T{0})
		{
		}

		constexpr generic_vec4(T sx, T sy, T sz)
			: x(sx)
			, y(sy)
			, z(sz)
			, w(T{0})
		{
		}

		constexpr generic_vec4(T sx, T sy, T sz, T sw)
			: x(sx)
			, y(sy)
			, z(sz)
			, w(sw)
		{
		}

		constexpr bool has_zero() const
		requires(std::is_integral_v<T>)
		{
			return x == T{0} or y == T{0} or z == T{0} or w == T{0};
		}

		constexpr bool is_zero() const
		requires(std::is_integral_v<T>)
		{
			return x == T{0} and y == T{0} and z == T{0} and w == T{0};
		}

		constexpr bool has_zero() const
		requires(std::is_floating_point_v<T>)
		{
			return math::is_close_enough_zero(x) or math::is_close_enough_zero(y) or math::is_close_enough_zero(z) or
				   math::is_close_enough_zero(w);
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
			v[2] = z;
			v[3] = w;
		}

		void operator<<(T* v)
		{
			assert::check(v != nullptr, "input buffer null");

			x = v[0];
			y = v[1];
			z = v[2];
			w = v[3];
		}

		void operator<<=(T* v)
		{
			assert::check(v != nullptr, "input buffer null");
			x = v[0];
			y = v[1];
			z = v[2];
			w = v[3];
		}

		constexpr operator generic_vec3<T>() const { return generic_vec3<T>(x, y, z); }

		constexpr auto operator<=>(const vec_type& other) const = default;

		constexpr bool operator==(const vec_type& other) const { return equals(other); }

		constexpr bool equals(const vec_type& other, const T epsilon = 0) const
		requires(std::is_integral_v<T>)
		{
			return is_close_enough(other, epsilon);
		}

		constexpr bool is_close_enough(const vec_type& lhs, T epsilon = 0) const
		requires(std::is_integral_v<T>)
		{
			return std::abs(x - lhs.x) == epsilon and std::abs(y - lhs.y) == epsilon and std::abs(z - lhs.z) == epsilon and
				   std::abs(w - lhs.w) == epsilon;
		}

		constexpr bool equals(const vec_type& other, const T epsilon = T{1e-5}) const
		requires(std::is_floating_point_v<T>)
		{
			return is_close_enough(other, epsilon);
		}

		constexpr bool is_close_enough(const vec_type& lhs, T epsilon = T{1e-5}) const
		requires(std::is_floating_point_v<T>)
		{
			return math::is_close_enough(x, lhs.x, epsilon) and math::is_close_enough(y, lhs.y, epsilon) and
				   math::is_close_enough(z, lhs.z, epsilon) and math::is_close_enough(w, lhs.w, epsilon);
		}

		[[nodiscard("Use the minimum value")]] constexpr vec_type min(const vec_type& other) const
		{
			return vec_type(std::min(x, other.x), std::min(y, other.y), std::min(z, other.z), std::min(w, other.w));
		}

		[[nodiscard("Use the maximum value")]] constexpr vec_type max(const vec_type& other) const
		{
			return vec_type(std::max(x, other.x), std::max(y, other.y), std::max(z, other.z), std::max(w, other.w));
		}

		[[nodiscard("Use the absolute value")]] constexpr vec_type abs() const
		{
			return vec_type(std::abs(x), std::abs(y), std::abs(z), std::abs(w));
		}

		// manhattan distance
		[[nodiscard("Use the distance value")]] constexpr T distance(const vec_type& other) const
		requires(std::is_integral_v<T>)
		{
			T result{};

			result = abs_diff(x, other.x);
			result += abs_diff(y, other.y);
			result += abs_diff(z, other.z);
			result += abs_diff(w, other.w);

			return result;
		}

		[[nodiscard("Use the distance value")]] constexpr T distance(const vec_type& other) const
		requires(std::is_floating_point_v<T>)
		{
			T result{};

			T tmp = as<T>(abs_diff(x, other.x));
			result += tmp * tmp;

			tmp = as<T>(abs_diff(y, other.y));
			result += tmp * tmp;

			tmp = as<T>(abs_diff(z, other.z));
			result += tmp * tmp;

			tmp = as<T>(abs_diff(w, other.w));
			result += tmp * tmp;

			return std::sqrt(result);
		}

		template<std::integral U = T>
		[[nodiscard("Use the length value")]] constexpr U length() const
		requires(std::is_integral_v<T>)
		{
			U result{};

			result = abs_diff(x, 0);
			result += abs_diff(y, 0);
			result += abs_diff(z, 0);
			result += abs_diff(w, 0);

			return result;
		}

		[[nodiscard("Use the length value")]] constexpr T length() const
		requires(std::is_floating_point_v<T>)
		{
			T result{0};

			result += x * x;
			result += y * y;
			result += z * z;
			result += w * w;

			return std::sqrt(result);
		}

		constexpr void normalize()
		requires(std::is_floating_point_v<T>)
		{
			*this /= length();
		}

		[[nodiscard("Use the normalized vector")]] constexpr vec_type normalized() const
		requires(std::is_floating_point_v<T>)
		{
			return *this / length();
		}

		template<std::floating_point T = f32>
		[[nodiscard("Use the projected vector")]] constexpr vec_type project(const vec_type& other) const
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
				return T{0.0};
			}

			T cosTheta = dot(other) / (length() * other.length());

			return std::acos(cosTheta) * T(180.0) / std::numbers::pi_v<T>;
		}

		template<arithmetic U, arithmetic R>
		[[nodiscard("Use the clamped value")]] constexpr vec_type clamp(U low, R hi) const
		{

			if (low > hi)
				std::swap(low, hi);

			vec_type result{std::clamp<T>(x, low, hi), std::clamp<T>(y, low, hi), std::clamp<T>(z, low, hi), std::clamp<T>(w, low, hi)};
			return result;
		}

		[[nodiscard("Use the dot product value")]] constexpr type dot(const vec_type& other) const
		{
			vec_type result{*this * other};

			return result.x + result.y + result.z + result.w;
		}

		[[nodiscard("Use the cross product")]] constexpr generic_vec3<T> cross(const vec_type& other) const
		{
			generic_vec3<T> result;
			result.x = y * other.z - z * other.y;
			result.y = z * other.x - x * other.z;
			result.z = x * other.y - y * other.x;
			return result;
		}

		// static
		static inline vec_type zero() { return vec_type(T{0}); }

		static inline vec_type one() { return vec_type(T{1}); }
	};

	// Free functions

	// add
	export template<arithmetic T, arithmetic U>
	constexpr void operator+=(generic_vec4<T>& lhs, const U& scalar)
	{
		lhs += generic_vec4<T>(as<T>(scalar));
	}

	export template<arithmetic T, arithmetic U>
	constexpr generic_vec4<T> operator+(const generic_vec4<T>& lhs, const U& scalar)
	{
		generic_vec4<T> result(lhs);
		result += generic_vec4<T>(as<T>(scalar));
		return result;
	}

	export template<arithmetic T, arithmetic U>
	constexpr generic_vec4<T> operator+(const generic_vec4<T>& lhs, const generic_vec4<U>& rhs)
	{
		generic_vec4<T> result(lhs);
		result += rhs;
		return result;
	}

	export template<arithmetic T, arithmetic U>
	constexpr void operator+=(generic_vec4<T>& lhs, const generic_vec4<U>& rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
		lhs.w += rhs.w;
	}

	// sub
	export template<arithmetic T, arithmetic U>
	constexpr void operator-=(generic_vec4<T>& lhs, const U& scalar)
	{
		lhs -= generic_vec4<T>(as<T>(scalar));
	}

	export template<arithmetic T, arithmetic U>
	constexpr generic_vec4<T> operator-(const generic_vec4<T>& lhs, const U& scalar)
	{
		generic_vec4<T> result(lhs);
		result -= generic_vec4<T>(as<T>(scalar));
		return result;
	}

	export template<arithmetic T, arithmetic U>
	constexpr generic_vec4<T> operator-(const generic_vec4<T>& lhs, const generic_vec4<U>& rhs)
	{
		generic_vec4<T> result(lhs);
		result -= rhs;
		return result;
	}

	export template<arithmetic T, arithmetic U>
	constexpr void operator-=(generic_vec4<T>& lhs, const generic_vec4<U>& rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
		lhs.w -= rhs.w;
	}

	// mul
	export template<arithmetic T, arithmetic U>
	constexpr void operator*=(generic_vec4<T>& lhs, const U& scalar)
	{
		lhs *= generic_vec4<T>(scalar);
	}

	export template<arithmetic T, arithmetic U>
	constexpr generic_vec4<T> operator*(const generic_vec4<T>& lhs, const U& scalar)
	{
		generic_vec4<T> result(lhs);
		result *= generic_vec4<T>(as<T>(scalar));
		return result;
	}

	export template<arithmetic T>
	constexpr generic_vec4<T> operator*(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		generic_vec4<T> result(lhs);
		result *= rhs;
		return result;
	}

	export template<arithmetic T>
	constexpr void operator*=(generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		lhs.x *= rhs.x;
		lhs.y *= rhs.y;
		lhs.z *= rhs.z;
		lhs.w *= rhs.w;
	}

	// div
	export template<arithmetic T, arithmetic U>
	constexpr generic_vec4<T> operator/(const generic_vec4<T>& lhs, const U scalar)
	{
		if (math::is_close_enough_zero(as<T>(scalar)))
			dbg::panic("divide by zero: {} / {}", lhs, scalar);

		generic_vec4<T> result(lhs);
		result /= generic_vec4<T>(as<T>(scalar));
		return result;
	}

	export template<arithmetic T>
	constexpr generic_vec4<T> operator/(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		generic_vec4<T> result(lhs);
		result /= rhs;
		return result;
	}

	export template<arithmetic T>
	constexpr void operator/=(generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		if (rhs.has_zero())
			dbg::panic("divide by zero: {} / {}", lhs, rhs);

		lhs.x /= rhs.x;
		lhs.y /= rhs.y;
		lhs.z /= rhs.z;
		lhs.w /= rhs.w;
	}

	export template<arithmetic T, arithmetic U>
	constexpr void operator/=(generic_vec4<T>& lhs, const U& scalar)
	{
		lhs /= generic_vec4<T>(scalar);
	}

	// mod
	export template<arithmetic T, arithmetic U>
	constexpr void operator%=(generic_vec4<T>& lhs, const U& scalar)
	{
		lhs %= generic_vec4<T>(scalar);
	}

	export template<arithmetic T>
	constexpr void operator%=(generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	requires(std::is_integral_v<T>)
	{
		lhs.x = mod(lhs.x, rhs.x);
		lhs.y = mod(lhs.y, rhs.y);
		lhs.z = mod(lhs.y, rhs.z);
		lhs.w = mod(lhs.y, rhs.w);
	}

	export template<arithmetic T>
	constexpr void operator%=(generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	requires(std::is_floating_point_v<T>)
	{
		lhs.x = std::fmodf(lhs.x, rhs.x);
		lhs.y = std::fmodf(lhs.y, rhs.y);
		lhs.z = std::fmodf(lhs.y, rhs.z);
		lhs.w = std::fmodf(lhs.y, rhs.w);
	}

	export template<arithmetic T>
	constexpr generic_vec4<T> operator%=(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		generic_vec4<T> result(lhs);
		result %= rhs;
		return result;
	}

	export template<arithmetic T>
	constexpr generic_vec4<T> operator%(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		generic_vec4<T> result(lhs);
		result %= rhs;
		return result;
	}

	export template<arithmetic T>
	[[nodiscard("Use the maximum value")]] constexpr generic_vec4<T> min(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.min(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the maximum vector")]] constexpr generic_vec4<T> max(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.max(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the absolute vector")]] constexpr generic_vec4<T> abs(const generic_vec4<T>& lhs)
	{
		return lhs.abs();
	}

	export template<arithmetic T>
	[[nodiscard("Use the distance value")]] constexpr T distance(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.distance(rhs);
	}

	export template<arithmetic T, arithmetic U, arithmetic R>
	[[nodiscard("Use the clamped vector")]] constexpr generic_vec4<T> clamp(const generic_vec4<T>& v, const U cmin, const R cmax)
	{
		return v.clamp(cmin, cmax);
	}

	export template<arithmetic T>
	[[nodiscard("Use the clamped value")]] constexpr T dot(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.dot(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the cross product vector")]] constexpr generic_vec3<T> cross(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the length value")]] constexpr T length(const generic_vec4<T>& rhs)
	{
		return rhs.length();
	}

	export template<std::floating_point T>
	[[nodiscard("Use the projected vector")]] constexpr generic_vec4<T> project(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.project(rhs);
	}

	export template<std::floating_point T>
	[[nodiscard("Use the angle value")]] constexpr auto angle(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.angle(rhs);
	}

	export template<std::floating_point T>
	[[nodiscard("Use the reflected vector")]] constexpr generic_vec4<T> reflect(const generic_vec4<T>& lhs, const generic_vec4<T>& rhs)
	{
		return lhs.reflect(rhs);
	}

	export template<arithmetic T>
	[[nodiscard("Use the summed vector value")]] constexpr T sum(const generic_vec4<T>& lhs)
	{
		return lhs.x + lhs.y + lhs.z + lhs.w;
	}


} // namespace deckard::math

export namespace std
{
	using namespace deckard;
	using namespace deckard::math;

	template<arithmetic T>
	struct hash<generic_vec4<T>>
	{
		size_t operator()(const generic_vec4<T>& value) const { return deckard::utils::hash_values(value.x, value.y, value.z, value.w); }
	};

	template<arithmetic T>
	struct formatter<generic_vec4<T>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const generic_vec4<T>& v, std::format_context& ctx) const
		requires(std::is_floating_point_v<T>)
		{
			return std::format_to(ctx.out(), "vec4({:.5f}, {:.5f}, {:.5f}, {:.5f})", v.x, v.y, v.z, v.w);
		}

		auto format(const generic_vec4<T>& v, std::format_context& ctx) const
		requires(std::is_integral_v<T>)
		{
			return std::format_to(ctx.out(), "vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
		}
	};
} // namespace std
