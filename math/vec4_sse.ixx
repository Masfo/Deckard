module;
#include <intrin.h>

export module deckard.math:vec4_sse;
import :vec3_sse;
import :utils;
import deckard.assert;
import deckard.debug;

namespace deckard::math::sse
{
	using m128 = __m128;

	union vec4data
	{
		struct xyz
		{
			f32 x, y, z, w;
		} c;

		f32 element[4]{0.0f};

		m128 reg;
	};

	export struct alignas(16) vec4
	{
		vec4data data;

		vec4()
			: vec4(0.0f)
		{
		}

		vec4(m128 r) { data.reg = r; }

		vec4(float scalar) { data.reg = _mm_set_ps1(scalar); }

		vec4(float x, float y)
			: vec4(x, y, 0.0f, 0.0f)
		{
		}

		vec4(float x, float y, float z)
			: vec4(x, y, z, 0.0)
		{
		}

		vec4(float x, float y, float z, float w) { data.reg = _mm_setr_ps(x, y, z, w); }

		vec4(const vec3& v, float w)
		{
			data.reg  = v.data.reg;
			auto tmp0 = _mm_shuffle_ps(_mm_set_ps1(w), v.data.reg, _MM_SHUFFLE(3, 2, 0, 0));
			data.reg  = _mm_shuffle_ps(data.reg, tmp0, _MM_SHUFFLE(0, 2, 1, 0));
		}

		using vec_type = vec4;

		void operator=(const m128& lhs) { data.reg = lhs; }

		void operator=(const vec_type& lhs) { data.reg = lhs.data.reg; }

		void operator+=(const vec_type& lhs) { data.reg = _mm_add_ps(data.reg, lhs.data.reg); }

		void operator-=(const vec_type& lhs) { data.reg = _mm_sub_ps(data.reg, lhs.data.reg); }

		void operator*=(const vec_type& lhs) { data.reg = _mm_mul_ps(data.reg, lhs.data.reg); }

		void operator/=(const vec_type& lhs) { data.reg = _mm_div_ps(data.reg, lhs.data.reg); }

		void operator+=(const float scalar) { data.reg = _mm_add_ps(data.reg, _mm_set_ps1(scalar)); }

		void operator-=(const float scalar) { data.reg = _mm_sub_ps(data.reg, _mm_set_ps1(scalar)); }

		void operator*=(const float scalar) { data.reg = _mm_mul_ps(data.reg, _mm_set_ps1(scalar)); }

		void operator/=(const float scalar) { data.reg = _mm_div_ps(data.reg, _mm_set_ps1(scalar)); }

		vec_type& operator++()
		{
			data.reg = _mm_add_ps(data.reg, one);
			return *this;
		}

		vec_type& operator--()
		{
			data.reg = _mm_sub_ps(data.reg, one);
			return *this;
		}

		vec_type operator++(int) { return vec_type(std::exchange(data.reg, _mm_add_ps(data.reg, one))); }

		vec_type operator--(int) { return vec_type(std::exchange(data.reg, _mm_sub_ps(data.reg, one))); }

		vec_type operator+(const vec_type& lhs) const { return vec_type(_mm_add_ps(data.reg, lhs.data.reg)); }

		vec_type operator-(const vec_type& lhs) const { return vec_type(_mm_sub_ps(data.reg, lhs.data.reg)); }

		vec_type operator*(const vec_type& lhs) const { return vec_type(_mm_mul_ps(data.reg, lhs.data.reg)); }

		vec_type operator/(const vec_type& lhs) const { return vec_type(_mm_div_ps(data.reg, lhs.data.reg)); }

		vec_type operator-() { return vec_type(_mm_mul_ps(data.reg, neg_one)); }

		vec_type operator+() const { return *this; }

		void operator>>(float* v) { _mm_store_ps(v, data.reg); }

		void operator<<(float* v)
		{
			assert::check(v != nullptr);
			data.reg = _mm_load_ps(v);
		}

		void operator<<=(float* v)
		{
			assert::check(v != nullptr);
			data.reg = _mm_load_ps(v);
		}

		explicit operator vec3() const { return _mm_shuffle_ps(data.reg, data.reg, _MM_SHUFFLE(3, 2, 1, 0)); }

		// operations
		vec_type min(const vec_type& lhs) const { return _mm_min_ps(data.reg, lhs.data.reg); }

		vec_type max(const vec_type& lhs) const { return _mm_max_ps(data.reg, lhs.data.reg); }

		vec_type abs() const { return _mm_andnot_ps(neg_zero, data.reg); }

		m128 dot(m128 rhs) const
		{
			m128 tmp0 = _mm_add_ps(rhs, _mm_movehl_ps(rhs, rhs));
			m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));
			return tmp1;
		}

		float length() const
		{
			m128 sqr    = _mm_mul_ps(data.reg, data.reg);
			m128 result = _mm_sqrt_ps(dot(sqr));

			return _mm_cvtss_f32(result);
		}

		vec_type normalized() const { return *this / length(); }

		void normalize() { *this = normalized(); }

		float distance(const vec_type& lhs) const
		{
			m128 tmp = _mm_sub_ps(data.reg, lhs.data.reg);
			m128 sqr = _mm_mul_ps(tmp, tmp);
			return _mm_cvtss_f32(_mm_sqrt_ps(dot(sqr)));
		}

		vec_type clamp(float cmin, float cmax) const
		{
			m128 tmp0 = _mm_min_ps(_mm_max_ps(data.reg, _mm_set_ps1(cmin)), _mm_set_ps1(cmax));
			return vec_type(tmp0);
		}

		// cmp
		bool operator==(const vec_type& lhs) const { return is_close_enough(lhs); }

		bool equals(const vec_type& lhs) const { return is_close_enough(lhs); }

		bool is_close_enough(const vec_type& lhs, float epsilon = 0.00001f) const
		{
			auto diff   = *this - lhs;
			auto result = _mm_cmple_ps(diff.abs().data.reg, _mm_set_ps1(epsilon));
			auto mask   = _mm_movemask_ps(result);
			return mask == 0xF;
		}

		vec_type cross(const vec_type& lhs) const
		{
			m128 tmp0 = _mm_shuffle_ps(data.reg, data.reg, _MM_SHUFFLE(3, 0, 2, 1));
			m128 tmp1 = _mm_shuffle_ps(lhs.data.reg, lhs.data.reg, _MM_SHUFFLE(3, 1, 0, 2));
			m128 tmp2 = _mm_mul_ps(tmp0, lhs.data.reg);
			m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
			m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
			return _mm_sub_ps(tmp3, tmp4);
		}

		vec_type reflect(const vec_type& normal) const { return *this - (normal * dot(normal) * 2); }

		// dot
		float dot(const vec_type& lhs) const
		{
			m128 mul = _mm_mul_ps(data.reg, lhs.data.reg);
			return _mm_cvtss_f32((dot(mul)));
		}

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] vec_type safe_divide(const vec_type& other) const
		{
			if (other.is_invalid())
			{
				return vec_type(inf_reg);
			}

			return *this / other;
		}

		[[nodiscard("Use the divide scalar")]] vec_type safe_divide(const float scalar) const
		{
			if (scalar == 0.0f)
			{
				return vec_type(inf_reg);
			}

			return *this / vec_type(scalar);
		}

		// has / is
		bool is_invalid() const { return has_zero() or has_inf() or has_nan(); }

		// has_
		bool has_zero() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, zero_reg));
			return mask > 0;
		}

		bool has_nan() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, data.reg));
			return mask != 0xF;
		}

		bool has_inf() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, inf_reg));
			return mask > 0;
		}

		// is_
		bool is_zero() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, zero_reg));
			return mask == 0xF;
		}

		bool is_nan() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, data.reg));
			return mask == 0;
		}

		bool is_inf() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, inf_reg));
			return mask == 0xF;
		}

		//
		bool operator<=(const vec_type& lhs) const
		{
			auto mask = _mm_movemask_ps(_mm_cmple_ps(data.reg, lhs.data.reg));
			return mask != 0;
		}

		bool operator>=(const vec_type& lhs) const
		{
			auto mask = _mm_movemask_ps(_mm_cmpge_ps(data.reg, lhs.data.reg));
			return mask != 0;
		}

		//
		bool operator>(const vec_type& lhs) const
		{
			auto mask = _mm_movemask_ps(_mm_cmpgt_ps(data.reg, lhs.data.reg));
			return mask != 0;
		}

		bool operator<(const vec_type& lhs) const
		{
			auto cmp  = _mm_cmplt_ps(data.reg, lhs.data.reg);
			auto mask = _mm_movemask_ps(cmp);
			return mask != 0;
		}

		f32 operator[](i32 index) const
		{
			assert::check(index < 4, "out-of-bounds, vec4 has 4 elements");
			switch (index)
			{
				case 0: return data.c.x;
				case 1: return data.c.y;
				case 2: return data.c.z;
				case 3: return data.c.w;
				default:
				{
					dbg::trace();
					dbg::panic("vec4: indexing out-of-bound");
				}
			}
		}

		f32& operator[](i32 index)
		{
			assert::check(index < 4, "out-of-bounds, vec4 has 4 elements");
			return *(reinterpret_cast<f32*>(&data.reg) + index);
		}

		inline static float nan_float = std::numeric_limits<float>::quiet_NaN();
		inline static float inf_float = std::numeric_limits<float>::infinity();

		static inline vec_type nan() { return vec_type(nan_float); }

		static inline vec_type inf() { return vec_type(inf_float); }

		static inline vec_type zero() { return vec_type(); }

		inline static m128 xyzmask  = _mm_set_ps(0, 1, 1, 1);
		inline static m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static m128 one     = _mm_set_ps1(1.0f);
		inline static m128 neg_one = _mm_set_ps1(-1.0f);

		inline static m128 zero_reg = _mm_set_ps1(0.0f);
		inline static m128 inf_reg  = _mm_set_ps1(inf_float);
		inline static m128 nan_reg  = _mm_set_ps1(nan_float);
	};

	vec4 operator*(const vec4& v, float scalar)
	{
		return vec4(v.data.c.x * scalar, v.data.c.y * scalar, v.data.c.z * scalar, v.data.c.w * scalar);
	}

	vec4 operator/(const vec4& v, float scalar)
	{
		scalar = 1.0f / scalar;
		return vec4(v.data.c.x * scalar, v.data.c.y * scalar, v.data.c.z * scalar, v.data.c.w * scalar);
	}

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

	export [[nodiscard("Use the cos vec4")]] vec4 cos(const vec4& lhs)
	{
		return {std::cos(lhs[0]), std::cos(lhs[1]), std::cos(lhs[2]), std::cos(lhs[3])};
	}

	export [[nodiscard("Use the sin vec4")]] vec4 sin(const vec4& lhs)
	{
		return {std::sin(lhs[0]), std::sin(lhs[1]), std::sin(lhs[2]), std::sin(lhs[3])};
	}


} // namespace deckard::math::sse

export using vec4 = deckard::math::sse::vec4;
static_assert(sizeof(vec4) == 16, "vec4 sse should be 16-bytes");
