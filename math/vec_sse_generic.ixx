module;
#include <array>
#include <cmath>
#include <immintrin.h>
#include <limits>
#include <numbers>
#include <xmmintrin.h>

export module deckard.math:vec_sse_generic;

import deckard.debug;
import deckard.math.utils;

namespace deckard::math::sse
{
	export template<size_t N>
	struct alignas(16) vec_n_sse
	{
		__m128 reg;

		vec_n_sse(const __m128 r)
			: reg(r)
		{
		}

		vec_n_sse(const float* v) { reg = _mm_load_ps(v); }

		vec_n_sse()
			: vec_n_sse(0.0f)
		{
		}

		vec_n_sse(float scalar) noexcept { reg = _mm_set_ps1(scalar); }

		vec_n_sse(float x, float y)
			: vec_n_sse(x, y, 0.0f, 0.0f)
		{
		}

		vec_n_sse(float x, float y, float z)
			: vec_n_sse(x, y, z, 0.0f)
		{
		}

		vec_n_sse(float x, float y, float z, float w) { reg = _mm_set_ps(w, z, y, x); }

		vec_n_sse(const vec_n_sse<3>& v, float w)
		{
			reg       = v.reg;
			auto tmp0 = _mm_shuffle_ps(_mm_set_ps1(w), v.reg, _MM_SHUFFLE(3, 2, 0, 0));
			reg       = _mm_shuffle_ps(reg, tmp0, _MM_SHUFFLE(0, 2, 1, 0));
		}

		vec_n_sse(const vec_n_sse<2>& v, float w) { reg = _mm_shuffle_ps(v.reg, _mm_set_ps1(w), _MM_SHUFFLE(0, 0, 1, 0)); }

		using vec_type = vec_n_sse;

		void operator=(const __m128& lhs) noexcept { reg = lhs; }

		void operator=(const vec_type& lhs) noexcept { reg = lhs.reg; }

		void operator+=(const vec_type& lhs) noexcept { reg = _mm_add_ps(reg, lhs.reg); }

		void operator-=(const vec_type& lhs) noexcept { reg = _mm_sub_ps(reg, lhs.reg); }

		void operator*=(const vec_type& lhs) noexcept { reg = _mm_mul_ps(reg, lhs.reg); }

		void operator/=(const vec_type& lhs) noexcept { reg = _mm_div_ps(reg, lhs.reg); }

		// operator = scalar
		void operator+=(const float scalar) noexcept { reg = _mm_add_ps(reg, _mm_set_ps1(scalar)); }

		void operator-=(const float scalar) noexcept { reg = _mm_sub_ps(reg, _mm_set_ps1(scalar)); }

		void operator*=(const float scalar) noexcept { reg = _mm_mul_ps(reg, _mm_set_ps1(scalar)); }

		void operator/=(const float scalar) noexcept { reg = _mm_div_ps(reg, _mm_set_ps1(scalar)); }

		vec_type operator+(const vec_type& lhs) const noexcept { return _mm_add_ps(reg, lhs.reg); }

		vec_type operator-(const vec_type& lhs) const noexcept { return _mm_sub_ps(reg, lhs.reg); }

		vec_type operator*(const vec_type& lhs) const noexcept { return _mm_mul_ps(reg, lhs.reg); }

		vec_type operator/(const vec_type& lhs) const noexcept { return _mm_div_ps(reg, lhs.reg); }

		vec_type operator-() noexcept { return vec_type(_mm_mul_ps(reg, neg_one)); }

		vec_type operator+() const noexcept { return *this; }

		void operator>>(float* v) noexcept { _mm_store_ps(v, reg); }

		void operator<<(float* v) noexcept
		{
			// TODO: assert
			// assert::check(v != nullptr);
			reg = _mm_load_ps(v);
		}

		void operator<<=(float* v) noexcept
		{
			// TODO: assert
			// assert::check(v!=nullptr);
			reg = _mm_load_ps(v);
		}

		// operations
		vec_type min(const vec_type& lhs) const noexcept { return _mm_min_ps(reg, lhs.reg); }

		vec_type max(const vec_type& lhs) const noexcept { return _mm_max_ps(reg, lhs.reg); }

		vec_type abs() const noexcept { return _mm_andnot_ps(neg_zero, reg); }

		float length() const noexcept
		{
			__m128 a      = _mm_mul_ps(reg, reg);
			auto   rc     = _mm_sqrt_ps(horizontal_add(a));
			float  result = _mm_cvtss_f32(rc);
			return result;
		}

		vec_type normalized() const noexcept { return *this / vec_type(length()); }

		void normalize() noexcept { *this = normalized(); }

		float distance(const vec_type& lhs) const noexcept
		{
			__m128 tmp = _mm_sub_ps(reg, lhs.reg);
			__m128 sqr = _mm_mul_ps(tmp, tmp);
			return _mm_cvtss_f32(_mm_sqrt_ps(horizontal_add(sqr)));
		}

		vec_type clamp(float cmin, float cmax) const noexcept
		{
			__m128 tmp0 = _mm_min_ps(_mm_max_ps(reg, _mm_set_ps1(cmin)), _mm_set_ps1(cmax));
			return vec_type(tmp0);
		}

		bool equals(const vec_type& lhs) const noexcept { return is_close_enough(lhs); }

		bool is_close_enough(const vec_type& lhs, float epsilon = 0.0000001f) const noexcept
		{
			// TODO: mask vec3 xyz, mask2 xy
			auto masked_this = *this;
			auto masked_lhs  = lhs;
			if constexpr (N == 2)
			{
				masked_this = *this * vec_type(xymask);
				masked_lhs  = lhs * vec_type(xymask);
			}
			else if constexpr (N == 3)
			{
				masked_this = *this * vec_type(xyzmask);
				masked_lhs  = lhs * vec_type(xyzmask);
			}
			auto diff   = masked_this - masked_lhs;
			auto result = _mm_cmple_ps(diff.abs().reg, _mm_set_ps1(epsilon));
			auto mask   = _mm_movemask_ps(result);
			return mask == 0xF;
		}

		vec_type cross(const vec_type& lhs) const noexcept
		{
			__m128 tmp0 = _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 0, 2, 1));
			__m128 tmp1 = _mm_shuffle_ps(lhs.reg, lhs.reg, _MM_SHUFFLE(3, 1, 0, 2));
			__m128 tmp2 = _mm_mul_ps(tmp0, lhs.reg);
			__m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
			__m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
			return vec_type(_mm_sub_ps(tmp3, tmp4));
		}

		float cross(const vec_type& lhs) const noexcept
		requires(N == 2)
		{
			__m128 tmp0 = _mm_shuffle_ps(lhs.reg, lhs.reg, _MM_SHUFFLE(0, 1, 0, 1));
			__m128 tmp1 = _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 0, 1, 0));
			tmp0        = _mm_mul_ps(tmp0, tmp1);
			tmp1        = _mm_shuffle_ps(tmp0, tmp0, _MM_SHUFFLE(0, 1, 0, 1));
			tmp0        = _mm_sub_ps(tmp0, tmp1);

			return _mm_cvtss_f32(tmp0);
		}

		// only use xyz
		float dot(const vec_type& lhs) const noexcept
		requires(N == 4)
		{
			__m128 mul = _mm_mul_ps(reg, lhs.reg);
			return horizontal_addf(mul);
		}

		float dot(const vec_n_sse<3>& lhs) const noexcept
		requires(N == 3)
		{
			__m128 masked_reg = _mm_mul_ps(reg, xyzmask);
			__m128 masked_lhs = _mm_mul_ps(lhs.reg, xyzmask);

			__m128 mul = _mm_mul_ps(masked_reg, masked_lhs);
			return horizontal_addf(mul);
		}

		float dot(const vec_n_sse<2>& lhs) const noexcept
		requires(N == 2)
		{
			__m128 masked_reg = _mm_mul_ps(reg, xymask);
			__m128 masked_lhs = _mm_mul_ps(lhs.reg, xymask);

			__m128 mul = _mm_mul_ps(masked_reg, masked_lhs);
			return horizontal_addf(mul);
		}

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] vec_type safe_divide(const vec_type& other) const noexcept
		{
			if (other.has_zero())
			{
				return vec_type(inf);
			}

			return *this / other;
		}

		[[nodiscard("Use the divide scalar")]] vec_type safe_divide(const float scalar) const noexcept
		{
			if (scalar == 0.0f)
				return vec_type(inf);

			return *this / vec_type(scalar);
		}

		//
		[[nodiscard("Use the projected vector")]] vec_type project(const vec_type& other) const noexcept
		requires(N == 2 or N == 3)
		{

			if (other.has_zero())
			{
				// dbg::trace("cannot project onto a zero vector: {} / {}", *this, other);
				return vec_type(inf);
			}

			auto dot_ab   = dot(other);
			auto b_length = other.length();

			float projection_scalar = dot_ab / (b_length * b_length);
			return other * projection_scalar;
		}

		[[nodiscard("Use the angle value")]] float angle(const vec_type& other) const noexcept
		requires(N == 2 or N == 3)
		{
			if (has_zero() or other.has_zero())
			{
				return inf_float;
			}

			float cosTheta = dot(other) / (length() * other.length());


			return std::acos(cosTheta) * 180.0f / std::numbers::pi_v<float>;
		}

		[[nodiscard("Use the rotated vector")]] vec_type rotate(const vec_type& axis, const float rad) const noexcept
		requires(N == 2 or N == 3)
		{
			const vec_type axis_norm = axis.normalized();
			const vec_type v         = *this;

			float cosTheta         = std::cos(rad);
			float sinTheta         = std::sin(rad);
			float oneMinusCosTheta = 1.0f - cosTheta;

			return (v * cosTheta) + (v.cross(axis) * sinTheta) + (axis * v.dot(axis)) * oneMinusCosTheta;
		}

		// has / is
		bool is_invalid() const noexcept { return has_zero() or has_inf() or has_nan(); }

		bool has_zero() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero));
			return mask <= 0xF;
		}

		bool is_zero() const noexcept
		requires(N == 4)
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero));
			return mask == 0xF;
		}

		bool has_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf));
			return mask != 0;
		}

		bool is_inf() const noexcept
		requires(N == 4)
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf));
			return mask == 0xF;
		}

		bool has_nan() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, nan));
			return mask != 0;
		}

		bool is_nan() const noexcept
		requires(N == 4)
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, nan));
			return mask == 0xF;
		}

		// cmp
		bool operator==(const vec_type& lhs) const noexcept { return is_close_enough(lhs); }

		//
		bool operator<=(const vec_type& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmple_ps(reg, lhs.reg));
			return mask != 0;
		}

		bool operator>=(const vec_type& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpge_ps(reg, lhs.reg));
			return mask != 0;
		}

		//
		bool operator>(const vec_type& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpgt_ps(reg, lhs.reg));
			return mask != 0;
		}

		bool operator<(const vec_type& lhs) const noexcept
		{
			auto cmp  = _mm_cmplt_ps(reg, lhs.reg);
			auto mask = _mm_movemask_ps(cmp);
			return mask != 0;
		}

		float operator[](const int index) const noexcept
		requires(N == 4)
		{
			switch (index)
			{
				case 0: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(0, 0, 0, 0)));
				case 1: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 1, 1, 1)));
				case 2: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(2, 2, 2, 2)));
				case 3: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 3, 3, 3)));
				default:
					std::unreachable();
					{
						// dbg::trace(std::source_location::current());
						// dbg::panic("vec4: indexing out-of-bound");
					}
			}
		}

		float operator[](const int index) const noexcept
		requires(N == 3)
		{
			switch (index)
			{
				case 0: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(0, 0, 0, 0)));
				case 1: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 1, 1, 1)));
				case 2: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(2, 2, 2, 2)));
				default:
					std::unreachable();
					{
						// dbg::trace(std::source_location::current());
						// dbg::panic("vec3: indexing out-of-bound");
					}
			}
		}

		float operator[](const int index) const noexcept
		requires(N == 2)
		{
			switch (index)
			{
				case 0: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(0, 0, 0, 0)));
				case 1: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 1, 1, 1)));
				default:
					std::unreachable();
					{
						// dbg::trace(std::source_location::current());
						// dbg::panic("vec2: indexing out-of-bound");
					}
			}
		}

		inline static float nan_float = std::numeric_limits<float>::quiet_NaN();
		inline static float inf_float = std::numeric_limits<float>::infinity();

		inline static __m128 xymask     = _mm_set_ps(0, 0, 1, 1);
		inline static __m128 nan_xymask = _mm_set_ps(nan_float, nan_float, 1, 1);
		inline static __m128 inf_xymask = _mm_set_ps(inf_float, inf_float, 1, 1);

		inline static __m128 xyzmask  = _mm_set_ps(0, 1, 1, 1);
		inline static __m128 zero     = _mm_set_ps1(0.0f);
		inline static __m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static __m128 one     = _mm_set_ps1(1.0f);
		inline static __m128 neg_one = _mm_set_ps1(-1.0f);

		inline static __m128 inf = _mm_set_ps1(std::numeric_limits<float>::infinity());
	};

	// Free functions

	// min ------------------------
	export [[nodiscard("Use the maximum value")]] vec_n_sse<4> min(const vec_n_sse<4>& lhs, const vec_n_sse<4>& rhs)
	{
		return lhs.min(rhs);
	}

	export [[nodiscard("Use the maximum value")]] vec_n_sse<3> min(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs)
	{
		return lhs.min(rhs);
	}

	export [[nodiscard("Use the maximum value")]] vec_n_sse<2> min(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs)
	{
		return lhs.min(rhs);
	}

	// max ------------------------
	export [[nodiscard("Use the maximum vector")]] vec_n_sse<4> max(const vec_n_sse<4>& lhs, const vec_n_sse<4>& rhs)
	{
		return lhs.max(rhs);
	}

	export [[nodiscard("Use the maximum vector")]] vec_n_sse<3> max(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs)
	{
		return lhs.max(rhs);
	}

	export [[nodiscard("Use the maximum vector")]] vec_n_sse<2> max(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs)
	{
		return lhs.max(rhs);
	}

	// abs ------------------------
	export [[nodiscard("Use the absolute vector")]] vec_n_sse<4> abs(const vec_n_sse<4>& lhs) { return lhs.abs(); }

	export [[nodiscard("Use the absolute vector")]] vec_n_sse<3> abs(const vec_n_sse<3>& lhs) { return lhs.abs(); }

	export [[nodiscard("Use the absolute vector")]] vec_n_sse<2> abs(const vec_n_sse<2>& lhs) { return lhs.abs(); }

	// distance ------------------------
	export [[nodiscard("Use the distance value")]] float distance(const vec_n_sse<4>& lhs, const vec_n_sse<4>& rhs)
	{
		return lhs.distance(rhs);
	}

	export [[nodiscard("Use the distance value")]] float distance(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs)
	{
		return lhs.distance(rhs);
	}

	export [[nodiscard("Use the distance value")]] float distance(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs)
	{
		return lhs.distance(rhs);
	}

	// clamp ------------------------
	export [[nodiscard("Use the clamped vector")]] vec_n_sse<4> clamp(const vec_n_sse<4>& v, float cmin, float cmax)
	{
		return v.clamp(cmin, cmax);
	}

	export [[nodiscard("Use the clamped vector")]] vec_n_sse<3> clamp(const vec_n_sse<3>& v, float cmin, float cmax)
	{
		return v.clamp(cmin, cmax);
	}

	export [[nodiscard("Use the clamped vector")]] vec_n_sse<2> clamp(const vec_n_sse<2>& v, float cmin, float cmax)
	{
		return v.clamp(cmin, cmax);
	}

	// length / magnitude ------------------------
	export [[nodiscard("Use the length value")]] float length(const vec_n_sse<4>& rhs) { return rhs.length(); }

	export [[nodiscard("Use the length value")]] float length(const vec_n_sse<3>& rhs) { return rhs.length(); }

	export [[nodiscard("Use the length value")]] float length(const vec_n_sse<2>& rhs) { return rhs.length(); }

	// normalize ------------------------
	export void normalize(vec_n_sse<4>& rhs) { rhs.normalize(); }

	export void normalize(vec_n_sse<3>& rhs) { rhs.normalize(); }

	export void normalize(vec_n_sse<2>& rhs) { rhs.normalize(); }

	// normalized
	export [[nodiscard("Use the normalized value")]] auto normalized(const vec_n_sse<4>& rhs) { return rhs.normalized(); }

	export [[nodiscard("Use the normalized value")]] auto normalized(const vec_n_sse<3>& rhs) { return rhs.normalized(); }

	export [[nodiscard("Use the normalized value")]] auto normalized(const vec_n_sse<2>& rhs) { return rhs.normalized(); }

	// dot product ------------------------
	export [[nodiscard("Use the dot product value")]] float dot(const vec_n_sse<4>& lhs, const vec_n_sse<4>& rhs) { return lhs.dot(rhs); }

	export [[nodiscard("Use the dot product value")]] float dot(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs) { return lhs.dot(rhs); }

	export [[nodiscard("Use the dot product value")]] float dot(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs) { return lhs.dot(rhs); }

	// cross product ------------------------
	export [[nodiscard("Use the projected vector")]] vec_n_sse<4> cross(const vec_n_sse<4>& lhs, const vec_n_sse<4>& rhs)
	{
		return lhs.cross(rhs);
	}

	export [[nodiscard("Use the projected vector")]] vec_n_sse<3> cross(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs)
	{
		return lhs.cross(rhs);
	}

	export [[nodiscard("Use the cross product vector")]] float cross(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs)
	{
		return lhs.cross(rhs);
	}

	// project ------------------------
	export [[nodiscard("Use the projected vector")]] vec_n_sse<3> project(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs)
	{
		return lhs.project(rhs);
	}

	export [[nodiscard("Use the projected vector")]] vec_n_sse<2> project(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs)
	{
		return lhs.project(rhs);
	}

	// divides ------------------------
	// 4
	export [[nodiscard("Use the divided value")]] vec_n_sse<4> safe_divide(const vec_n_sse<4>& lhs, const vec_n_sse<4>& rhs)
	{
		return lhs.safe_divide(rhs);
	}

	export [[nodiscard("Use the divided value")]] vec_n_sse<4> safe_divide(const vec_n_sse<4>& lhs, const float scalar)
	{
		return lhs.safe_divide(scalar);
	}

	// 3
	export [[nodiscard("Use the divided value")]] vec_n_sse<3> safe_divide(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs)
	{
		return lhs.safe_divide(rhs);
	}

	export [[nodiscard("Use the divided value")]] vec_n_sse<3> safe_divide(const vec_n_sse<3>& lhs, const float scalar)
	{
		return lhs.safe_divide(scalar);
	}

	// 2
	export [[nodiscard("Use the divided value")]] vec_n_sse<2> safe_divide(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs)
	{
		return lhs.safe_divide(rhs);
	}

	export [[nodiscard("Use the divided value")]] vec_n_sse<2> safe_divide(const vec_n_sse<2>& lhs, const float scalar)
	{
		return lhs.safe_divide(scalar);
	}

	// angle ------------------------
	export [[nodiscard("Use the angle value")]] float angle(const vec_n_sse<3>& lhs, const vec_n_sse<3>& rhs) { return lhs.angle(rhs); }

	export [[nodiscard("Use the angle value")]] float angle(const vec_n_sse<2>& lhs, const vec_n_sse<2>& rhs) { return lhs.angle(rhs); }

	// rotate ------------------------
	export [[nodiscard("Use the rotated vector")]] vec_n_sse<3> rotate(const vec_n_sse<3>& v, const vec_n_sse<3>& axis, float angle)
	{
		return v.rotate(axis, angle);
	}

	export [[nodiscard("Use the rotated vector")]] vec_n_sse<2> rotate(const vec_n_sse<2>& v, const vec_n_sse<2>& axis, float angle)
	{
		return v.rotate(axis, angle);
	}

} // namespace deckard::math::sse
