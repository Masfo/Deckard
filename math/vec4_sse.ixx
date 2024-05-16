module;
#include <array>
#include <immintrin.h>
#include <limits>
#include <xmmintrin.h>

export module deckard.math.vec4.sse;

import deckard.math.vec3.sse;
import deckard.math.utility;
import deckard.debug;

namespace deckard::math::sse
{

	export template<typename T, size_t N>
	struct alignas(16) vec_sse_n
	{
		__m128 reg;
	};

	export struct alignas(16) vec4
	{
		vec4()
			: vec4(0.0f)
		{
		}

		vec4(__m128 r)
			: reg(r){};

		void operator=(const __m128& lhs) noexcept { reg = lhs; }

		void operator=(const vec4& lhs) noexcept { reg = lhs.reg; }

		operator __m128() const { return reg; }

		vec4(float scalar) noexcept { reg = _mm_set_ps1(scalar); }

		vec4(float x, float y)
			: vec4(x, y, 0.0f, 0.0f)
		{
		}

		vec4(float x, float y, float z)
			: vec4(x, y, z, 0.0)
		{
		}

		vec4(float x, float y, float z, float w) { reg = _mm_set_ps(w, z, y, x); }

		vec4(const vec3& v, float w)
		{
			reg       = v.reg;
			auto tmp0 = _mm_shuffle_ps(_mm_set_ps1(w), v.reg, _MM_SHUFFLE(3, 2, 0, 0));
			reg       = _mm_shuffle_ps(reg, tmp0, _MM_SHUFFLE(0, 2, 1, 0));
		}

		void operator+=(const vec4& lhs) noexcept { reg = _mm_add_ps(reg, lhs.reg); }

		void operator-=(const vec4& lhs) noexcept { reg = _mm_sub_ps(reg, lhs.reg); }

		void operator*=(const vec4& lhs) noexcept { reg = _mm_mul_ps(reg, lhs.reg); }

		void operator/=(const vec4& lhs) noexcept { reg = _mm_div_ps(reg, lhs.reg); }

		vec4 operator+(const vec4& lhs) const noexcept { return vec4(_mm_add_ps(reg, lhs.reg)); }

		vec4 operator-(const vec4& lhs) const noexcept { return vec4(_mm_sub_ps(reg, lhs.reg)); }

		vec4 operator*(const vec4& lhs) const noexcept { return vec4(_mm_mul_ps(reg, lhs.reg)); }

		vec4 operator/(const vec4& lhs) const noexcept { return vec4(_mm_div_ps(reg, lhs.reg)); }

		vec4 operator-() noexcept { return vec4(_mm_mul_ps(reg, neg_one)); }

		// operations
		vec4 min(const vec4& lhs) const noexcept { return _mm_min_ps(reg, lhs.reg); }

		vec4 max(const vec4& lhs) const noexcept { return _mm_max_ps(reg, lhs.reg); }

		vec4 abs() const noexcept { return _mm_andnot_ps(neg_zero, reg); }

		float length() const noexcept
		{
			__m128 a      = _mm_mul_ps(reg, reg);
			auto   rc     = _mm_sqrt_ps(horizontal_add(a));
			float  result = _mm_cvtss_f32(rc);
			return result;
		}

		vec4 normalized() const noexcept { return *this / length(); }

		void normalize() noexcept { *this = normalized(); }

		float distance(const vec4& lhs) const noexcept
		{
			__m128 tmp = _mm_sub_ps(reg, lhs.reg);
			__m128 sqr = _mm_mul_ps(tmp, tmp);
			return _mm_cvtss_f32(_mm_sqrt_ps(horizontal_add(sqr)));
		}

		vec4 clamp(float cmin, float cmax) const noexcept { return vec4(_mm_min_ps(_mm_max_ps(reg, vec4(cmin)), vec4(cmax))); }

		bool equals(const vec4& lhs) const noexcept { return is_close_enough(lhs); }

		bool is_close_enough(const vec4& lhs, float epsilon = 0.0000001f) const noexcept
		{
			auto diff   = *this - lhs;
			auto result = _mm_cmple_ps(diff.abs().reg, _mm_set_ps1(epsilon));
			auto mask   = _mm_movemask_ps(result);
			return mask == 0xF;
		}

		vec4 cross(const vec4& lhs) const noexcept
		{
			__m128 tmp0 = _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 0, 2, 1));
			__m128 tmp1 = _mm_shuffle_ps(lhs.reg, lhs.reg, _MM_SHUFFLE(3, 1, 0, 2));
			__m128 tmp2 = _mm_mul_ps(tmp0, lhs.reg);
			__m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
			__m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
			return _mm_sub_ps(tmp3, tmp4);
		}

		// only use xyz
		float dot(const vec4& lhs) const noexcept
		{
			__m128 mul = _mm_mul_ps(reg, lhs.reg);
			return horizontal_addf(mul);
		}

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] vec4 safe_divide(const vec4& other) const noexcept
		{
			if (other.has_zero())
			{
				return vec4(inf);
			}

			return *this / other;
		}

		[[nodiscard("Use the divide scalar")]] vec4 safe_divide(const float scalar) const noexcept
		{
			if (scalar == 0.0f)
				return vec4(inf);

			return *this / vec4(scalar);
		}

		// has / is
		bool has_zero() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero));
			return mask <= 0xF;
		}

		bool is_zero() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero));
			return mask == 0xF;
		}

		bool has_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf));
			return mask <= 0xF;
		}

		bool is_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf));
			return mask == 0xF;
		}

		// cmp
		bool operator==(const vec4& lhs) const noexcept { return is_close_enough(lhs); }

		//
		bool operator<=(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmple_ps(reg, lhs.reg));
			return mask != 0;
		}

		bool operator>=(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpge_ps(reg, lhs.reg));
			return mask != 0;
		}

		//
		bool operator>(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpgt_ps(reg, lhs.reg));
			return mask != 0;
		}

		bool operator<(const vec4& lhs) const noexcept
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
				case 2: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(2, 2, 2, 2)));
				case 3: return _mm_cvtss_f32(_mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 3, 3, 3)));
				default:
				{
					dbg::trace(std::source_location::current());
					dbg::panic("vec4: indexing out-of-bound");
				}
			}
		}

		inline static __m128 xyzmask  = _mm_set_ps(0, 1, 1, 1);
		inline static __m128 zero     = _mm_set_ps1(0.0f);
		inline static __m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static __m128 one     = _mm_set_ps1(1.0f);
		inline static __m128 neg_one = _mm_set_ps1(-1.0f);

		inline static __m128 inf = _mm_set_ps1(std::numeric_limits<float>::infinity());


		__m128 reg;
	};

	static_assert(sizeof(vec4) == 16);

	// Free functions
	export [[nodiscard("Use the maximum value")]] vec4 min(const vec4& lhs, const vec4& rhs) { return lhs.min(rhs); }

	export [[nodiscard("Use the maximum vector")]] vec4 max(const vec4& lhs, const vec4& rhs) { return lhs.max(rhs); }

	export [[nodiscard("Use the absolute vector")]] vec4 abs(const vec4& lhs) { return lhs.abs(); }

	export [[nodiscard("Use the distance value")]] float distance(const vec4& lhs, const vec4& rhs) { return lhs.distance(rhs); }

	export [[nodiscard("Use the clamped vector")]] vec4 clamp(const vec4& v, float cmin, float cmax) { return v.clamp(cmin, cmax); }

	export [[nodiscard("Use the clamped value")]] float dot(const vec4& lhs, const vec4& rhs) { return lhs.dot(rhs); }

	export [[nodiscard("Use the length value")]] float length(const vec4& rhs) { return rhs.length(); }

	export void normalize(vec4& rhs) { rhs.normalize(); }

	export [[nodiscard("Use the normalized value")]] auto normalized(const vec4& rhs) { return rhs.normalized(); }

	export [[nodiscard("Use the projected vector")]] vec4 cross(const vec4& lhs, const vec4& rhs) { return lhs.cross(rhs); }

	export [[nodiscard("Use the divided value")]] vec4 safe_divide(const vec4& lhs, const vec4& rhs) { return lhs.safe_divide(rhs); }

	export [[nodiscard("Use the divided value")]] vec4 safe_divide(const vec4& lhs, const float scalar) { return lhs.safe_divide(scalar); }


} // namespace deckard::math::sse
