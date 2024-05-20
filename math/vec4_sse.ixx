module;
#include <array>
#include <immintrin.h>
#include <limits>
#include <xmmintrin.h>

export module deckard.math:vec4_sse;
import :vec3_sse;
import :utils;

import deckard.assert;
import deckard.debug;
import deckard.assert;

namespace deckard::math::sse
{
	using m128 = __m128;

	export void test()
	{
		m128 a0{7, 14, 15, 6};
		m128 a1{4, 8, 12, 3};
		m128 a2{14, 21, 6, 9};
		m128 a3{13, 7, 6, 4};

		m128 b0{5, 7, 14, 2};
		m128 b1{8, 16, 4, 9};
		m128 b2{13, 6, 8, 4};
		m128 b3{6, 3, 2, 4};

		_MM_TRANSPOSE4_PS(b0, b1, b2, b3);

		b0 = _mm_mul_ps(a0, b0);
		b1 = _mm_mul_ps(a1, b1);
		b2 = _mm_mul_ps(a2, b2);
		b3 = _mm_mul_ps(a3, b3);

		m128 tmp0 = _mm_add_ps(b0, _mm_movehl_ps(b0, b0));
		m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));


		int x = 0;
	}

	/*
	template <int index>
	inline m128 Broadcast(const m128 & a)
	{
		// _mm_cast is for compile only, no opcodes generated
		return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(a), index * 0x55));
	}
	*/

	export struct alignas(16) vec4
	{
		vec4()
			: vec4(0.0f)
		{
		}

		vec4(m128 r)
			: reg(r){};

		vec4(float scalar) noexcept { reg = _mm_set_ps1(scalar); }

		vec4(float x, float y)
			: vec4(x, y, 0.0f, 0.0f)
		{
		}

		vec4(float x, float y, float z)
			: vec4(x, y, z, 0.0)
		{
		}

		vec4(float x, float y, float z, float w) { reg = _mm_setr_ps(x, y, z, w); }

		vec4(const vec3& v, float w)
		{
			reg       = v.reg;
			auto tmp0 = _mm_shuffle_ps(_mm_set_ps1(w), v.reg, _MM_SHUFFLE(3, 2, 0, 0));
			reg       = _mm_shuffle_ps(reg, tmp0, _MM_SHUFFLE(0, 2, 1, 0));
		}

		using vec_type = vec4;

		void operator=(const m128& lhs) noexcept { reg = lhs; }

		void operator=(const vec_type& lhs) noexcept { reg = lhs.reg; }

		void operator+=(const vec_type& lhs) noexcept { reg = _mm_add_ps(reg, lhs.reg); }

		void operator-=(const vec_type& lhs) noexcept { reg = _mm_sub_ps(reg, lhs.reg); }

		void operator*=(const vec_type& lhs) noexcept { reg = _mm_mul_ps(reg, lhs.reg); }

		void operator/=(const vec_type& lhs) noexcept { reg = _mm_div_ps(reg, lhs.reg); }

		void operator+=(const float scalar) noexcept { reg = _mm_add_ps(reg, _mm_set_ps1(scalar)); }

		void operator-=(const float scalar) noexcept { reg = _mm_sub_ps(reg, _mm_set_ps1(scalar)); }

		void operator*=(const float scalar) noexcept { reg = _mm_mul_ps(reg, _mm_set_ps1(scalar)); }

		void operator/=(const float scalar) noexcept { reg = _mm_div_ps(reg, _mm_set_ps1(scalar)); }

		vec_type& operator++() noexcept
		{
			reg = _mm_add_ps(reg, one);
			return *this;
		}

		vec_type operator++(int) noexcept
		{
			m128 tmp = reg;
			reg      = _mm_add_ps(reg, one);
			return vec_type(tmp);
		}

		vec_type& operator--() noexcept
		{
			reg = _mm_sub_ps(reg, one);
			return *this;
		}

		vec_type operator--(int) noexcept
		{
			m128 tmp = reg;
			reg      = _mm_sub_ps(reg, one);
			return vec_type(tmp);
		}

		vec_type operator+(const vec_type& lhs) const noexcept { return vec_type(_mm_add_ps(reg, lhs.reg)); }

		vec_type operator-(const vec_type& lhs) const noexcept { return vec_type(_mm_sub_ps(reg, lhs.reg)); }

		vec_type operator*(const vec_type& lhs) const noexcept { return vec_type(_mm_mul_ps(reg, lhs.reg)); }

		vec_type operator/(const vec_type& lhs) const noexcept { return vec_type(_mm_div_ps(reg, lhs.reg)); }

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

		m128 dot(m128 rhs) const noexcept
		{
			m128 tmp0 = _mm_add_ps(rhs, _mm_movehl_ps(rhs, rhs));
			m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));
			return tmp1;
		}

		float length() const noexcept
		{
			m128 sqr    = _mm_mul_ps(reg, reg);
			m128 result = _mm_sqrt_ps(dot(sqr));

			return _mm_cvtss_f32(result);
		}

		vec_type normalized() const noexcept { return *this / length(); }

		void normalize() noexcept { *this = normalized(); }

		float distance(const vec_type& lhs) const noexcept
		{
			m128 tmp = _mm_sub_ps(reg, lhs.reg);
			m128 sqr = _mm_mul_ps(tmp, tmp);
			return _mm_cvtss_f32(_mm_sqrt_ps(dot(sqr)));
		}

		vec_type clamp(float cmin, float cmax) const noexcept
		{
			m128 tmp0 = _mm_min_ps(_mm_max_ps(reg, _mm_set_ps1(cmin)), _mm_set_ps1(cmax));
			return vec_type(tmp0);
		}

		// cmp
		bool operator==(const vec_type& lhs) const noexcept { return is_close_enough(lhs); }

		bool equals(const vec_type& lhs) const noexcept { return is_close_enough(lhs); }

		bool is_close_enough(const vec_type& lhs, float epsilon = 0.0000001f) const noexcept
		{
			auto diff   = *this - lhs;
			auto result = _mm_cmple_ps(diff.abs().reg, _mm_set_ps1(epsilon));
			auto mask   = _mm_movemask_ps(result);
			return mask == 0xF;
		}

		vec_type cross(const vec_type& lhs) const noexcept
		{
			m128 tmp0 = _mm_shuffle_ps(reg, reg, _MM_SHUFFLE(3, 0, 2, 1));
			m128 tmp1 = _mm_shuffle_ps(lhs.reg, lhs.reg, _MM_SHUFFLE(3, 1, 0, 2));
			m128 tmp2 = _mm_mul_ps(tmp0, lhs.reg);
			m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
			m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
			return _mm_sub_ps(tmp3, tmp4);
		}

		vec_type reflect(const vec_type& normal) const noexcept { return *this - (normal * dot(normal) * 2); }

		// dot
		float dot(const vec_type& lhs) const noexcept
		{
			m128 mul = _mm_mul_ps(reg, lhs.reg);
			return _mm_cvtss_f32((dot(mul)));
		}

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] vec_type safe_divide(const vec_type& other) const noexcept
		{
			if (other.is_invalid())
			{
				return vec_type(inf_reg);
			}

			return *this / other;
		}

		[[nodiscard("Use the divide scalar")]] vec_type safe_divide(const float scalar) const noexcept
		{
			if (scalar == 0.0f)
				return vec_type(inf_reg);

			return *this / vec_type(scalar);
		}

		// has / is
		bool is_invalid() const noexcept { return has_zero() or has_inf() or has_nan(); }

		// has_
		bool has_zero() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero_reg));
			return mask > 0;
		}

		bool has_nan() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, reg));
			return mask != 0xF;
		}

		bool has_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf_reg));
			return mask > 0;
		}

		// is_
		bool is_zero() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, zero_reg));
			return mask == 0xF;
		}

		bool is_nan() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, reg));
			return mask == 0;
		}

		bool is_inf() const noexcept
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(reg, inf_reg));
			return mask == 0xF;
		}

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

		inline static float nan_float = std::numeric_limits<float>::quiet_NaN();
		inline static float inf_float = std::numeric_limits<float>::infinity();

		static inline vec_type nan() noexcept { return vec_type(nan_float); }

		static inline vec_type inf() noexcept { return vec_type(inf_float); }

		static inline vec_type zero() noexcept { return vec_type(); }

		inline static m128 xyzmask  = _mm_set_ps(0, 1, 1, 1);
		inline static m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static m128 one     = _mm_set_ps1(1.0f);
		inline static m128 neg_one = _mm_set_ps1(-1.0f);

		inline static m128 zero_reg = _mm_set_ps1(0.0f);
		inline static m128 inf_reg  = _mm_set_ps1(inf_float);
		inline static m128 nan_reg  = _mm_set_ps1(nan_float);


		m128 reg;
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

	export [[nodiscard("Use the reflected vector")]] vec4 reflect(const vec4& dir, const vec4& normal) { return dir.reflect(normal); }

	export [[nodiscard("Use the divided value")]] vec4 safe_divide(const vec4& lhs, const vec4& rhs) { return lhs.safe_divide(rhs); }

	export [[nodiscard("Use the divided value")]] vec4 safe_divide(const vec4& lhs, const float scalar) { return lhs.safe_divide(scalar); }


} // namespace deckard::math::sse
