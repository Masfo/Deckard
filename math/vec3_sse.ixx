module;
#include <xmmintrin.h>


export module deckard.math:vec3_sse;
import deckard.debug;
import deckard.assert;
import :vec2_sse;
import :utils;

namespace deckard::math::sse
{
	using m128 = __m128;

	union vec3data
	{
		struct xyz
		{
			f32 x, y, z, pad1;
		} c;

		f32 element[4]{0.0f};

		m128 reg;
	};

	export struct alignas(16) vec3
	{
		vec3data data;

		vec3()
			: vec3(0.0f)
		{
		}

		vec3(const float* v)
		{
			// mask xyz
			data.reg = _mm_load_ps(v);
		}

		vec3(m128 r) { data.reg = r; }

		vec3(float s) { data.reg = _mm_set_ps(1.0f, s, s, s); }

		vec3(float x, float y)
			: vec3(x, y, 0.0f)
		{
		}

		vec3(float x, float y, float z) { data.reg = _mm_set_ps(1.0f, z, y, x); }

		vec3(const vec2& v, float w) { data.reg = _mm_shuffle_ps(v.data.reg, _mm_set_ps1(w), _MM_SHUFFLE(0, 0, 1, 0)); }

		using vec_type = vec3;

		void operator=(const m128& lhs) { data.reg = lhs; }

		void operator=(const vec_type& lhs) { data.reg = lhs.data.reg; }

		void operator+=(const vec_type& lhs) { data.reg = _mm_add_ps(data.reg, lhs.data.reg); }

		void operator-=(const vec_type& lhs) { data.reg = _mm_sub_ps(data.reg, lhs.data.reg); }

		void operator*=(const vec_type& lhs) { data.reg = _mm_mul_ps(data.reg, lhs.data.reg); }

		void operator/=(const vec_type& lhs) { data.reg = _mm_div_ps(data.reg, lhs.data.reg); }

		vec_type operator+(const vec_type& lhs) const { return _mm_add_ps(data.reg, lhs.data.reg); }

		vec_type operator-(const vec_type& lhs) const { return _mm_sub_ps(data.reg, lhs.data.reg); }

		vec_type operator*(const vec_type& lhs) const { return _mm_mul_ps(data.reg, lhs.data.reg); }

		vec_type operator/(const vec_type& lhs) const { return _mm_div_ps(data.reg, lhs.data.reg); }

		vec_type operator-() { return vec_type(_mm_mul_ps(data.reg, neg_one)); }

		vec_type operator+() const { return *this; }

		void operator>>(float* v) { _mm_store_ps(v, data.reg); }

		void operator<<(float* v)
		{
			// mask xyz
			data.reg = _mm_load_ps(v);
		}

		void operator<<=(float* v) { data.reg = _mm_load_ps(v); }

		explicit operator vec2() const { return _mm_shuffle_ps(data.reg, data.reg, _MM_SHUFFLE(3, 2, 1, 0)); }

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
			m128 a       = _mm_mul_ps(data.reg, data.reg);
			a            = _mm_mul_ps(a, xyzmask);
			auto  rc     = _mm_sqrt_ps(dot(a));
			float result = _mm_cvtss_f32(rc);
			return result;
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

		bool operator==(const vec_type& lhs) const { return is_close_enough(lhs); }

		bool operator!=(const vec_type& lhs) const { return not is_close_enough(lhs); }

		bool equals(const vec_type& lhs) const { return is_close_enough(lhs); }

		bool is_close_enough(const vec_type& lhs, float epsilon = 0.001f) const
		{
#if 1
			auto masked_this = *this * vec_type(xyzmask);
			auto masked_lhs  = lhs * vec_type(xyzmask);

			auto diff   = masked_this - masked_lhs;
			auto result = _mm_cmple_ps(diff.abs().data.reg, _mm_set_ps1(epsilon));

			auto mask = _mm_movemask_ps(result);

			return mask == 0xF;
#else
			for (int i = 0; i < 3; i++)
			{
				if (not math::is_close_enough(lhs[i], (*this)[i], epsilon))
					return false;
			}
			return true;
#endif
		}

		// cross
		vec_type cross(const vec_type& lhs) const
		{
			m128 tmp0 = _mm_shuffle_ps(data.reg, data.reg, _MM_SHUFFLE(3, 0, 2, 1));
			m128 tmp1 = _mm_shuffle_ps(lhs.data.reg, lhs.data.reg, _MM_SHUFFLE(3, 1, 0, 2));
			m128 tmp2 = _mm_mul_ps(tmp0, lhs.data.reg);
			m128 tmp3 = _mm_mul_ps(tmp0, tmp1);
			m128 tmp4 = _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
			return _mm_sub_ps(tmp3, tmp4);
		}

		// dot
		float dot(const vec_type& lhs) const
		{
			m128 masked_reg = _mm_mul_ps(data.reg, xyzmask);
			m128 masked_lhs = _mm_mul_ps(lhs.data.reg, xyzmask);

			m128 mul = _mm_mul_ps(masked_reg, masked_lhs);
			return _mm_cvtss_f32((dot(mul)));
		}

		[[nodiscard("Use the projected vector")]] vec_type project(const vec_type& other) const
		{

			if (other.has_zero())
			{
				dbg::trace("vec3: divide by zero");
				return vec_type(inf_reg);
			}

			auto dot_ab   = dot(other);
			auto b_length = other.length();

			float projection_scalar = dot_ab / (b_length * b_length);
			return other * projection_scalar;
		}

		[[nodiscard("Use the angle value")]] float angle(const vec_type& other) const
		{
			if (other.has_zero())
			{
				dbg::trace("vec3: divide by zero");
				return inf_float;
			}

			float cosTheta = dot(other) / (length() * other.length());


			return std::acos(cosTheta) * 180.0f / std::numbers::pi_v<float>;
		}

		[[nodiscard("Use the rotated vector")]] vec_type rotate(const vec_type& axis, const float rad) const
		{
			const vec_type axis_norm = axis.normalized();
			const vec_type v         = *this;

			float cosTheta         = std::cos(rad);
			float sinTheta         = std::sin(rad);
			float oneMinusCosTheta = 1.0f - cosTheta;

			return (v * cosTheta) + (v.cross(axis) * sinTheta) + (axis * v.dot(axis)) * oneMinusCosTheta;
		}

		// reflect
		vec_type reflect(const vec_type& normal) const { return *this - (normal * dot(normal) * 2); }

		// divide - non panicking
		[[nodiscard("Use the divide vector")]] vec_type safe_divide(const vec_type& other) const
		{
			if (other.has_zero())
			{
				return vec_type(inf_reg);
			}

			return *this / other;
		}

		[[nodiscard("Use the divide scalar")]] vec_type safe_divide(const float scalar) const
		{
			if (scalar == 0.0f)
				return vec_type(inf_reg);

			return *this / vec_type(scalar);
		}

		// has / is
		bool is_invalid() const { return has_zero() or has_inf() or has_nan(); }

		bool has_zero() const
		{
			// auto xyz_masked = _mm_mul_ps(reg, xyzmask);
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
			return mask == 0x7; // 0,0,0,1
		}

		bool is_nan() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, data.reg));
			return mask != 0xF;
		}

		bool is_inf() const
		{
			auto mask = _mm_movemask_ps(_mm_cmpeq_ps(data.reg, inf_reg));
			return mask >= 0x7; // inf,inf,inf,X
		}

		// cmp


		//
		bool operator<=(const vec_type& lhs) const
		{
			auto reg_mask = _mm_mul_ps(data.reg, xyzmask);
			auto lhs_mask = _mm_mul_ps(lhs.data.reg, xyzmask);

			auto mask = _mm_movemask_ps(_mm_cmple_ps(reg_mask, lhs_mask));
			return mask != 0;
		}

		bool operator>=(const vec_type& lhs) const
		{
			auto reg_mask = _mm_mul_ps(data.reg, xyzmask);
			auto lhs_mask = _mm_mul_ps(lhs.data.reg, xyzmask);

			auto mask = _mm_movemask_ps(_mm_cmpge_ps(reg_mask, lhs_mask));
			return mask != 0;
		}

		//
		bool operator>(const vec_type& lhs) const
		{
			auto reg_mask = _mm_mul_ps(data.reg, xyzmask);
			auto lhs_mask = _mm_mul_ps(lhs.data.reg, xyzmask);
			auto mask     = _mm_movemask_ps(_mm_cmpgt_ps(reg_mask, lhs_mask));
			return mask != 0;
		}

		bool operator<(const vec_type& lhs) const
		{
			auto reg_mask = _mm_mul_ps(data.reg, xyzmask);
			auto lhs_mask = _mm_mul_ps(lhs.data.reg, xyzmask);

			auto cmp  = _mm_cmplt_ps(reg_mask, lhs_mask);
			auto mask = _mm_movemask_ps(cmp);
			return mask != 0;
		}

		float operator[](const int index) const
		{
			assert::check(index < 3, "out-of-bounds, vec3 has 2 elements");

			switch (index)
			{
				case 0: return data.c.x;
				case 1: return data.c.y;
				case 2: return data.c.z;
				default:
				{
					dbg::trace();
					dbg::panic("vec3: indexing out-of-bound");
				}
			}
		}

		float& operator[](int index)
		{
			assert::check(index < 3, "out-of-bounds, vec3 has 3 elements");
			return *(reinterpret_cast<float*>(&data.reg) + index);
		}

		inline static float nan_float = std::numeric_limits<float>::quiet_NaN();
		inline static float inf_float = std::numeric_limits<float>::infinity();

		static inline vec_type nan() { return vec_type(nan_float); }

		static inline vec_type inf() { return vec_type(inf_float); }

		static inline vec_type zero() { return vec_type(); }

		inline static m128 xyzmask     = _mm_set_ps(0, 1, 1, 1);
		inline static m128 inf_xyzmask = _mm_set_ps(inf_float, 1, 1, 1);

		inline static m128 neg_zero = _mm_set_ps1(-0.0f);

		inline static m128 one     = _mm_set_ps1(1.0f);
		inline static m128 neg_one = _mm_set_ps1(-1.0f);

		inline static m128 zero_reg = _mm_set_ps1(0.0f);
		inline static m128 inf_reg  = _mm_set_ps1(inf_float);
		inline static m128 nan_reg  = _mm_set_ps1(nan_float);
	};

	vec3 operator*(const vec3& v, float scalar) { return vec3(v.data.c.x * scalar, v.data.c.y * scalar, v.data.c.z * scalar); }

	vec3 operator/(const vec3& v, float scalar)
	{
		scalar = 1.0f / scalar;
		return vec3(v.data.c.x * scalar, v.data.c.y * scalar, v.data.c.z * scalar);
	}

	static_assert(sizeof(vec3) == 16);

	// Free functions
	export [[nodiscard("Use the maximum value")]] vec3 min(const vec3& lhs, const vec3& rhs) { return lhs.min(rhs); }

	export [[nodiscard("Use the maximum vector")]] vec3 max(const vec3& lhs, const vec3& rhs) { return lhs.max(rhs); }

	export [[nodiscard("Use the absolute vector")]] vec3 abs(const vec3& lhs) { return lhs.abs(); }

	export [[nodiscard("Use the distance value")]] float distance(const vec3& lhs, const vec3& rhs) { return lhs.distance(rhs); }

	export [[nodiscard("Use the clamped vector")]] vec3 clamp(const vec3& v, float cmin, float cmax) { return v.clamp(cmin, cmax); }

	export [[nodiscard("Use the clamped value")]] float dot(const vec3& lhs, const vec3& rhs) { return lhs.dot(rhs); }

	export [[nodiscard("Use the length value")]] float length(const vec3& rhs) { return rhs.length(); }

	export void normalize(vec3& rhs) { rhs.normalize(); }

	export [[nodiscard("Use the normalized value")]] auto normalized(const vec3& rhs) { return rhs.normalized(); }

	export [[nodiscard("Use the projected vector")]] vec3 cross(const vec3& lhs, const vec3& rhs) { return lhs.cross(rhs); }

	export [[nodiscard("Use the projected vector")]] vec3 project(const vec3& lhs, const vec3& rhs) { return lhs.project(rhs); }

	export [[nodiscard("Use the angle value")]] float angle(const vec3& lhs, const vec3& rhs) { return lhs.angle(rhs); }

	export [[nodiscard("Use the rotated vector")]] vec3 rotate(const vec3& v, const vec3& axis, float angle)
	{
		return v.rotate(axis, angle);
	}

	export [[nodiscard("Use the reflected vector")]] vec3 reflect(const vec3& dir, const vec3& normal) { return dir.reflect(normal); }

	export [[nodiscard("Use the divided value")]] vec3 safe_divide(const vec3& lhs, const vec3& rhs) { return lhs.safe_divide(rhs); }

	export [[nodiscard("Use the divided value")]] vec3 safe_divide(const vec3& lhs, const float scalar) { return lhs.safe_divide(scalar); }

	export [[nodiscard("Use the cos vec3")]] vec3 cos(const vec3& lhs) { return {std::cos(lhs[0]), std::cos(lhs[1]), std::cos(lhs[2])}; }

	export [[nodiscard("Use the sin vec3")]] vec3 sin(const vec3& lhs) { return {std::sin(lhs[0]), std::sin(lhs[1]), std::sin(lhs[2])}; }


} // namespace deckard::math::sse

export using vec3 = deckard::math::sse::vec3;
static_assert(sizeof(vec3) == 16, "vec3 sse should be 16-bytes");
