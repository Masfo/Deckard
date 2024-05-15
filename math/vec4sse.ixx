module;
#include <cmath>
#include <emmintrin.h>
#include <xmmintrin.h>
export module deckard.math.vec4sse;

namespace deckard::math::sse
{
	class zero;

	export struct alignas(16) vec4
	{
		vec4()
			: vec4(0.0f)
		{
		}

		vec4(__m128 r)
			: reg(r){};

		vec4& operator=(const __m128& lhs) noexcept
		{
			reg = lhs;
			return *this;
		}

		vec4& operator=(const vec4& lhs) noexcept
		{
			reg = lhs.reg;
			return *this;
		}

		operator __m128() const { return reg; }

		vec4(float x, float y, float z, float w) { reg = _mm_set_ps(w, z, y, x); }

		vec4(float scalar) noexcept { reg = _mm_set_ps1(scalar); }

		vec4& operator+=(const vec4& lhs) noexcept
		{
			reg = _mm_add_ps(reg, lhs.reg);
			return *this;
		}

		vec4 operator+(const vec4& lhs) const noexcept
		{
			auto result = *this;
			result += lhs;
			return result;
		}

		vec4& operator-=(const vec4& lhs) noexcept
		{
			reg = _mm_sub_ps(reg, lhs.reg);
			return *this;
		}

		vec4 operator-(const vec4& lhs) const noexcept
		{
			auto result = *this;
			result -= lhs;
			return result;
		}

		vec4& operator*=(const vec4& lhs) noexcept
		{
			reg = _mm_mul_ps(reg, lhs.reg);
			return *this;
		}

		vec4 operator*(const vec4& lhs) const noexcept
		{
			auto result = *this;
			result *= lhs;
			return result;
		}

		vec4& operator/=(const vec4& lhs) noexcept
		{
			reg = _mm_div_ps(reg, lhs.reg);
			return *this;
		}

		vec4 operator/(const vec4& lhs) const noexcept
		{
			auto result = *this;
			result /= lhs;
			return result;
		}

		vec4& operator-() noexcept
		{
			*this *= vec4(neg_one);
			return *this;
		}

		vec4 min(const vec4& lhs) const noexcept { return _mm_min_ps(reg, lhs.reg); }

		vec4 max(const vec4& lhs) const noexcept { return _mm_max_ps(reg, lhs.reg); }

		vec4 abs() const noexcept { return _mm_andnot_ps(neg_zero, reg); }

		float horizontal_add(const vec4& lhs) const noexcept
		{
			__m128 t1 = _mm_movehl_ps(lhs.reg, lhs.reg);
			__m128 t2 = _mm_add_ps(lhs.reg, t1);
			__m128 t3 = _mm_shuffle_ps(t2, t2, 1);
			__m128 t4 = _mm_add_ss(t2, t3);
			return _mm_cvtss_f32(t4);
		};

		float length() const noexcept
		{
			__m128 a      = _mm_mul_ps(reg, reg);
			auto   rc     = _mm_sqrt_ps(_mm_set_ps1(horizontal_add(a)));
			float  result = _mm_cvtss_f32(rc);
			return result;
		}

		vec4 normalized() const noexcept { return *this / length(); }

		void normalize() noexcept { *this = normalized(); }

		float distance(const vec4& lhs) const noexcept
		{
			vec4 result;
			result += *this - lhs;
			return horizontal_add(result.abs());
		}

		vec4 clamp(const vec4& lhs) const noexcept
		{
			//

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
		bool operator==(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, lhs.reg));
			return mask == 0xF;
		}

		//
		bool operator<=(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmple_ps(reg, lhs.reg));
			return mask == 0xF;
		}

		bool operator>=(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpge_ps(reg, lhs.reg));
			return mask == 0xF;
		}

		//
		bool operator>(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpgt_ps(reg, lhs.reg));
			return mask == 0xF;
		}

		bool operator<(const vec4& lhs) const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmplt_ps(reg, lhs.reg));
			return mask == 0xF;
		}

		inline static __m128 zero     = _mm_set_ps1(0.0f);
		inline static __m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static __m128 one     = _mm_set_ps1(1.0f);
		inline static __m128 neg_one = _mm_set_ps1(-1.0f);

		inline static __m128 inf      = _mm_div_ps(one, zero);
		inline static __m128 epsilon  = _mm_set_ps1(0.00000001f);
		inline static __m128 epsilon2 = _mm_set_ps1(2 * 0.00000001f);


		__m128 reg;
	};

	static_assert(sizeof(vec4) == 16);

} // namespace deckard::math::sse
