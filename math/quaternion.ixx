module;
#include <intrin.h>
export module deckard.math:quaternion;

import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.vec;
import deckard.matrix;
import deckard.math.utils;
import deckard.utils.hash;


import std;

namespace deckard::math
{

	export class quat
	{
	private:
		friend f32  dot(const quat& a, const quat b);
		friend quat cross(const quat& q1, const quat& q2);
		friend quat conjugate(const quat& q);
		friend quat normalize(quat q);
		friend quat slerp(const quat& x, const quat& y, f32 t);
		friend quat mix(const quat& x, const quat& y, f32 t);


	private:
		vec4 data{1.0f, 0.0f, 0.0f, 0.0f};

	public:
		quat() = default;

		quat(f32 w, f32 x, f32 y, f32 z)
		{
			data.w = w;

			data.x = x;
			data.y = y;
			data.z = z;
		}

		quat(f32 w, const vec3& v)
		{
			data.w = w;

			data.x = v.x;
			data.y = v.y;
			data.z = v.z;
		}

		quat(const vec3& v)
		{
			const vec3 half = v * 0.5f;
			vec3       c    = cos(half);
			vec3       s    = sin(half);

			data.w = c.x * c.y * c.z + s.x * s.y * s.z;

			data.x = s.x * c.y * c.z - c.x * s.y * s.z;
			data.y = c.x * s.y * c.z + s.x * c.y * s.z;
			data.z = c.x * c.y * s.z - s.x * s.y * c.z;
		}

		quat operator-() const { return quat(-data.w, -data.x, -data.y, -data.z); }

		quat operator+() const { return *this; }

		auto operator[](size_t index) const -> f32
		{
			assert::check(index < 4, "out-of-bounds, quat has 4 elements");
			return data[index];
		}

		bool equals(const quat& lhs) const { return data == lhs.data; }

		quat operator+(const quat& other) const
		{
			return quat(data.w + other.data.w, data.x + other.data.x, data.y + other.data.y, data.z + other.data.z);
		}

		quat operator-(const quat& other) const
		{
			return quat(data.w - other.data.w, data.x - other.data.x, data.y - other.data.y, data.z - other.data.z);
		}

		quat operator*(const quat& other) const
		{
			const f32 w = data.w * other.data.w - data.x * other.data.x - data.y * other.data.y - data.z * other.data.z;

			const f32 x = data.w * other.data.x + data.x * other.data.w + data.y * other.data.z - data.z * other.data.y;
			const f32 y = data.w * other.data.y + data.y * other.data.w + data.z * other.data.x - data.x * other.data.z;
			const f32 z = data.w * other.data.z + data.z * other.data.w + data.x * other.data.y - data.y * other.data.x;

			return quat(w, x, y, z);
		}

		quat operator*(const f32 scalar) const { return quat(data.w * scalar, data.x * scalar, data.y * scalar, data.z * scalar); }

		quat operator/(const f32 scalar) const
		{
			assert::check(scalar != 0.0f, "Division by zero in quaternion division");
			return quat(data.w / scalar, data.x / scalar, data.y / scalar, data.z / scalar);
		}

		quat& operator+=(const quat& other)
		{
			data.w += other.data.w;

			data.x += other.data.x;
			data.y += other.data.y;
			data.z += other.data.z;

			return *this;
		}

		quat& operator-=(const quat& other)
		{
			data.w -= other.data.w;

			data.x -= other.data.x;
			data.y -= other.data.y;
			data.z -= other.data.z;

			return *this;
		}

		quat& operator*=(const quat& other)
		{
			const quat temp(*this);

			data.w = temp.data.w * other.data.w - temp.data.x * other.data.x - temp.data.y * other.data.y - temp.data.z * other.data.z;

			data.x = temp.data.w * other.data.x + temp.data.x * other.data.w + temp.data.y * other.data.z - temp.data.z * other.data.y;
			data.y = temp.data.w * other.data.y + temp.data.y * other.data.w + temp.data.z * other.data.x - temp.data.x * other.data.z;
			data.z = temp.data.w * other.data.z + temp.data.z * other.data.w + temp.data.x * other.data.y - temp.data.y * other.data.x;

			return *this;
		}

		quat& operator*=(const f32 scalar)
		{
			data.w *= scalar;

			data.x *= scalar;
			data.y *= scalar;
			data.z *= scalar;

			return *this;
		}

		quat& operator/=(const f32 scalar)
		{
			assert::check(scalar != 0.0f, "Division by zero in quaternion division");

			data.w /= scalar;

			data.x /= scalar;
			data.y /= scalar;
			data.z /= scalar;

			return *this;
		}

		quat from_mat4(const mat4& m)
		{
			const f32 m00 = m[0, 0];
			const f32 m11 = m[1, 1];
			const f32 m22 = m[2, 2];
			const f32 sum = m00 + m11 + m22;

			if (sum > 0.0f)
			{
				data.w = std::sqrt(sum + 1.0f) * 0.5f;
				f32 f  = 0.25f / data.w;

				data.x = (m[2, 1] - m[1, 2]) * f;
				data.y = (m[0, 2] - m[2, 0]) * f;
				data.z = (m[1, 0] - m[0, 1]) * f;
			}
			else if ((m00 > m11) and (m00 > m22))
			{
				data.w = std::sqrt(m00 - m11 - m22 + 1.0f) * 0.5f;
				f32 f  = 0.25f / data.x;

				data.x = (m[1, 0] + m[0, 1]) * f;
				data.z = (m[0, 2] + m[2, 0]) * f;
				data.w = (m[2, 1] - m[1, 2]) * f;
			}
			else if (m11 > m22)
			{
				data.y = std::sqrt(m11 - m00 - m22 + 1.0f) * 0.5f;
				f32 f  = 0.25f / data.y;

				data.x = (m[1, 0] + m[0, 1]) * f;
				data.z = (m[2, 1] + m[1, 2]) * f;
				data.w = (m[0, 2] - m[2, 0]) * f;
			}
			else
			{
				data.w = std::sqrt(m22 - m00 - m11 + 1.0f) * 0.5f;
				f32 f  = 0.25f / data.w;

				data.x = (m[0, 2] + m[0, 1]) * f;
				data.y = (m[2, 1] + m[1, 2]) * f;
				data.z = (m[1, 0] - m[0, 1]) * f;
			}

			return *this;
		}

		mat4 to_mat4() const
		{
			const float x2 = data.x * data.x;
			const float y2 = data.y * data.y;
			const float z2 = data.z * data.z;

			const float xz = data.x * data.z;
			const float xy = data.x * data.y;
			const float yz = data.y * data.z;

			const float wx = data.w * data.x;
			const float wy = data.w * data.y;
			const float wz = data.w * data.z;

			return mat4{
			  0,
			  0,
			  0,
			  1,

			  1.0f - 2.0f * (y2 + z2),
			  2.0f * (xy + wz),
			  2.0f * (xz - wy),

			  0,
			  2.0f * (xy - wz),
			  1.0f - 2.0f * (x2 + z2),
			  2.0f * (yz + wx),

			  0,
			  2.0f * (xz + wy),
			  2.0f * (yz - wx),
			  1.0f - 2.0f * (x2 + y2),
			  0};
		}
	};

	export bool operator==(const quat& q1, const quat& q2) { return q1.equals(q2); }

	export quat operator*(f32 scalar, const quat& q) { return q * scalar; }

	export quat operator*=(f32 scalar, quat& q)
	{
		q *= scalar;
		return q;
	}

	export quat operator/(f32 scalar, const quat& q) { return q / scalar; }

	export quat operator/=(f32 scalar, quat& q)
	{
		q /= scalar;
		return q;
	}

	export f32 dot(const quat& a, const quat b)
	{
		vec4 tmp(a.data.w * b.data.w, a.data.x * b.data.x, a.data.y * b.data.y, a.data.z * b.data.z);
		return (tmp.x + tmp.y) + (tmp.z + tmp.w);
	}

	export f32 length(const quat& q) { return std::sqrt(dot(q, q)); };

	export quat conjugate(const quat& q) { return quat(q.data.w, -q.data.x, -q.data.y, -q.data.z); }

	export quat inverse(const quat& q) { return conjugate(q) / dot(q, q); }

	export quat cross(const quat& q1, const quat& q2)
	{
		return {q1.data.w * q2.data.w - q1.data.x * q2.data.x - q1.data.y * q2.data.y - q1.data.z * q2.data.z,
				q1.data.w * q2.data.x + q1.data.x * q2.data.w + q1.data.y * q2.data.z - q1.data.z * q2.data.y,
				q1.data.w * q2.data.y + q1.data.y * q2.data.w + q1.data.z * q2.data.x - q1.data.x * q2.data.z,
				q1.data.w * q2.data.z + q1.data.z * q2.data.w + q1.data.x * q2.data.y - q1.data.y * q2.data.x};
	}

	export quat normalize(quat q)
	{
		f32 len = length(q);

		if (math::is_close_enough_zero(len))
			return q;

		f32 over = 1.0f / len;

		q.data.w *= over;

		q.data.x *= over;
		q.data.y *= over;
		q.data.z *= over;

		return q;
	}

	export quat rotate(const quat& q, const f32 radians, const vec3& v)
	{
		auto tmp = v.normalized();

		f32 s = std::sin(radians * 0.5f);
		return q * quat(std::cos(radians * 0.5f), tmp.x * s, tmp.y * s, tmp.z * s);
	}

	export quat rotate_x(const quat& q, f32 radians)
	{
		const float half = radians * 0.5f;
		return quat(std::cos(half), std::sin(half), 0.0f, 0.0f);
	}

	export quat rotate_y(const quat& q, f32 radians)
	{
		const float half = radians * 0.5f;
		return quat(std::cos(half), 0, std::sin(half), 0);
	}

	export quat rotate_z(const quat& q, f32 radians)
	{
		const float half = radians * 0.5f;
		return quat(std::cos(half), 0.0f, 0.0f, std::sin(half));
	}

	f32 mix(f32 x, f32 y, f32 t) { return (x * (1.0f - t) + y * t); }

	quat mix(const quat& x, const quat& y, f32 t)
	{
		f32 theta = dot(x, y);

		if (theta > (1.0f - 0.000001f))
		{
			return quat(mix(x.data.w, y.data.w, t), mix(x.data.x, y.data.x, t), mix(x.data.y, y.data.y, t), mix(x.data.z, y.data.z, t));
		}

		f32 angle = std::acos(theta);
		return (std::sin((1.0f - t) * angle) * x + std::sin(t * angle) * y) / std::sin(angle);
	}

	export quat lerp(const quat& x, const quat& y, f32 t)
	{
		assert::check(t >= 0.0f);
		assert::check(t <= 1.0f);

		return x * (1.0f - t) + (y * t);
	}

	export quat slerp(const quat& x, const quat& y, f32 t)
	{
		quat z     = y;
		f32  theta = dot(x, y);
		if (theta < 0.0f)
		{
			z     = -y;
			theta = -theta;
		}

		if (theta > (1.0f - 0.000001f))
		{
			return quat(mix(x.data.w, z.data.w, t), mix(x.data.x, z.data.x, t), mix(x.data.y, z.data.y, t), mix(x.data.z, z.data.z, t));
		}


		f32 angle = std::acos(theta);
		return (std::sin((1.0f - t) * angle) * x + std::sin(t * angle) * z) / std::sin(angle);
	}


} // namespace deckard::math

namespace std
{
	template<>
	struct hash<quat>
	{
		size_t operator()(const quat& q) const { return deckard::utils::hash_values(q[3], q[0], q[1], q[1]); }
	};

	template<>
	struct formatter<quat>
	{
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const quat& q, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "quat({:.5f}, {:.5f}, {:.5f}, {:.5f})", q[3], q[0], q[1], q[2]);
		}
	};
} // namespace std
