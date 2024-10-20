
export module deckard.math:matrix;
import :vec.generic;
import :vec2_sse;
import :vec3_sse;
import :vec4_sse;

import std;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.assert;
import deckard.utils.hash;

namespace deckard::math
{

	class mat4_generic
	{
	private:
		// TODO: vec4
		std::array<f32, 16> data{0.0f};

	public:
		struct fill
		{
		} inline static fill;

		mat4_generic()
			: mat4_generic(1.0f)
		{
		}

		// identity
		mat4_generic(const f32 v) { data[0] = data[5] = data[10] = data[15] = v; }

		mat4_generic(const f32 v, struct fill) { data.fill(v); }

		mat4_generic(f32 f1, f32 f2, f32 f3, f32 f4, f32 f5, f32 f6, f32 f7, f32 f8, f32 f9, f32 f10, f32 f11, f32 f12, f32 f13, f32 f14,
					 f32 f15, f32 f16)
		{
			data[0]  = f1;
			data[1]  = f2;
			data[2]  = f3;
			data[3]  = f4;
			data[4]  = f5;
			data[5]  = f6;
			data[6]  = f7;
			data[7]  = f8;
			data[8]  = f9;
			data[9]  = f10;
			data[10] = f11;
			data[11] = f12;
			data[12] = f13;
			data[13] = f14;
			data[14] = f15;
			data[15] = f16;
		}

		mat4_generic(const std::array<f32, 16>& v) { std::ranges::copy_n(v.begin(), 16, data.begin()); }

		mat4_generic(const f32* v) { std::ranges::copy_n(v, 16, data.begin()); }

		mat4_generic(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3)
		{
			data[0]  = v0[0];
			data[1]  = v0[1];
			data[2]  = v0[2];
			data[3]  = v0[3];
			data[4]  = v1[0];
			data[5]  = v1[1];
			data[6]  = v1[2];
			data[7]  = v1[3];
			data[8]  = v2[0];
			data[9]  = v2[1];
			data[10] = v2[2];
			data[11] = v2[3];
			data[12] = v3[0];
			data[13] = v3[1];
			data[14] = v3[2];
			data[15] = v3[3];
		}

		const f32& operator[](std::size_t i, std::size_t j) const
		{
			assert::check(i < 4 or j < 4, "mat4: indexing out-of-bounds");

			return data[i * 4 + j];
		}

		f32& operator[](std::size_t i, std::size_t j)
		{
			assert::check(i < 4 or j < 4, "mat4: indexing out-of-bounds");
			return data[i * 4 + j];
		}

		f32& operator[](int index)
		{
			assert::check(index < data.size(), "mat4: indexing out-of-bounds");
			return data[index];
		}

		const f32& operator[](u32 index) const
		{
			assert::check(index < data.size(), "mat4_generic: indexing out-of-bounds");
			return data[index];
		}

		const vec4 vec4_index(u32 index) const
		{
			assert::check(index < 4, "mat4_generic: indexing out-of-bounds");

			return vec4(data[index * 4 + 0], data[index * 4 + 1], data[index * 4 + 2], data[index * 4 + 3]);
		}

		mat4_generic operator*(const mat4_generic& rhs)
		{
			std::array<f32, 16> tmp{0.0f};

			for (int col = 0; col < 4; ++col)
			{
				for (int row = 0; row < 4; ++row)
				{
					f32 sum{0.0f};
					for (int i = 0; i < 4; ++i)
						sum += this->operator[](i, row) * rhs[col, i];

					tmp[row + col * 4] = sum;
				}
			}
			return mat4_generic(tmp.data());
		}

		vec4 operator*(const vec4& v) const
		{
			return vec4(data[0] * v[0] + data[4] * v[1] + data[8] * v[2] + data[12] * v[3],
						data[1] * v[0] + data[5] * v[1] + data[9] * v[2] + data[13] * v[3],
						data[2] * v[0] + data[6] * v[1] + data[10] * v[2] + data[14] * v[3],
						data[3] * v[0] + data[7] * v[1] + data[11] * v[2] + data[15] * v[3]);
		}

		mat4_generic operator-() const
		{
			mat4_generic m(*this);

			m = m * mat4_generic(-1.0f);
			return m;
		}

		mat4_generic operator+() const { return *this; }

		bool operator==(const mat4_generic& lhs) const { return is_close_enough(lhs); }

		bool equals(const mat4_generic& lhs) const { return is_close_enough(lhs); }

		bool is_close_enough(const mat4_generic& lhs, const f32 epsilon = 0.00001f) const
		{
			for (u32 i = 0; i < 16; ++i)
			{
				if (not math::is_close_enough(lhs[i], data[i], epsilon))
					return false;
			}
			return true;
		}

		static mat4_generic identity() { return mat4_generic(1.0f); }
	};

	// Free functions


	// multiply mat4_generic by mat4_generic
	export mat4_generic operator*(const mat4_generic& lhs, const mat4_generic& rhs)
	{
		std::array<f32, 16> tmp{0.0f};

		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				f32 sum{0.0f};
				for (int i = 0; i < 4; ++i)
					sum += lhs[i, row] * rhs[col, i];

				tmp[row + col * 4] = sum;
			}
		}
		return mat4_generic(tmp.data());
	}

	export mat4_generic operator+(const mat4_generic& lhs, const mat4_generic& rhs)
	{
		mat4_generic result(1.0f);
		for (int i = 0; i < 16; ++i)
			result[i] = lhs[i] + rhs[i];
		return result;
	}

	export mat4_generic operator-(const mat4_generic& lhs, const mat4_generic& rhs)
	{
		mat4_generic result(1.0f);
		for (int i = 0; i < 16; ++i)
			result[i] = lhs[i] - rhs[i];
		return result;
	}

	export mat4_generic inverse(const mat4_generic& mat);

	export mat4_generic operator/(const mat4_generic& lhs, const mat4_generic& rhs) { return lhs * inverse(rhs); }

	export void operator*=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs * rhs; }

	export void operator/=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs / rhs; }

	export void operator+=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs + rhs; }

	export void operator-=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs - rhs; }

	export mat4_generic inverse(const mat4_generic& mat)
	{


		f32 A2323 = mat[10] * mat[15] - mat[11] * mat[14];
		f32 A1323 = mat[9] * mat[15] - mat[11] * mat[13];
		f32 A1223 = mat[9] * mat[14] - mat[10] * mat[13];
		f32 A0323 = mat[8] * mat[15] - mat[11] * mat[12];
		f32 A0223 = mat[8] * mat[14] - mat[10] * mat[12];
		f32 A0123 = mat[8] * mat[13] - mat[9] * mat[12];
		f32 A2313 = mat[6] * mat[15] - mat[7] * mat[14];
		f32 A1313 = mat[5] * mat[15] - mat[7] * mat[13];
		f32 A1213 = mat[5] * mat[14] - mat[6] * mat[13];
		f32 A2312 = mat[6] * mat[11] - mat[7] * mat[10];
		f32 A1312 = mat[5] * mat[11] - mat[7] * mat[9];
		f32 A1212 = mat[5] * mat[10] - mat[6] * mat[9];
		f32 A0313 = mat[4] * mat[15] - mat[7] * mat[12];
		f32 A0213 = mat[4] * mat[14] - mat[6] * mat[12];
		f32 A0312 = mat[4] * mat[11] - mat[7] * mat[8];
		f32 A0212 = mat[4] * mat[10] - mat[6] * mat[8];
		f32 A0113 = mat[4] * mat[13] - mat[5] * mat[12];
		f32 A0112 = mat[4] * mat[9] - mat[5] * mat[8];

		f32 det =
		  mat[0] * (mat[5] * A2323 - mat[6] * A1323 + mat[7] * A1223) - mat[1] * (mat[4] * A2323 - mat[6] * A0323 + mat[7] * A0223) +
		  mat[2] * (mat[4] * A1323 - mat[5] * A0323 + mat[7] * A0123) - mat[3] * (mat[4] * A1223 - mat[5] * A0223 + mat[6] * A0123);

		det = 1.0f / det;

		return mat4_generic(
		  det * (mat[5] * A2323 - mat[6] * A1323 + mat[7] * A1223),
		  det * -(mat[1] * A2323 - mat[2] * A1323 + mat[3] * A1223),
		  det * (mat[1] * A2313 - mat[2] * A1313 + mat[3] * A1213),
		  det * -(mat[1] * A2312 - mat[2] * A1312 + mat[3] * A1212),
		  det * -(mat[4] * A2323 - mat[6] * A0323 + mat[7] * A0223),
		  det * (mat[0] * A2323 - mat[2] * A0323 + mat[3] * A0223),
		  det * -(mat[0] * A2313 - mat[2] * A0313 + mat[3] * A0213),
		  det * (mat[0] * A2312 - mat[2] * A0312 + mat[3] * A0212),
		  det * (mat[4] * A1323 - mat[5] * A0323 + mat[7] * A0123),
		  det * -(mat[0] * A1323 - mat[1] * A0323 + mat[3] * A0123),
		  det * (mat[0] * A1313 - mat[1] * A0313 + mat[3] * A0113),
		  det * -(mat[0] * A1312 - mat[1] * A0312 + mat[3] * A0112),
		  det * -(mat[4] * A1223 - mat[5] * A0223 + mat[6] * A0123),
		  det * (mat[0] * A1223 - mat[1] * A0223 + mat[2] * A0123),
		  det * -(mat[0] * A1213 - mat[1] * A0213 + mat[2] * A0113),
		  det * (mat[0] * A1212 - mat[1] * A0212 + mat[2] * A0112));
	}

	export mat4_generic transpose(const mat4_generic& mat)
	{
		return mat4_generic(
		  vec4(mat[0, 0], mat[1, 0], mat[2, 0], mat[3, 0]),
		  vec4(mat[0, 1], mat[1, 1], mat[2, 1], mat[3, 1]),
		  vec4(mat[0, 2], mat[1, 2], mat[2, 2], mat[3, 2]),
		  vec4(mat[0, 3], mat[1, 3], mat[2, 3], mat[3, 3]));
	}

	export mat4_generic lookat_rh(const vec3& eye, const vec3& center, const vec3& up)
	{
		//
		vec3 f = normalized(center - eye);
		vec3 s = normalized(cross(f, up));
		vec3 u = cross(s, f);

		return mat4_generic(
		  vec4(s[0], u[0], -f[0], 0.0f), //
		  vec4(s[1], u[1], -f[1], 0.0f), //
		  vec4(s[2], u[2], -f[2], 0.0f), //
		  vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f));
	}

	export mat4_generic scale(const mat4_generic& mat, const vec3& scale)
	{
		return mat4_generic(
		  vec4(mat[0, 0] * scale[0], mat[0, 1] * scale[0], mat[0, 2] * scale[0], mat[0, 3] * scale[0]),
		  vec4(mat[1, 0] * scale[1], mat[1, 1] * scale[1], mat[1, 2] * scale[1], mat[1, 3] * scale[1]),
		  vec4(mat[2, 0] * scale[2], mat[2, 1] * scale[2], mat[2, 2] * scale[2], mat[2, 3] * scale[2]),
		  vec4(mat[3, 0], mat[3, 1], mat[3, 2], mat[3, 3]));
	}

	export mat4_generic translate(const mat4_generic& mat, const vec3& translate)
	{

		vec4 v(mat[0, 0], mat[0, 1], mat[0, 2], 1);

		v *= translate[0];
		v += vec4(mat[1, 0], mat[1, 1], mat[1, 2], mat[1, 3]) * translate[1];
		v += vec4(mat[2, 0], mat[2, 1], mat[2, 2], mat[2, 3]) * translate[2];
		v += vec4(mat[3, 0], mat[3, 1], mat[3, 2], mat[3, 3]);


		return mat4_generic(
		  vec4(mat[0, 0], mat[0, 1], mat[0, 2], mat[0, 3]),
		  vec4(mat[1, 0], mat[1, 1], mat[1, 2], mat[1, 3]),
		  vec4(mat[2, 0], mat[2, 1], mat[2, 2], mat[2, 3]),
		  v);
	}

	export mat4_generic perspective(f32 fov, f32 aspect, f32 near, f32 far)
	{
		f32 const tanHalfFovy = std::tan(fov / 2.0f);

		return mat4_generic(
		  vec4(1.0f / (aspect * tanHalfFovy), 0.0f, 0.0f, 0.0f),
		  vec4(0.0f, 1.0f / tanHalfFovy, 0.0f, 0.0f),
		  vec4(0.0f, 0.0f, -(far + near) / (far - near), -1.0f),
		  vec4(0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f));
	}

	export mat4_generic ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
	{
		return mat4_generic(
		  vec4(2 / (right - left), 0.0f, 0.0f, 0.0f),
		  vec4(0.0f, 2 / (top - bottom), 0.0f, 0.0f),
		  vec4(0.0f, 0.0f, -(2 / (far - near)), 0.0f),
		  vec4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f));
	}

	export mat4_generic rotate(const mat4_generic& m, f32 radians, const vec3& v)
	{
		f32 a = radians;
		f32 c = std::cos(a);
		f32 s = std::sin(a);

		vec3 axis(v.normalized());
		vec3 temp(axis * (1.0f - c));

		const mat4_generic Rotate(
		  vec4(c + temp[0] * axis[0], temp[0] * axis[1] + s * axis[2], temp[0] * axis[2] - s * axis[1], 0.0f),
		  vec4(0 + temp[1] * axis[0] - s * axis[2], c + temp[1] * axis[1], temp[1] * axis[2] + s * axis[0], 0.0f),
		  vec4(0 + temp[2] * axis[0] + s * axis[1], temp[2] * axis[1] - s * axis[0], c + temp[2] * axis[2], 0.0f),
		  vec4(0.0f, 0.0f, 0.0f, 1.0f));

		return m * Rotate;
	}

	export vec3 project(const vec3& obj, const mat4_generic model, const mat4_generic& proj, const vec4& viewport)
	{
		vec4 temp(obj, 1.0f);
		temp = model * temp;
		temp = proj * temp;

		temp /= temp[3];

		temp    = temp * 0.5f + 0.5f;
		temp[0] = temp[0] * viewport[2] + viewport[0];
		temp[1] = temp[1] * viewport[3] + viewport[1];

		return vec3{temp};
	}

	export vec3 unproject(const vec3& win, const mat4_generic& model, const mat4_generic& proj, const vec4& viewport)
	{
		mat4_generic inv = inverse(proj * model);

		vec4 temp(win, 1.0f);

		temp[0] = (temp[0] - viewport[0]) / viewport[2];
		temp[1] = (temp[1] - viewport[1]) / viewport[3];

		temp = temp * 2.0f - 1.0f;

		vec4 obj = inv * temp;
		obj /= obj[3];

		return vec3{obj};
	}

	export mat4_generic frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
	{
		//
		mat4_generic ret{0.0f};
		ret[0] = (2.0f * near) / (right - left);
		ret[5] = (2.0f * near) / (top - bottom);

		ret[8]  = (right + left) / (right - left);
		ret[9]  = (top + bottom) / (top - bottom);
		ret[10] = -(far + near) / (far - near);
		ret[11] = -1.0f;

		ret[14] = -(2.0f * far * near) / (far - near);

		return ret;
	}

	export f32 determinent(const mat4_generic& m)
	{

		f32  SubFactor00 = m[10] * m[15] - m[14] * m[11];
		f32  SubFactor01 = m[9] * m[15] - m[13] * m[11];
		f32  SubFactor02 = m[9] * m[14] - m[13] * m[10];
		f32  SubFactor03 = m[8] * m[15] - m[12] * m[11];
		f32  SubFactor04 = m[8] * m[14] - m[12] * m[10];
		f32  SubFactor05 = m[8] * m[13] - m[12] * m[9];
		vec4 DetCof(+(m[5] * SubFactor00 - m[6] * SubFactor01 + m[7] * SubFactor02),
					-(m[4] * SubFactor00 - m[6] * SubFactor03 + m[7] * SubFactor04),
					+(m[4] * SubFactor01 - m[5] * SubFactor03 + m[7] * SubFactor05),
					-(m[4] * SubFactor02 - m[5] * SubFactor04 + m[6] * SubFactor05));
		return m[0] * DetCof[0] + m[1] * DetCof[1] + m[2] * DetCof[2] + m[3] * DetCof[3];
	}

	// TODO:	project_world_to_screen
	//			project_screen_to_world
	//
	// https://vallentin.dev/blog/post/cgmath-screen-to-world
	//
	// struct Ray
	// {
	//		start : Point3<f32>,
	//      direction : Vector3<f32>,
	// }
	//
	//	let front = project_screen_to_world(Vector3::new(mx, my, 1.0), projection, viewport);
	//	let back = project_screen_to_world(Vector3::new (mx, my, 0.0), projection, viewport);
	//
	// if let (Some(front), Some(back)) = (front, back)
	// {
	//		let ray = Ray{
	//			start : Point3::from_vec(back),
	//			direction : (front - back).normalize(),
	//		};
	// }

	// rotate around unit vector
} // namespace deckard::math

export using mat4 = deckard::math::mat4_generic;
