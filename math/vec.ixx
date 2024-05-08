export module deckard.math.vec;

import std;
import deckard.as;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.math.utility;
import deckard.helpers;

namespace deckard::math
{

	template<typename T, size_t N>
	requires(std::integral<T> or std::floating_point<T>) and (N > 1)
	struct vec_n
	{
		using type     = T;
		using vec_type = vec_n<T, N>;

		vec_n() = default;

		vec_n(T scalar) noexcept
		requires(N >= 2)
		{
			m_data.fill(scalar);
		}

		vec_n(T x, T y) noexcept
		requires(N >= 2)
		{
			m_data[0] = x;
			m_data[1] = y;
		}

		vec_n(T x, T y, T z) noexcept
		requires(N >= 3)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
		}

		vec_n(T x, T y, T z, T w) noexcept
		requires(N == 4)
		{
			m_data[0] = x;
			m_data[1] = y;
			m_data[2] = z;
			m_data[3] = w;
		}

		vec_n(const std::initializer_list<T> list, const std::source_location& loc = std::source_location::current()) noexcept
		{

			if (list.size() == 1)
			{
				m_data.fill(*list.begin());
				return;
			}
			dbg::if_true(list.size() > N,
						 "{}({}): Warning: initializer list (length: {}) is longer than the container (length: {}).",
						 loc.file_name(),
						 loc.line(),
						 list.size(),
						 N);

			std::copy_n(list.begin(), std::min(N, list.size()), m_data.begin());
		}

		constexpr operator vec_type() const noexcept
		{
			vec_type result{*this};
			return result;
		}

		constexpr T& at(size_t index) noexcept { return m_data[index]; }

		constexpr const T& at(size_t index) const noexcept { return m_data[index]; }

		constexpr T& operator[](size_t index) noexcept { return m_data[index]; }

		constexpr const T& operator[](size_t index) const noexcept { return m_data[index]; }

		bool has_zero() const noexcept
		{

			for (int i = 0; i < N; ++i)
				if (m_data[i] == T{0})
					return true;
			return false;
		};

		constexpr vec_type& operator+=(const vec_type& other) noexcept
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] += other[i];
			}
			return *this;
		}

		constexpr vec_type operator+(const vec_type& other) const noexcept
		{
			vec_type result = *this;
			result += other;
			return result;
		}

		// sub
		constexpr vec_type& operator-=(const vec_type& other) noexcept
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] -= other[i];
			}
			return *this;
		}

		constexpr vec_type operator-(const vec_type& other) const noexcept
		{
			vec_type result = *this;
			result -= other;
			return result;
		}

		// mul
		constexpr vec_type& operator*=(const vec_type& other) noexcept
		{
			for (size_t i = 0; i < N; ++i)
			{
				m_data[i] *= other[i];
			}
			return *this;
		}

		constexpr vec_type operator*(const vec_type& other) const noexcept
		{
			vec_type result = *this;
			result *= other;
			return result;
		}

		constexpr vec_type& operator/=(const vec_type& other) noexcept
		{
			if (other.has_zero())
			{
				dbg::panic("divide by zero: {} / {}", *this, other);
			}

			for (size_t i = 0; i < N; ++i)
				m_data[i] /= other[i];

			return *this;
		}

		// div
		constexpr vec_type operator/(const vec_type& other) const noexcept
		{
			if (other.has_zero())
			{
				dbg::panic("divide by zero: {} / {}", *this, other);
			}

			vec_type result = *this;
			for (size_t i = 0; i < N; ++i)
				result[i] /= other[i];

			return result;
		}

		// div
		template<typename U>
		requires(std::integral<U> or std::floating_point<U>)
		constexpr vec_type operator/(const U& scalar) const noexcept
		{
			if (scalar == U{})
			{
				dbg::panic("divide by scalar zero: {}/{}", *this, scalar);
			}

			vec_type result = *this;

			for (size_t i = 0; i < N; ++i)
				result[i] /= as<T>(scalar);

			return result;
		}

		// unary
		constexpr vec_type& operator-() const noexcept { return *this * vec_type(-1); }

		constexpr vec_type& operator+() const noexcept { return *this; }

		constexpr auto operator<=>(const vec_type& other) const noexcept = default;

		constexpr bool operator==(const vec_type& other) const noexcept { return equals(other, T{}); }

		constexpr bool equals(const vec_type& other, T epsilon = T{0.000001}) const noexcept
		{
			for (size_t i = 0; i < N; ++i)
			{
				if (not is_close_enough(m_data[i], other[i], epsilon))
					return false;
			}
			return true;
		}

		constexpr vec_type diff(const vec_type& other) const noexcept
		{
			vec_type result = *this;

			for (size_t i = 0; i < N; ++i)
				result[i] -= other[i];

			return result;
		}

		// other functions
		[[nodiscard("Use the minimum value")]] constexpr vec_type min(const vec_type& other) const noexcept
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::min(m_data[i], other[i]);

			return result;
		}

		[[nodiscard("Use the maximum value")]] constexpr vec_type max(const vec_type& other) const noexcept
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::max(m_data[i], other[i]);

			return result;
		}

		[[nodiscard("Use the absolute value")]] constexpr vec_type abs() const noexcept
		{
			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::abs(m_data[i]);

			return result;
		}

		template<std::integral U>
		[[nodiscard("Use the distance value")]] constexpr U distance(const vec_type& other) const noexcept
		{
			U result{};
			for (size_t i = 0; i < N; ++i)
				result += as<U>(std::abs(m_data[i] - other[i]));
			return result;
		}

		template<std::floating_point U>
		[[nodiscard("Use the distance value")]] constexpr U distance(const vec_type& other) const noexcept
		{
			U result{};
			for (size_t i = 0; i < N; ++i)
				result += as<U>(std::abs(m_data[i] - other[i]));
			return result;
		}

		template<std::floating_point U>
		[[nodiscard("Use the clamped value")]] constexpr vec_type clamp(const U cmin, const U cmax) const noexcept
		{

			vec_type result{0};
			for (size_t i = 0; i < N; ++i)
				result[i] = std::clamp(m_data[i], cmin, cmax);

			return result;
		}

		template<typename U = T>
		[[nodiscard("Use the cross product")]] constexpr U cross(const vec_type& other) const noexcept
		requires(N == 2)
		{
			return m_data[0] * other[1] - m_data[1] * other[0];
		}

		template<typename T, size_t M = N>
		requires(N >= 3)
		[[nodiscard("Use the cross product")]] constexpr auto cross(const vec_n<T, M>& other) const noexcept
		{
			vec_n<T, 3> result;

			result[0] = m_data[1] * other[2] - m_data[2] * other[1];
			result[1] = m_data[2] * other[0] - m_data[0] * other[2];
			result[2] = m_data[0] * other[1] - m_data[1] * other[0];

			return result;
		}

		template<typename U = T>
		[[nodiscard("Use the dot product value")]] constexpr U dot(const vec_type& other) const noexcept
		{
			U result{};
			for (size_t i = 0; i < N; ++i)
				result += as<U>(m_data[i] * other[i]);
			return result;
		}

		std::array<T, N> m_data{T{}};
	};

	// Free functions
	export template<typename T, size_t N>
	[[nodiscard("Use the maximum value")]] constexpr vec_n<T, N> min(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.min(rhs);
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the maximum value")]] constexpr vec_n<T, N> max(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.max(rhs);
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the absolute value")]] constexpr vec_n<T, N> abs(const vec_n<T, N>& lhs)
	{
		return lhs.abs();
	}

	export template<typename T, size_t N>
	[[nodiscard("Use the distance value")]] constexpr T distance(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.distance<T>(rhs);
	}

	export template<typename T, typename U = T, size_t N>
	[[nodiscard("Use the clamped value")]] constexpr vec_n<T, N> clamp(const vec_n<T, N>& v, const U cmin, const U cmax)
	{
		return v.clamp(cmin, cmax);
	}

	export template<typename T, size_t N>
	requires(N == 2)
	[[nodiscard("Use the clamped value")]] constexpr auto cross(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<typename T, size_t N, size_t M>
	requires(N >= 3 and M >= 3)
	[[nodiscard("Use the clamped value")]] constexpr vec_n<T, 3> cross(const vec_n<T, N>& lhs, const vec_n<T, M>& rhs)
	{
		return lhs.cross(rhs);
	}

	export template<typename T, size_t N>
	requires(N >= 2)
	[[nodiscard("Use the clamped value")]] constexpr T dot(const vec_n<T, N>& lhs, const vec_n<T, N>& rhs)
	{
		return lhs.dot(rhs);
	}

	export using vec4 = vec_n<float, 4>;
	export using vec3 = vec_n<float, 3>;
	export using vec2 = vec_n<float, 2>;

	static_assert(sizeof(vec4) == 4 * 4, "vec4 ist 4 floats");
	static_assert(sizeof(vec3) == 3 * 4, "vec3 ist 3 floats");
	static_assert(sizeof(vec2) == 2 * 4, "vec2 ist 2 floats");


} // namespace deckard::math

// STD specials
export namespace std

{
	using namespace deckard::math;

	template<>
	struct hash<vec4>
	{
		size_t operator()(const vec4& value) const { return deckard::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<std::integral T, size_t len>
	struct formatter<vec_n<T, len>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const vec_n<T, len>& vec, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "vec{}("), len;
			for (size_t i = 0; i < len; ++i)
				std::format_to(ctx.out(), "{}{}", vec[i], i < len - 1 ? "," : "");

			return std::format_to(ctx.out(), ")");
		}
	};

	template<std::floating_point T, size_t len>
	struct formatter<vec_n<T, len>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const vec_n<T, len>& vec, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "vec{}(", len);
			for (size_t i = 0; i < len; ++i)
				std::format_to(ctx.out(), "{:.3f}{}", vec[i], i < len - 1 ? ", " : "");

			return std::format_to(ctx.out(), ")");
		}
	};

} // namespace std
