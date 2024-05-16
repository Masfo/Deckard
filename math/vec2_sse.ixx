module;
#include <array>
#include <cmath>
#include <immintrin.h>
#include <limits>
#include <numbers>
#include <xmmintrin.h>

export module deckard.math.vec2.sse;

import deckard.debug;

// import deckard.math.utility;

namespace deckard::math::sse
{
	__m128 horizontal_add(const __m128& lhs) noexcept
	{
		__m128 shuf = _mm_shuffle_ps(lhs, lhs, _MM_SHUFFLE(2, 3, 0, 1)); // [ C D | A B ]
		__m128 sums = _mm_add_ps(lhs, shuf);                             // sums = [ D+C C+D | B+A A+B ]
		shuf        = _mm_movehl_ps(shuf, sums); //  [   C   D | D+C C+D ]  // let the compiler avoid a mov by reusing shuf
		sums        = _mm_add_ss(sums, shuf);
		return sums;
	};

	float horizontal_addf(const __m128& lhs) noexcept { return _mm_cvtss_f32(horizontal_add(lhs)); };

	export struct alignas(16) vec2
	{
		vec2()
			: vec2(0.0f)
		{
		}

		vec2(__m128 r)
			: reg(r){};

		vec2& operator=(const __m128& lhs) noexcept
		{
			reg = lhs;
			return *this;
		}

		vec2& operator=(const vec2& lhs) noexcept
		{
			reg = lhs.reg;
			return *this;
		}

		operator __m128() const { return reg; }

		vec2(float s) noexcept { reg = _mm_set_ps(1.0f, 1.0f, s, s); }

		vec2(float x, float y) { reg = _mm_set_ps(1.0f, 1.0f, y, x); }

		vec2& operator+=(const vec2& lhs) noexcept
		{
			reg = _mm_add_ps(reg, lhs.reg);
			return *this;
		}

		vec2 operator+(const vec2& lhs) const noexcept
		{
			auto result = *this;
			result += lhs;

			return result;
		}

		vec2& operator-=(const vec2& lhs) noexcept
		{
			reg = _mm_sub_ps(reg, lhs.reg);
			return *this;
		}

		vec2 operator-(const vec2& lhs) const noexcept
		{
			auto result = *this;
			result -= lhs;
			return result;
		}

		vec2& operator*=(const vec2& lhs) noexcept
		{
			reg = _mm_mul_ps(reg, lhs.reg);
			return *this;
		}

		vec2 operator*(const vec2& lhs) const noexcept
		{
			auto result = *this;
			result *= lhs;
			return result;
		}

		vec2& operator/=(const vec2& lhs) noexcept
		{
			reg = _mm_div_ps(reg, lhs.reg);

			return *this;
		}

		vec2 operator/(const vec2& lhs) const noexcept
		{
			auto result = *this;
			result /= lhs;
			return result;
		}

		vec2& operator-() noexcept
		{
			reg = _mm_mul_ps(reg, neg_one);
			return *this;
		}

		vec2 min(const vec2& lhs) const noexcept { return _mm_min_ps(reg, lhs.reg); }

		vec2 max(const vec2& lhs) const noexcept { return _mm_max_ps(reg, lhs.reg); }

		vec2 abs() const noexcept { return _mm_andnot_ps(neg_zero, reg); }

		float length() const noexcept
		{
			__m128 a     = _mm_mul_ps(reg, reg);
			a            = _mm_mul_ps(a, xymask);
			auto  rc     = _mm_sqrt_ps(horizontal_add(a));
			float result = _mm_cvtss_f32(rc);
			return result;
		}

		vec2 normalized() const noexcept { return *this / length(); }

		void normalize() noexcept { *this = normalized(); }

		float distance(const vec2& lhs) const noexcept
		{
			__m128 tmp = _mm_sub_ps(reg, lhs.reg);
			__m128 sqr = _mm_mul_ps(tmp, tmp);
			return _mm_cvtss_f32(_mm_sqrt_ps(horizontal_add(sqr)));
		}

		vec2 clamp(float cmin, float cmax) const noexcept { return vec2(_mm_min_ps(_mm_max_ps(reg, vec2(cmin)), vec2(cmax))); }

		bool equals(const vec2& lhs) const noexcept { return is_close_enough(lhs); }

		bool is_close_enough(const vec2& lhs, float epsilon = 0.0000001f) const noexcept
		{
			auto masked_this = *this * vec2(xymask);
			auto masked_lhs  = lhs * vec2(xymask);

			auto diff   = masked_this - masked_lhs;
			auto result = _mm_cmple_ps(diff.abs().reg, _mm_set_ps1(epsilon));
			auto mask   = _mm_movemask_ps(result);
			return mask == 0xF;
		}

		// cross


		float cross(const vec2& lhs) const noexcept
		{
			__m128 tmp0 = _mm_shuffle_ps(lhs.reg, lhs.reg, _MM_SHUFFLE(0, 1, 0, 1));
			__m128 tmp1 = _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 0, 1, 0));
			tmp0        = _mm_mul_ps(tmp0, tmp1);
			tmp1        = _mm_shuffle_ps(tmp0, tmp0, _MM_SHUFFLE(0, 1, 0, 1));
			tmp0        = _mm_sub_ps(tmp0, tmp1);

			return _mm_cvtss_f32(tmp0);
		}

		// dot
		float dot(const vec2& lhs) const noexcept
		{
			__m128 masked_reg = _mm_mul_ps(reg, xymask);
			__m128 masked_lhs = _mm_mul_ps(lhs.reg, xymask);

			__m128 mul = _mm_mul_ps(masked_reg, masked_lhs);
			return horizontal_addf(mul);
		}

		[[nodiscard("Use the projected vector")]] vec2 project(const vec2& other) const noexcept
		{

			if (other.has_zero())
			{
				// TODO:
				// dbg::trace("cannot project onto a zero vector: {} / {}", *this, other);
				return vec2(inf);
			}

			auto dot_ab   = dot(other);
			auto b_length = other.length();

			float projection_scalar = dot_ab / (b_length * b_length);
			return other * projection_scalar;
		}

		[[nodiscard("Use the angle value")]] float angle(const vec2& other) const noexcept
		{
			if (has_zero() or other.has_zero() or has_inf())
			{
				// TODO
				// dbg::trace("cannot take angle between zero vectors: {} / {}", *this, other);
				return std::numeric_limits<float>::infinity();
			}

			float cosTheta = dot(other) / (length() * other.length());


			return std::acos(cosTheta) * 180.0f / std::numbers::pi_v<float>;
		}

		[[nodiscard("Use the rotated vector")]] vec2 rotate(const vec2& axis, const float rad) const noexcept
		{
			const vec2 axis_norm = axis.normalized();
			const vec2 v         = *this;

			float cosTheta         = std::cos(rad);
			float sinTheta         = std::sin(rad);
			float oneMinusCosTheta = 1.0f - cosTheta;

			return (v * cosTheta) + (v.cross(axis) * sinTheta) + (axis * v.dot(axis)) * oneMinusCosTheta;
		}

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] vec2 safe_divide(const vec2& other) const noexcept
		{
			if (other.has_zero())
				return vec2(inf);

			return *this / other;
		}

		[[nodiscard("Use the divide scalar")]] vec2 safe_divide(const float scalar) const noexcept
		{
			if (scalar == 0.0f)
				return vec2(inf);

			return *this / vec2(scalar);
		}

		// has / is
		bool has_zero() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero));
			return mask != 0;
		}

		bool is_zero() const noexcept
		{

			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(_mm_mul_ps(reg, xymask), zero));
			return mask == 0xF;
		}

		bool has_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf));
			return mask != 0x0;
		}

		bool is_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(_mm_mul_ps(reg, inf_xymask), inf));
			return mask == 0xF;
		}

		// cmp
		bool operator==(const vec2& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, lhs.reg));
			return mask == 0xF;
		}

		//
		bool operator<=(const vec2& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmple_ps(reg, lhs.reg));
			return mask != 0;
		}

		bool operator>=(const vec2& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpge_ps(reg, lhs.reg));
			return mask != 0;
		}

		//
		bool operator>(const vec2& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpgt_ps(reg, lhs.reg));
			return mask != 0;
		}

		bool operator<(const vec2& lhs) const noexcept
		{
			auto cmp  = _mm_cmplt_ps(reg, lhs.reg);
			auto mask = _mm_movemask_ps(cmp);
			return mask != 0;
		}

		float operator[](const int index) const noexcept
		{

			switch (index)
			{
				case 0: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(0, 0, 0, 0)));
				case 1: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(1, 1, 1, 1)));
				default: dbg::panic("vec2: indexing out-of-bound");
			}
		}

		inline static float nan_float = std::numeric_limits<float>::quiet_NaN();
		inline static float inf_float = std::numeric_limits<float>::infinity();


		inline static __m128 xymask     = _mm_set_ps(0, 0, 1, 1);
		inline static __m128 nan_xymask = _mm_set_ps(nan_float, nan_float, 1, 1);
		inline static __m128 inf_xymask = _mm_set_ps(inf_float, inf_float, 1, 1);


		inline static __m128 zero     = _mm_set_ps1(0.0f);
		inline static __m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static __m128 one     = _mm_set_ps1(1.0f);
		inline static __m128 neg_one = _mm_set_ps1(-1.0f);

		inline static __m128 inf = _mm_set_ps1(inf_float);


		__m128 reg;
	};

	static_assert(sizeof(vec2) == 16);

	// Free functions
	export [[nodiscard("Use the maximum value")]] vec2 min(const vec2& lhs, const vec2& rhs) { return lhs.min(rhs); }

	export [[nodiscard("Use the maximum vector")]] vec2 max(const vec2& lhs, const vec2& rhs) { return lhs.max(rhs); }

	export [[nodiscard("Use the absolute vector")]] vec2 abs(const vec2& lhs) { return lhs.abs(); }

	export [[nodiscard("Use the distance value")]] float distance(const vec2& lhs, const vec2& rhs) { return lhs.distance(rhs); }

	export [[nodiscard("Use the clamped vector")]] vec2 clamp(const vec2& v, float cmin, float cmax) { return v.clamp(cmin, cmax); }

	export [[nodiscard("Use the clamped value")]] float dot(const vec2& lhs, const vec2& rhs) { return lhs.dot(rhs); }

	export [[nodiscard("Use the length value")]] float length(const vec2& rhs) { return rhs.length(); }

	export void normalize(vec2& rhs) { rhs.normalize(); }

	export [[nodiscard("Use the normalized value")]] auto normalized(const vec2& rhs) { return rhs.normalized(); }

	export [[nodiscard("Use the projected vector")]] float cross(const vec2& lhs, const vec2& rhs) { return lhs.cross(rhs); }

	export [[nodiscard("Use the projected vector")]] vec2 project(const vec2& lhs, const vec2& rhs) { return lhs.project(rhs); }

	export [[nodiscard("Use the angle value")]] float angle(const vec2& lhs, const vec2& rhs) { return lhs.angle(rhs); }

	export [[nodiscard("Use the rotated vector")]] vec2 rotate(const vec2& v, const vec2& axis, float angle)
	{
		return v.rotate(axis, angle);
	}

	export [[nodiscard("Use the divided value")]] vec2 safe_divide(const vec2& lhs, const vec2& rhs) { return lhs.safe_divide(rhs); }

	export [[nodiscard("Use the divided value")]] vec2 safe_divide(const vec2& lhs, const float scalar) { return lhs.safe_divide(scalar); }

} // namespace deckard::math::sse
