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

	export inline constexpr f32 slerp_lerp_threshold = 0.9995f;

	export class quat
	{
	private:
		friend f32  dot(const quat& a, const quat& b);
		friend quat cross(const quat& q1, const quat& q2);
		friend quat conjugate(const quat& q);
		friend quat slerp(const quat& x, const quat& y, f32 t);
		friend quat mix(const quat& x, const quat& y, f32 t);
		friend vec3 operator*(const quat& q, const vec3& v) noexcept;
		friend quat normalize(const quat& q) noexcept;


	private:
		// xyz, w
		vec4 data{0.0f, 0.0f, 0.0f, 1.0f};

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

		quat(const mat4& m) { *this = from_mat4(m); }

		quat operator-() const { return quat(-data.w, -data.x, -data.y, -data.z); }

		quat operator+() const { return *this; }

		auto operator[](size_t index) const -> f32
		{
			assert::check(index < 4, "out-of-bounds, quat has 4 elements");
			return data[index];
		}

		f32 w() const { return data.w; }

		f32 x() const { return data.x; }

		f32 y() const { return data.y; }

		f32 z() const { return data.z; }

		bool equals(const quat& lhs) const
		{
			return math::is_close_enough(data.w, lhs.data.w) and math::is_close_enough(data.x, lhs.data.x) and
				   math::is_close_enough(data.y, lhs.data.y) and math::is_close_enough(data.z, lhs.data.z);
		}

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

		quat operator*(const f32 scalar) const
		{
			return quat(data.w * scalar, data.x * scalar, data.y * scalar, data.z * scalar);
		}

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

			data.w = temp.data.w * other.data.w - temp.data.x * other.data.x - temp.data.y * other.data.y -
					 temp.data.z * other.data.z;

			data.x = temp.data.w * other.data.x + temp.data.x * other.data.w + temp.data.y * other.data.z -
					 temp.data.z * other.data.y;
			data.y = temp.data.w * other.data.y + temp.data.y * other.data.w + temp.data.z * other.data.x -
					 temp.data.x * other.data.z;
			data.z = temp.data.w * other.data.z + temp.data.z * other.data.w + temp.data.x * other.data.y -
					 temp.data.y * other.data.x;

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

		static quat from_mat4(const mat4& m)
		{
			quat result;

			const f32 m00 = m[0, 0];
			const f32 m11 = m[1, 1];
			const f32 m22 = m[2, 2];
			const f32 sum = m00 + m11 + m22;

			if (sum > 0.0f)
			{
				result.data.w = std::sqrt(sum + 1.0f) * 0.5f;
				const f32 f   = 0.25f / result.data.w;

				result.data.x = (m[2, 1] - m[1, 2]) * f;
				result.data.y = (m[0, 2] - m[2, 0]) * f;
				result.data.z = (m[1, 0] - m[0, 1]) * f;
			}
			else if ((m00 > m11) and (m00 > m22))
			{
				result.data.x = std::sqrt(m00 - m11 - m22 + 1.0f) * 0.5f;
				const f32 f   = 0.25f / result.data.x;

				result.data.y = (m[1, 0] + m[0, 1]) * f;
				result.data.z = (m[2, 0] + m[0, 2]) * f;
				result.data.w = (m[2, 1] - m[1, 2]) * f;
			}
			else if (m11 > m22)
			{
				result.data.y = std::sqrt(m11 - m00 - m22 + 1.0f) * 0.5f;
				const f32 f   = 0.25f / result.data.y;

				result.data.x = (m[1, 0] + m[0, 1]) * f;
				result.data.z = (m[2, 1] + m[1, 2]) * f;
				result.data.w = (m[0, 2] - m[2, 0]) * f;
			}
			else
			{
				result.data.z = std::sqrt(m22 - m00 - m11 + 1.0f) * 0.5f;
				const f32 f   = 0.25f / result.data.z;

				result.data.x = (m[2, 0] + m[0, 2]) * f;
				result.data.y = (m[2, 1] + m[1, 2]) * f;
				result.data.w = (m[1, 0] - m[0, 1]) * f;
			}

			return result;
		}

		mat4 to_mat4() const noexcept
		{
			const f32 x2 = data.x * data.x;
			const f32 y2 = data.y * data.y;
			const f32 z2 = data.z * data.z;
			const f32 xz = data.x * data.z;
			const f32 xy = data.x * data.y;
			const f32 yz = data.y * data.z;
			const f32 wx = data.w * data.x;
			const f32 wy = data.w * data.y;
			const f32 wz = data.w * data.z;

			// row 0
			const f32 r00 = 1.0f - 2.0f * (y2 + z2);
			const f32 r01 = 2.0f * (xy + wz);
			const f32 r02 = 2.0f * (xz - wy);

			// row 1
			const f32 r10 = 2.0f * (xy - wz);
			const f32 r11 = 1.0f - 2.0f * (x2 + z2);
			const f32 r12 = 2.0f * (yz + wx);

			// row 2
			const f32 r20 = 2.0f * (xz + wy);
			const f32 r21 = 2.0f * (yz - wx);
			const f32 r22 = 1.0f - 2.0f * (x2 + y2);

			return mat4{r00, r01, r02, 0.0f, r10, r11, r12, 0.0f, r20, r21, r22, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
		}

		std::string to_string() const
		{
			return std::format("quat(w: {:.5f}, x: {:.5f}, y: {:.5f}, z: {:.5f})", data.w, data.x, data.y, data.z);
		}
	};

	export bool operator==(const quat& q1, const quat& q2) { return q1.equals(q2); }

	export f32 dot(const quat& a, const quat& b)
	{
		vec4 tmp(a.data.w * b.data.w, a.data.x * b.data.x, a.data.y * b.data.y, a.data.z * b.data.z);
		return (tmp.x + tmp.y) + (tmp.z + tmp.w);
	}

	export f32 length(const quat& q) { return std::sqrt(dot(q, q)); }

	export quat conjugate(const quat& q) { return quat(q.data.w, -q.data.x, -q.data.y, -q.data.z); }

	export quat inverse(const quat& q)
	{
		const f32 d = dot(q, q);
		assert::check(d > 0.0f, "Cannot invert a quaternion with zero length");

		return conjugate(q) / d;
	}

	export quat operator*(f32 scalar, const quat& q) { return q * scalar; }


	export quat cross(const quat& q1, const quat& q2)
	{
		return {q1.data.w * q2.data.w - q1.data.x * q2.data.x - q1.data.y * q2.data.y - q1.data.z * q2.data.z,
				q1.data.w * q2.data.x + q1.data.x * q2.data.w + q1.data.y * q2.data.z - q1.data.z * q2.data.y,
				q1.data.w * q2.data.y + q1.data.y * q2.data.w + q1.data.z * q2.data.x - q1.data.x * q2.data.z,
				q1.data.w * q2.data.z + q1.data.z * q2.data.w + q1.data.x * q2.data.y - q1.data.y * q2.data.x};
	}

	export [[nodiscard]] quat normalize(const quat& q) noexcept
	{
		const f32 len_sq = q.data.w * q.data.w + q.data.x * q.data.x + q.data.y * q.data.y + q.data.z * q.data.z;

		if (not(len_sq > std::numeric_limits<f32>::epsilon()))
			return quat(1.0f, 0.0f, 0.0f, 0.0f);

		const f32 inv_len = 1.0f / std::sqrt(len_sq);

		return q * inv_len;
	}

	export quat rotate(const quat& q, const f32 radians, const vec3& v)
	{
		auto tmp = v.normalized();

		f32 s = std::sin(radians * 0.5f);
		return q * quat(std::cos(radians * 0.5f), tmp.x * s, tmp.y * s, tmp.z * s);
	}

	export quat rotate_x(const quat& q, f32 radians)
	{

		const float half     = radians * 0.5f;
		quat        rotation = quat(std::cos(half), std::sin(half), 0.0f, 0.0f);
		return q * rotation;
	}

	export quat rotate_y(const quat& q, f32 radians)
	{
		const float half     = radians * 0.5f;
		quat        rotation = quat(std::cos(half), 0.0f, std::sin(half), 0.0f);
		return q * rotation;
	}

	export quat rotate_z(const quat& q, f32 radians)
	{
		const float half     = radians * 0.5f;
		quat        rotation = quat(std::cos(half), 0.0f, 0.0f, std::sin(half));
		return q * rotation;
	}

	export quat mix(const quat& x, const quat& y, f32 t)
	{
		f32 theta = dot(x, y);

		quat to = y;
		if (theta < 0.0f)
		{
			theta = -theta;
			to    = -y;
		}

		if (theta > slerp_lerp_threshold)
		{
			return normalize(quat(
			  mix(x.data.w, to.data.w, t),
			  mix(x.data.x, to.data.x, t),
			  mix(x.data.y, to.data.y, t),
			  mix(x.data.z, to.data.z, t)));
		}

		f32 angle = std::acos(std::clamp(theta, -1.0f, 1.0f));

		return (std::sin((1.0f - t) * angle) * x + std::sin(t * angle) * to) / std::sin(angle);
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

		if (theta > slerp_lerp_threshold)
		{
			return quat(mix(x.data.w, z.data.w, t),
						mix(x.data.x, z.data.x, t),
						mix(x.data.y, z.data.y, t),
						mix(x.data.z, z.data.z, t));
		}


		f32 angle = std::acos(std::clamp(theta, -1.0f, 1.0f));
		return (std::sin((1.0f - t) * angle) * x + std::sin(t * angle) * z) / std::sin(angle);
	}

	export [[nodiscard]] vec3 operator*(const quat& q, const vec3& v) noexcept
	{
		const vec3 qv(q.data.x, q.data.y, q.data.z);
		const vec3 t = 2.0f * cross(qv, v);
		return v + q.data.w * t + cross(qv, t);
	}

	export inline std::ostream& operator<<(std::ostream& os, const quat& s) { return os << s.to_string(); }


} // namespace deckard::math

namespace std
{
	template<>
	struct hash<quat>
	{
		size_t operator()(const quat& q) const { return deckard::utils::hash_values(q[3], q[0], q[1], q[2]); }
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
