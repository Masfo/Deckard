module;
#include <immintrin.h>
export module deckard.math:quaternion;

import deckard.assert;
import deckard.types;
import deckard.debug;

import :vec3_sse;
import :matrix;
import :utils;
import std;

namespace deckard::math::sse
{
	using m128 = __m128;

	union QuatData
	{
		struct xyz
		{
			f32 w, x, y, z;
		} c;

		f32 element[4]{0.0f};

		m128 SSE;
	};

	static_assert(sizeof(QuatData) == 16);

	struct quat
	{
		QuatData data;

		quat()
		{
			data.c.w = 1.0f;
			data.c.x = data.c.y = data.c.z = 0.0f;
		}

		quat(m128 reg) { data.SSE = reg; }

		quat(f32 s, f32 xx, f32 yy, f32 zz)
		{
			data.c.w = s;
			data.c.x = xx;
			data.c.y = yy;
			data.c.z = zz;
		}

		quat(f32 s, const vec3& v)
		{
			data.c.w = s;
			data.c.x = v.data.c.x;
			data.c.y = v.data.c.y;
			data.c.z = v.data.c.z;
		}

		quat(const vec3& v)
		{
			const vec3 half = v * 0.5f;

			vec3 c = cos(half);
			vec3 s = sin(half);

			data.c.w = c[0] * c[1] * c[2] + s[0] * s[1] * s[2];

			data.c.x = s[0] * c[1] * c[2] - c[0] * s[1] * s[2];
			data.c.y = c[0] * s[1] * c[2] + s[0] * c[1] * s[2];
			data.c.z = c[0] * c[1] * s[2] - s[0] * s[1] * c[2];
		}

		// operator vec3&() const { return reinterpret_cast<vec3&>(data.c.x); }
		//
		const vec3& vec3_part() const { return reinterpret_cast<const vec3&>(data.c.x); }

		explicit operator vec3() const { return vec3{data.c.x, data.c.y, data.c.z}; }

		// explicit operator vec4() const { return vec4{data.c.x, data.c.y, data.c.z, data.c.w}; }

		vec4 vec4_part() { return reinterpret_cast<vec4&>(data.c.x); }

		const vec4 vec4_part() const { return reinterpret_cast<const vec4&>(data.c.x); }

		f32 operator[](i32 index) const
		{
			switch (index)
			{
				case 0: return data.c.w;
				case 1: return data.c.x;
				case 2: return data.c.y;
				case 3: return data.c.z;
				default:
				{
					dbg::trace();
					dbg::panic("quat: indexing out-of-bound");
				}
			}
		}

		f32& operator[](i32 index)
		{
			assert::check(index < 4, "out-of-bounds, vec4 has 4 elements");
			return *(reinterpret_cast<f32*>(&data.element[0]) + index);
		}

		quat& operator+=(const quat& q)
		{
			data.c.w += q.data.c.w;
			data.c.x += q.data.c.x;
			data.c.y += q.data.c.y;
			data.c.z += q.data.c.z;
			return *this;
		}

		quat& operator-=(const quat& q)
		{
			data.c.w -= q.data.c.w;
			data.c.x -= q.data.c.x;
			data.c.y -= q.data.c.y;
			data.c.z -= q.data.c.z;
			return *this;
		}

		quat& operator*=(const quat& q)
		{
			const quat temp(*this);

			data.c.w = temp.data.c.w * q.data.c.w - temp.data.c.x * q.data.c.x - temp.data.c.y * q.data.c.y - temp.data.c.z * q.data.c.z;

			data.c.x = temp.data.c.w * q.data.c.x + temp.data.c.x * q.data.c.w + temp.data.c.y * q.data.c.z - temp.data.c.z * q.data.c.y;
			data.c.y = temp.data.c.w * q.data.c.y + temp.data.c.y * q.data.c.w + temp.data.c.z * q.data.c.x - temp.data.c.x * q.data.c.z;
			data.c.z = temp.data.c.w * q.data.c.z + temp.data.c.z * q.data.c.w + temp.data.c.x * q.data.c.y - temp.data.c.y * q.data.c.x;

			return *this;
		}

		quat& operator*=(const f32 s)
		{
			data.c.w *= s;

			data.c.x *= s;
			data.c.y *= s;
			data.c.z *= s;
			return *this;
		}

		quat& operator/=(const f32 s)
		{
			data.c.w /= s;

			data.c.x /= s;
			data.c.y /= s;
			data.c.z /= s;
			return *this;
		}

		inline static m128 neg_one = _mm_set_ps1(-1.0f);

		quat operator-() { return quat(_mm_mul_ps(data.SSE, neg_one)); }

		quat operator+() const { return *this; }

		// quat abs() const { return {std::abs(data.c.w), std::abs(data.c.x), std::abs(data.c.y), std::abs(data.c.z)}; }

		bool equals(const quat& lhs) const { return is_close_enough(lhs); }

		bool is_close_enough(const quat& lhs, f32 epsilon = 0.00001f) const
		{
			m128 diff   = _mm_sub_ps(data.SSE, lhs.data.SSE);
			auto result = _mm_cmple_ps(diff, _mm_set_ps1(epsilon));
			auto mask   = _mm_movemask_ps(result);
			return mask == 0xF;
		}

		void set_rotation_matrix(const mat4& m)
		{
			const f32 m00 = m(0, 0);
			const f32 m11 = m(1, 1);
			const f32 m22 = m(2, 2);
			const f32 sum = m00 + m11 + m22;

			if (sum > 0.0f)
			{
				data.c.w = std::sqrt(sum + 1.0f) * 0.5f;
				f32 f    = 0.25f / data.c.w;

				data.c.x = (m(2, 1) - m(1, 2)) * f;
				data.c.y = (m(0, 2) - m(2, 0)) * f;
				data.c.z = (m(1, 0) - m(0, 1)) * f;
			}
			else if ((m00 > m11) and (m00 > m22))
			{
				data.c.w = std::sqrt(m00 - m11 - m22 + 1.0f) * 0.5f;
				f32 f    = 0.25f / data.c.x;

				data.c.x = (m(1, 0) + m(0, 1)) * f;
				data.c.z = (m(0, 2) + m(2, 0)) * f;
				data.c.w = (m(2, 1) - m(1, 2)) * f;
			}
			else if (m11 > m22)
			{
				data.c.y = std::sqrt(m11 - m00 - m22 + 1.0f) * 0.5f;
				f32 f    = 0.25f / data.c.y;

				data.c.x = (m(1, 0) + m(0, 1)) * f;
				data.c.z = (m(2, 1) + m(1, 2)) * f;
				data.c.w = (m(0, 2) - m(2, 0)) * f;
			}
			else
			{
				data.c.w = std::sqrt(m22 - m00 - m11 + 1.0f) * 0.5f;
				f32 f    = 0.25f / data.c.w;

				data.c.x = (m(0, 2) + m(0, 1)) * f;
				data.c.y = (m(2, 1) + m(1, 2)) * f;
				data.c.z = (m(1, 0) - m(0, 1)) * f;
			}
		}

		mat4 get_rotation_matrix()
		{
			const float x2 = data.c.x * data.c.x;
			const float y2 = data.c.y * data.c.y;
			const float z2 = data.c.z * data.c.z;

			const float xz = data.c.x * data.c.z;
			const float xy = data.c.x * data.c.y;
			const float yz = data.c.y * data.c.z;

			const float wx = data.c.w * data.c.x;
			const float wy = data.c.w * data.c.y;
			const float wz = data.c.w * data.c.z;

			return mat4{
			  0,
			  0,
			  0,
			  1,
			  //
			  1.0f - 2.0f * (y2 + z2),
			  2.0f * (xy + wz),
			  2.0f * (xz - wy),
			  0,
			  //
			  2.0f * (xy - wz),
			  1.0f - 2.0f * (x2 + z2),
			  2.0f * (yz + wx),
			  0,
			  //
			  2.0f * (xz + wy),
			  2.0f * (yz - wx),
			  1.0f - 2.0f * (x2 + y2),
			  0};
		}

		quat conjugate() const { return quat(data.c.w, -data.c.x, -data.c.y, -data.c.z); }
	};

	export bool operator==(const quat& q1, const quat& q2) { return q1.equals(q2); }

	export const quat operator+(const quat& q1, const quat& q2)
	{
		return quat(q1.data.c.w + q2.data.c.w, q1.data.c.x + q2.data.c.x, q1.data.c.y + q2.data.c.y, q1.data.c.z + q2.data.c.z);
	}

	export const quat operator-(const quat& q1, const quat& q2)
	{
		return quat(q1.data.c.w - q2.data.c.w, q1.data.c.x - q2.data.c.x, q1.data.c.y - q2.data.c.y, q1.data.c.z - q2.data.c.z);
	}

	export quat operator*(const quat& q1, const quat& q2)
	{
		const f32 w = q1.data.c.w * q2.data.c.w - q1.data.c.x * q2.data.c.x - q1.data.c.y * q2.data.c.y - q1.data.c.z * q2.data.c.z;
		const f32 x = q1.data.c.w * q2.data.c.x + q1.data.c.x * q2.data.c.w + q1.data.c.y * q2.data.c.z - q1.data.c.z * q2.data.c.y;
		const f32 y = q1.data.c.w * q2.data.c.y + q1.data.c.y * q2.data.c.w + q1.data.c.z * q2.data.c.x - q1.data.c.x * q2.data.c.z;
		const f32 z = q1.data.c.w * q2.data.c.z + q1.data.c.z * q2.data.c.w + q1.data.c.x * q2.data.c.y - q1.data.c.y * q2.data.c.x;

		return quat(w, x, y, z);
	}

	export const quat operator*(const quat& q, float scalar)
	{
		return quat(q.data.c.w * scalar, q.data.c.x * scalar, q.data.c.y * scalar, q.data.c.z * scalar);
	}

	export const quat operator*(f32 scalar, const quat& q) { return q * scalar; }

	export const quat operator/(const quat& q, float scalar)
	{
		return quat(q.data.c.w / scalar, q.data.c.x / scalar, q.data.c.y / scalar, q.data.c.z / scalar);
	}

	export f32 dot(const quat& a, const quat b)
	{
		vec4 tmp(a.data.c.w * b.data.c.w, a.data.c.x * b.data.c.x, a.data.c.y * b.data.c.y, a.data.c.z * b.data.c.z);
		return (tmp.data.c.x + tmp.data.c.y) + (tmp.data.c.z + tmp.data.c.w);
	}

	export f32 length(const quat& q) { return std::sqrt(dot(q, q)); };

	export quat conjugate(const quat& q) { return q.conjugate(); }

	export quat normalize(const quat& q)
	{
		f32 len = length(q);

		if (is_close_enough_zero(len))
			return {};

		f32 over = 1.0f / len;

		return {q.data.c.w * over, q.data.c.x * over, q.data.c.y * over, q.data.c.z * over};
	}

	export quat cross(const quat& q1, const quat& q2)
	{
		return {q1.data.c.w * q2.data.c.w - q1.data.c.x * q2.data.c.x - q1.data.c.y * q2.data.c.y - q1.data.c.z * q2.data.c.z,
				q1.data.c.w * q2.data.c.x + q1.data.c.x * q2.data.c.w + q1.data.c.y * q2.data.c.z - q1.data.c.z * q2.data.c.y,
				q1.data.c.w * q2.data.c.y + q1.data.c.y * q2.data.c.w + q1.data.c.z * q2.data.c.x - q1.data.c.x * q2.data.c.z,
				q1.data.c.w * q2.data.c.z + q1.data.c.z * q2.data.c.w + q1.data.c.x * q2.data.c.y - q1.data.c.y * q2.data.c.x};
	}

	export quat inverse(const quat& q) { return conjugate(q) / dot(q, q); }

	export quat rotate(const quat& q, const f32 angle, const vec3& v)
	{
		auto tmp = v;

		f32 len = tmp.length();
		if (std::abs(len - 1.0f) > 0.001f)
		{
			f32 over = 1.0f / len;
			tmp.data.c.x *= over;
			tmp.data.c.y *= over;
			tmp.data.c.z *= over;
		}

		f32 s = std::sin(angle * 0.5f);
		return q * quat(std::cos(angle * 0.5f), tmp.data.c.x * s, tmp.data.c.y * s, tmp.data.c.z * s);
	}

	export quat rotationX(float angle)
	{
		const float half = angle * 0.5f;
		return quat(std::sin(half), 0, 0, std::cos(half));
	}

	export quat rotationY(float angle)
	{
		const float half = angle * 0.5f;
		return quat(0, std::sin(half), 0, std::cos(half));
	}

	export quat rotationZ(float angle)
	{
		const float half = angle * 0.5f;
		return quat(0, 0, std::sin(half), std::cos(half));
	}
} // namespace deckard::math::sse

export using quat = deckard::math::sse::quat;

static_assert(sizeof(quat) == 16, "quat sse should be 16-bytes");
