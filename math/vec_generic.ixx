
export module deckard.math:vec.generic;
import :utils;

import std;
import deckard.debug;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.utils.hash;

namespace deckard::math
{

	export template<arithmetic T, size_t N>
	requires(N > 1)
	struct vec_n
	{
		using type     = T;
		using vec_type = vec_n<T, N>;

		constexpr vec_n() = default;

		constexpr vec_n(T x, T y)
		requires(N >= 2)
		{
			m_data[0] = x;
			m_data[1] = y;
		}

		constexpr vec_n(T x, T y, T z)
		requires(N >= 3)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
		}

		constexpr vec_n(T x, T y, T z, T w)
		requires(N == 4)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
			m_data[3] = w;
		}

		template<typename U, typename S>
		constexpr vec_n(const vec_n<U, 2>& v)
		requires(N == 2)
		{
			m_data[0] = v[0];
			m_data[1] = v[1];
		}

		template<typename U, typename S>
		constexpr vec_n(const vec_n<U, 3>& v, S scalar)
		requires(N == 4)
		{
			m_data[0] = v[0];
			m_data[1] = v[1];
			m_data[2] = v[2];
			m_data[3] = scalar;
		}

		template<typename U, typename S>
		constexpr vec_n(const vec_n<U, 2>& v, S scalar)
		requires(N == 3)
		{
			m_data[0] = v[0];
			m_data[1] = v[1];
			m_data[2] = scalar;
		}

		constexpr vec_n(const std::initializer_list<T> list, const std::source_location& loc = std::source_location::current())
		{

			if (list.size() == 1)
			{
				m_data.fill(*list.begin());
				return;
			}
			dbg::if_true(
			  list.size() > N,
			  "{}({}): Warning: initializer list (length: {}) is longer than the container (length: {}).",
			  loc.file_name(),
			  loc.line(),
			  list.size(),
			  N);

			std::copy_n(list.begin(), std::min(N, list.size()), m_data.begin());
		}

		constexpr operator vec_type() const
		{
			vec_type result{*this};
			return result;
		}

		constexpr T& at(size_t index) { return m_data[index]; }

		constexpr const T& at(size_t index) const { return m_data[index]; }

		constexpr T& operator[](size_t index) { return m_data[index]; }

		constexpr const T& operator[](size_t index) const
		{
			assert::check(index < N, "indexing out-of-bounds");
			return m_data[index];
		}

		constexpr bool has_zero() const
		{
			for (int i = 0; i < N; ++i)
			{
				if (m_data[i] == T{0})
					return true;
			}
			return false;
		};

		constexpr bool is_zero() const
		{
			for (int i = 0; i < N; ++i)
			{
				if (m_data[i] != T{0})
					return false;
			}
			return true;
		}

		bool is_nan() const
		{
			for (int i = 0; i < N; ++i)
			{
				if (std::isnan(m_data[i]) == false)
					return false;
			}
			return true;
		}

		constexpr bool has_nan() const
		{
			for (int i = 0; i < N; ++i)
			{
				if (std::isnan(m_data[i]))
					return true;
			}
			return false;
		};

		constexpr bool has_inf() const
		{
			for (int i = 0; i < N; ++i)
			{
				if (std::isinf(m_data[i]))
					return true;
			}
			return false;
		};

		constexpr bool is_inf() const
		{
			for (int i = 0; i < N; ++i)
			{
				if (std::isinf(m_data[i]) == false)
					return false;
			}
			return true;
		};

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

		constexpr void operator+=(const vec_type& other)
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] += other[i];
			}
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
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] -= other[i];
			}
		}

		void operator+=(const float scalar)
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] += scalar;
			}
		}

		void operator-=(const float scalar)
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] -= scalar;
			}
		}

		void operator*=(const float scalar)
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] *= scalar;
			}
		}

		void operator/=(const float scalar)
		{
			if (math::is_close_enough(scalar, 0.0f))
				dbg::panic("divide by zero: {} / {}", *this, scalar);

			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] /= scalar;
			}
		}

		constexpr vec_type operator-(const vec_type& other) const
		{
			vec_type result = *this;
			result -= other;
			return result;
		}

		// mul
		constexpr void operator*=(const vec_type& other)
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] *= other[i];
			}
		}

		constexpr vec_type operator*(const vec_type& other) const
		{
			vec_type result = *this;
			result *= other;
			return result;
		}

		constexpr void operator/=(const vec_type& other)
		{
			if (other.has_zero())
				dbg::panic("divide by zero: {} / {}", *this, other);

			for (size_t i = 0; i < N; ++i)
				m_data[i] /= other[i];
		}

		// div
		constexpr vec_type operator/(const vec_type& other) const
		{
			if (other.has_zero())
				dbg::panic("divide by zero: {} / {}", *this, other);

			vec_type result = *this;
			for (size_t i = 0; i < N; ++i)
				result[i] /= other[i];

			return result;
		}

		// div
		template<typename U = T>
		constexpr vec_type operator/(const U& scalar) const
		{
			if (scalar == U{0})
				dbg::panic("divide by scalar zero: {} / {}", *this, scalar);

			vec_type result = *this;

			for (size_t i = 0; i < N; ++i)
				result[i] /= as<T>(scalar);

			return result;
		}

		// mod
		constexpr void operator%=(const vec_type& other)
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] = mod(m_data[i], other[i]);
			}
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
			*this *= vec_type(T{-1.0});
			return *this;
		}

		constexpr vec_type operator+() const { return *this; }

		void operator>>(T* v)
		{
			for (size_t i = 0; i < N; ++i)
				v[i] = m_data[i];
		}

		void operator<<(T* v)
		{
			assert::check(v != nullptr, "input buffer null");

			for (size_t i = 0; i < N; ++i)
				m_data[i] = v[i];
		}

		void operator<<=(T* v)
		{
			assert::check(v != nullptr, "input buffer null");
			for (size_t i = 0; i < N; ++i)
				m_data[i] = v[i];
		}

		constexpr auto operator<=>(const vec_type& other) const = default;

		constexpr bool operator==(const vec_type& other) const { return equals(other, T{}); }

		constexpr bool equals(const vec_type& other, const T epsilon = T(0.000001)) const
		requires(std::floating_point<T>)
		{
			return is_close_enough(other, epsilon);
		}

		constexpr bool equals(const vec_type& other, const T epsilon = 0) const
		requires(std::integral<T>)
		{
			return is_close_enough(other, epsilon);
		}

		constexpr bool is_close_enough(const vec_type& lhs, float epsilon = 0.000001f) const
		requires(std::floating_point<T>)
		{
			for (size_t i = 0; i < N; ++i)
			{
				if (not math::is_close_enough(m_data[i], lhs[i], epsilon))
					return false;
			}
			return true;
		}

		constexpr bool is_close_enough(const vec_type& lhs, T epsilon = 0) const
		requires(std::integral<T>)
		{
			for (size_t i = 0; i < N; ++i)
			{
				if (not math::is_close_enough(m_data[i], lhs[i], epsilon))
					return false;
			}
			return true;
		}

		constexpr vec_type diff(const vec_type& other) const
		{
			vec_type result = *this;

			for (size_t i = 0; i < N; ++i)
				result[i] -= other[i];

			return result;
		}

		// other functions

		// min
		[[nodiscard("Use the minimum value")]] constexpr vec_type min(const vec_type& other) const
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::min(m_data[i], other[i]);

			return result;
		}

		// max
		[[nodiscard("Use the maximum value")]] constexpr vec_type max(const vec_type& other) const
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::max(m_data[i], other[i]);

			return result;
		}

		// abs
		[[nodiscard("Use the absolute value")]] constexpr vec_type abs() const
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::abs(m_data[i]);

			return result;
		}

		// magnitude/length
		template<typename U = T>
		[[nodiscard("Use the length value")]] constexpr U length() const
		{
			U result{0};
			for (size_t i = 0; i < N; ++i)
				result += m_data[i] * m_data[i];

			return sse::sqrt(result);
		}

		// normalize
		constexpr void normalize()
		requires(N >= 2)
		{
			const auto len = length();

			for (size_t i = 0; i < N; ++i)
				m_data[i] /= len;
		}

		// normalized
		[[nodiscard("Use the normalized value")]] constexpr vec_type normalized() const
		requires(N >= 2)
		{
			vec_type   result{0};
			const auto len = length();

			for (size_t i = 0; i < N; ++i)
				result[i] = m_data[i] / len;

			return result;
		}

		// manhattan distance
		template<typename U = T>
		[[nodiscard("Use the distance value")]] constexpr U distance(const vec_type& other) const
		{
			U result{};
			for (size_t i = 0; i < N; ++i)
			{
				U tmp = as<U>(std::abs(m_data[i] - other[i]));
				result += tmp * tmp;
			}
			return std::sqrt(result);
		}

		// clamp
		template<typename U = T>
		[[nodiscard("Use the clamped value")]] constexpr vec_type clamp(const U cmin, const U cmax) const
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::clamp(m_data[i], cmin, cmax);

			return result;
		}

		// 2d cross product
		template<typename U = T>
		[[nodiscard("Use the cross product")]] constexpr U cross(const vec_type& other) const
		requires(N == 2)
		{
			return m_data[0] * other[1] - m_data[1] * other[0];
		}

		// 3d+ cross product
		template<typename T, size_t N>
		requires(N >= 3)
		[[nodiscard("Use the cross product")]] constexpr auto cross(const vec_n<T, N>& other) const
		{
			vec_n<T, 3> result;

			result[0] = m_data[1] * other[2] - m_data[2] * other[1];
			result[1] = m_data[2] * other[0] - m_data[0] * other[2];
			result[2] = m_data[0] * other[1] - m_data[1] * other[0];

			return result;
		}

		template<typename U = T>
		[[nodiscard("Use the dot product value")]] constexpr U dot(const vec_type& other) const
		{
			U result{};
			for (size_t i = 0; i < N; ++i)
				result += as<U>(m_data[i] * other[i]);
			return result;
		}

		// 2d/3d projection
		[[nodiscard("Use the projected vector")]] constexpr vec_type project(const vec_type& other) const
		requires(N == 2 or N == 3)
		{
			if (other.has_zero())
			{
				dbg::trace("cannot project onto a zero vector: {} / {}", *this, other);
				return vec_type(std::numeric_limits<T>::quiet_NaN());
			}

			auto dot_ab   = dot(other);
			auto b_length = other.length();

			T projection_scalar = dot_ab / (b_length * b_length);
			return other * projection_scalar;
		}

		// 2d/3d angle
		[[nodiscard("Use the angle value")]] constexpr T angle(const vec_type& other) const
		requires(N == 2 or N == 3)
		{
			if (has_zero() or other.has_zero())
			{
				dbg::trace("cannot take angle between zero vectors: {} / {}", *this, other);
				return std::numeric_limits<T>::quiet_NaN();
			}

			T cosTheta = dot(other) / (length() * other.length());


			return std::acos(cosTheta) * T{180.0} / std::numbers::pi_v<T>;
		}

		// 3d-rotate
		[[nodiscard("Use the rotated vector")]] constexpr vec_type rotate(const vec_type& axis, const T rad) const
		requires(N == 3)
		{
			const vec_type axis_norm = axis.normalized();
			const vec_type v         = *this;

			T cosTheta         = std::cos(rad);
			T sinTheta         = std::sin(rad);
			T oneMinusCosTheta = T{1.0} - cosTheta;

			return (v * cosTheta) + (v.cross(axis) * sinTheta) + (axis * v.dot(axis)) * oneMinusCosTheta;
		}

		[[nodiscard("Use the reflected vector")]] constexpr vec_type reflect(const vec_type& other) const
		{
			//
			const vec_type v(*this);
			return v - (other * v.dot(other) * 2);
		}

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] constexpr vec_type safe_divide(const vec_type& other) const
		requires(std::floating_point<T>)
		{
			if (other.has_zero())
				return vec_type(std::numeric_limits<T>::infinity());

			vec_type result = *this;
			for (size_t i = 0; i < N; ++i)
				result[i] /= other[i];

			return result;
		}

		template<typename U = T>
		[[nodiscard("Use the divide scalar")]] constexpr vec_type safe_divide(const U scalar) const
		requires(std::floating_point<U>)
		{
			if (scalar == U{})
				return vec_type(std::numeric_limits<U>::infinity());

			vec_type result = *this;

			for (size_t i = 0; i < N; ++i)
				result[i] /= as<U>(scalar);

			return result;
		}

		std::array<T, N> m_data{T{}};


		inline static T nan_value = std::numeric_limits<T>::quiet_NaN();
		inline static T inf_value = std::numeric_limits<T>::infinity();

		static inline vec_type nan() { return vec_type(nan_value); }

		static inline vec_type inf() { return vec_type(inf_value); }

		static inline vec_type zero() { return vec_type(T{0}); }
	};

	// Free functions
	export template<typename T, size_t N>
	[[nodiscard("Use the maximum value")]] constexpr vec_n<T, N> min(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.min(rhs);
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the maximum vector")]] constexpr vec_n<T, N> max(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.max(rhs);
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the absolute vector")]] constexpr vec_n<T, N> abs(const vec_n<T, N>& lhs)
	{
		return lhs.abs();
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the distance value")]] constexpr T distance(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.distance<T>(rhs);
	}

	export template<typename T, typename U = T, size_t N>
	[[nodiscard("Use the clamped vector")]] constexpr vec_n<T, N> clamp(const vec_n<T, N>& v, const U cmin, const U cmax)
	{
		return v.clamp(cmin, cmax);
	}

	export template<typename T, size_t N>
	requires(N == 2)
	[[nodiscard("Use the cross product vector")]] constexpr auto cross(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<typename T, size_t N, size_t M>
	requires(N >= 3 and M >= 3)
	[[nodiscard("Use the clamped vector")]] constexpr vec_n<T, 3> cross(const vec_n<T, N>& lhs, const vec_n<T, M>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<typename T, size_t N>
	requires(N >= 2)
	[[nodiscard("Use the clamped value")]] constexpr T dot(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.dot(rhs);
	}

	export template<typename T, size_t N>
	requires(N >= 2)
	[[nodiscard("Use the length value")]] constexpr T length(const vec_n<T, N>& rhs)
	{
		return rhs.length();
	}

	export template<typename T, size_t N>
	requires(N >= 2)
	constexpr void normalize(const vec_n<T, N>& rhs)
	{
		rhs.normalize();
	}

	export template<typename T, size_t N>
	requires(N >= 2)
	[[nodiscard("Use the normalized value")]] constexpr auto normalized(const vec_n<T, N>& rhs)
	{
		return rhs.normalized();
	}

	export template<typename T, size_t N>
	requires(N == 3)
	[[nodiscard("Use the projected vector")]] constexpr vec_n<T, N> cross(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<typename T, size_t N>
	requires(N == 2 or N == 3)
	[[nodiscard("Use the projected vector")]] constexpr vec_n<T, N> project(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.project(rhs);
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the reflected vector")]] constexpr vec_n<T, N> reflect(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.reflect(rhs);
	}

	export template<typename T, size_t N>
	requires(N == 2 or N == 3)
	[[nodiscard("Use the angle value")]] constexpr T angle(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.angle(rhs);
	}

	export template<typename T, size_t N>
	requires(N == 2 or N == 3)
	[[nodiscard("Use the rotated vector")]] constexpr vec_n<T, N> rotate(const vec_n<T, N>& v, const vec_n<T, N>& axis, const T angle)
	{
		return v.rotate(axis, angle);
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the divided value")]] constexpr vec_n<T, N> safe_divide(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.safe_divide(rhs);
	}

	export template<typename T, typename U = T, size_t N>
	[[nodiscard("Use the divided value")]] constexpr vec_n<T, N> safe_divide(const vec_n<T, N>& lhs, const U scalar)
	{
		return lhs.safe_divide(scalar);
	}


} // namespace deckard::math

using namespace deckard;
export using uvec2 = deckard::math::vec_n<u64, 2>;
export using uvec3 = deckard::math::vec_n<u64, 3>;
export using uvec4 = deckard::math::vec_n<u64, 4>;

export using ivec2 = deckard::math::vec_n<i64, 2>;
export using ivec3 = deckard::math::vec_n<i64, 3>;
export using ivec4 = deckard::math::vec_n<i64, 4>;

namespace deckard::math
{

	export template<typename T>
	struct grid_order
	{
		inline bool operator()(const T& v1, const T& v2) const { return (v1[1] < v2[1]) || (v1[1] == v2[1] && v1[0] < v2[0]); }
	};

	export template<typename T>
	struct grid_order_reverse
	{
		inline bool operator()(const T& v1, const T& v2) const
		{
			//
			return (v1[1] > v2[1]) || (v1[1] == v2[1] && v1[0] > v2[0]);
		}
	};

} // namespace deckard::math

static_assert(sizeof(deckard::math::vec_n<f32, 4>) == 16);
static_assert(sizeof(deckard::math::vec_n<f32, 3>) == 12);
static_assert(sizeof(deckard::math::vec_n<f32, 2>) == 8);

namespace std
{
	using namespace deckard::math;

	template<arithmetic T>
	struct hash<vec_n<T, 2>>
	{
		size_t operator()(const vec_n<T, 2>& value) const { return deckard::utils::hash_values(value[0], value[1]); }
	};

	template<arithmetic T>
	struct hash<vec_n<T, 3>>
	{
		size_t operator()(const vec_n<T, 3>& value) const { return deckard::utils::hash_values(value[0], value[1], value[2]); }
	};

	template<arithmetic T>
	struct hash<vec_n<T, 4>>
	{
		size_t operator()(const vec_n<T, 4>& value) const { return deckard::utils::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<arithmetic T, size_t N>
	struct hash<vec_n<T, N>>
	{
		size_t operator()(const vec_n<T, N>& value) const { return deckard::utils::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<arithmetic T, size_t N>
	struct formatter<vec_n<T, N>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const vec_n<T, N>& vec, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "vec{}(", N);

			if constexpr (std::is_integral_v<T>)
			{
				for (int i = 0; i < N; ++i)
					std::format_to(ctx.out(), "{}{}", vec[i], i < N - 1 ? ", " : "");
			}
			else
			{
				for (int i = 0; i < N; ++i)
					std::format_to(ctx.out(), "{:.3f}{}", vec[i], i < N - 1 ? ", " : "");
			}

			return std::format_to(ctx.out(), ")");
		}
	};
} // namespace std
