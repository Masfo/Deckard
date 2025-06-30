
export module deckard.matrix;

import std;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.assert;
import deckard.utils.hash;
import deckard.vec;
import deckard.math.utils;

namespace deckard::math
{

	struct mat4_generic
	{

		using type = vec4;
		std::array<type, 4> mat;

		struct fill
		{
		} inline static fill;

		// identity


		mat4_generic()
			: mat4_generic(1.0f)
		{
		}

		explicit mat4_generic(f32 scalar) // identity
		{
			mat[0].x = scalar;
			mat[1].y = scalar;
			mat[2].z = scalar;
			mat[3].w = scalar;
		}

		mat4_generic(const f32 v, struct fill)
		{
			mat[0] = {v, v, v, v};
			mat[1] = {v, v, v, v};
			mat[2] = {v, v, v, v};
			mat[3] = {v, v, v, v};
		}

		mat4_generic(f32 e00, f32 e01, f32 e02, f32 e03, f32 e04, f32 e05, f32 e06, f32 e07, f32 e08, f32 e09, f32 e10, f32 e11, f32 e12,
					 f32 e13, f32 e14, f32 e15)
		{
			mat[0] = {e00, e01, e02, e03};
			mat[1] = {e04, e05, e06, e07};
			mat[2] = {e08, e09, e10, e11};
			mat[3] = {e12, e13, e14, e15};
		}

		mat4_generic(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3)
		{
			mat[0] = v0;
			mat[1] = v1;
			mat[2] = v2;
			mat[3] = v3;
		}

		vec4& operator[](size_t index)
		{
			assert::check(index < mat.size(), "mat4: indexing out-of-bounds");
			return mat[index];
		}

		const vec4& operator[](size_t index) const
		{
			assert::check(index < mat.size(), "mat4: indexing out-of-bounds");
			return mat[index];
		}

		mat4_generic operator*(const f32 scalar) const
		{
			return mat4_generic(mat[0] * scalar, mat[1] * scalar, mat[2] * scalar, mat[3] * scalar);
		}

		mat4_generic operator*(const mat4_generic& rhs) const
		{
			mat4_generic result;

			result[0] = mat[0] * rhs[0].x + mat[1] * rhs[0].y + mat[2] * rhs[0].z + mat[3] * rhs[0].w;
			result[1] = mat[0] * rhs[1].x + mat[1] * rhs[1].y + mat[2] * rhs[1].z + mat[3] * rhs[1].w;
			result[2] = mat[0] * rhs[2].x + mat[1] * rhs[2].y + mat[2] * rhs[2].z + mat[3] * rhs[2].w;
			result[3] = mat[0] * rhs[3].x + mat[1] * rhs[3].y + mat[2] * rhs[3].z + mat[3] * rhs[3].w;

			return result;
		}

		void operator*=(const mat4_generic& rhs) { *this = *this * rhs; }

		mat4_generic operator+(const mat4_generic& rhs) const
		{
			mat4_generic result;

			result[0] = mat[0] + rhs[0];
			result[1] = mat[1] + rhs[1];
			result[2] = mat[2] + rhs[2];
			result[3] = mat[3] + rhs[3];

			return result;
		}

		mat4_generic operator-(const mat4_generic& rhs) const
		{
			mat4_generic result;

			result[0] = mat[0] - rhs[0];
			result[1] = mat[1] - rhs[1];
			result[2] = mat[2] - rhs[2];
			result[3] = mat[3] - rhs[3];

			return result;
		}

		mat4_generic operator-() const
		{
			mat4_generic m(*this);

			m *= mat4_generic(-1.0f);
			return m;
		}

		mat4_generic operator+() const { return *this; }

		mat4_generic operator/(const f32 scalar) const
		{
			if (math::is_close_enough_zero(scalar))
				dbg::panic("divide by zero: {} / {}", *this, scalar);

			mat4_generic result;

			result[0] = mat[0] / scalar;
			result[1] = mat[1] / scalar;
			result[2] = mat[2] / scalar;
			result[3] = mat[3] / scalar;

			return result;
		}

		bool operator==(const mat4_generic& lhs) const { return is_close_enough(lhs); }

		bool equals(const mat4_generic& lhs) const { return is_close_enough(lhs); }

		bool is_close_enough(const mat4_generic& lhs) const
		{
			return mat[0] == lhs[0] and mat[1] == lhs[1] and mat[2] == lhs[2] and mat[3] == lhs[3];
		}

		mat4_generic scale(const vec3& scale) const
		{
			const vec4 XS(scale.x);
			const vec4 YS(scale.y);
			const vec4 ZS(scale.z);

			return mat4_generic(mat[0] * XS, mat[1] * YS, mat[2] * ZS, mat[3]);
		}

		mat4_generic rotate(f32 radians, const vec3& v) const
		{
			const f32 a = radians;
			const f32 c = std::cos(a);
			const f32 s = std::sin(a);

			vec3 axis(v.normalized());
			vec3 temp(axis * (1.0f - c));

			mat4_generic rotate;
			rotate[0].x = c + temp.x * axis.x;
			rotate[0].y = temp.x * axis.y + s * axis.z;
			rotate[0].z = temp.x * axis.z - s * axis.y;

			rotate[1].x = temp.y * axis.x - s * axis.z;
			rotate[1].y = c + temp.y * axis.y;
			rotate[1].z = temp.y * axis.z + s * axis.x;

			rotate[2].x = temp.z * axis.x + s * axis.y;
			rotate[2].y = temp.z * axis.y - s * axis.x;
			rotate[2].z = c + temp.z * axis.z;

			mat4_generic result;

			result[0] = mat[0] * rotate[0].x + mat[1] * rotate[0].y + mat[2] * rotate[0].z;
			result[1] = mat[0] * rotate[1].x + mat[1] * rotate[1].y + mat[2] * rotate[1].z;
			result[2] = mat[0] * rotate[2].x + mat[1] * rotate[2].y + mat[2] * rotate[2].z;
			result[3] = mat[3];

			return result;
		}

		mat4_generic translate(const vec3& translate) const
		{
			mat4_generic result(*this);
			result[3] = mat[0] * translate.x + mat[1] * translate.y + mat[2] * translate.z + mat[3];
			return result;
		}

		f32 determinant() const
		{

			f32 SubFactor00 = mat[2].z * mat[3].w - mat[3].z * mat[2].w;
			f32 SubFactor01 = mat[2].y * mat[3].w - mat[3].y * mat[2].w;
			f32 SubFactor02 = mat[2].y * mat[3].w - mat[3].y * mat[2].z;
			f32 SubFactor03 = mat[2].x * mat[3].w - mat[3].x * mat[2].w;
			f32 SubFactor04 = mat[2].x * mat[3].z - mat[3].x * mat[2].z;
			f32 SubFactor05 = mat[2].x * mat[3].y - mat[3].x * mat[2].y;

			vec4 DetCof(+(mat[1].y * SubFactor00 - mat[1].z * SubFactor01 + mat[1].w * SubFactor02),
						-(mat[1].x * SubFactor00 - mat[1].z * SubFactor03 + mat[1].w * SubFactor04),
						+(mat[1].x * SubFactor01 - mat[1].y * SubFactor03 + mat[1].w * SubFactor05),
						-(mat[1].x * SubFactor02 - mat[1].y * SubFactor04 + mat[1].z * SubFactor05));

			return mat[0].x * DetCof.x + mat[0].y * DetCof.y + mat[0].z * DetCof.z + mat[0].w * DetCof.w;
		}

		mat4_generic inverse() const
		{
			f32 Coef00 = mat[2].z * mat[3].w - mat[3].z * mat[2].w;
			f32 Coef02 = mat[1].z * mat[3].w - mat[3].z * mat[1].w;
			f32 Coef03 = mat[1].z * mat[2].w - mat[2].z * mat[1].w;

			f32 Coef04 = mat[2].y * mat[3].w - mat[3].y * mat[2].w;
			f32 Coef06 = mat[1].y * mat[3].w - mat[3].y * mat[1].w;
			f32 Coef07 = mat[1].y * mat[2].w - mat[2].y * mat[1].w;

			f32 Coef08 = mat[2].y * mat[3].z - mat[3].y * mat[2].z;
			f32 Coef10 = mat[1].y * mat[3].z - mat[3].y * mat[1].z;
			f32 Coef11 = mat[1].y * mat[2].z - mat[2].y * mat[1].z;

			f32 Coef12 = mat[2].x * mat[3].w - mat[3].x * mat[2].w;
			f32 Coef14 = mat[1].x * mat[3].w - mat[3].x * mat[1].w;
			f32 Coef15 = mat[1].x * mat[2].w - mat[2].x * mat[1].w;

			f32 Coef16 = mat[2].x * mat[3].z - mat[3].x * mat[2].z;
			f32 Coef18 = mat[1].x * mat[3].z - mat[3].x * mat[1].z;
			f32 Coef19 = mat[1].x * mat[2].z - mat[2].x * mat[1].z;

			f32 Coef20 = mat[2].x * mat[3].y - mat[3].x * mat[2].y;
			f32 Coef22 = mat[1].x * mat[3].y - mat[3].x * mat[1].y;
			f32 Coef23 = mat[1].x * mat[2].y - mat[2].x * mat[1].y;

			vec4 Fac0(Coef00, Coef00, Coef02, Coef03);
			vec4 Fac1(Coef04, Coef04, Coef06, Coef07);
			vec4 Fac2(Coef08, Coef08, Coef10, Coef11);
			vec4 Fac3(Coef12, Coef12, Coef14, Coef15);
			vec4 Fac4(Coef16, Coef16, Coef18, Coef19);
			vec4 Fac5(Coef20, Coef20, Coef22, Coef23);

			vec4 Vec0(mat[1].x, mat[0].x, mat[0].x, mat[0].x);
			vec4 Vec1(mat[1].y, mat[0].y, mat[0].y, mat[0].y);
			vec4 Vec2(mat[1].z, mat[0].z, mat[0].z, mat[0].z);
			vec4 Vec3(mat[1].w, mat[0].w, mat[0].w, mat[0].w);

			vec4 Inv0(Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2);
			vec4 Inv1(Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4);
			vec4 Inv2(Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5);
			vec4 Inv3(Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5);

			vec4 SignA(+1, -1, +1, -1);
			vec4 SignB(-1, +1, -1, +1);

			mat4_generic Inverse(Inv0 * SignA, Inv1 * SignB, Inv2 * SignA, Inv3 * SignB);

			vec4 Row0(Inverse[0].x, Inverse[1].x, Inverse[2].x, Inverse[3].x);

			vec4 Dot0(mat[0] * Row0);
			f32  Dot1 = (Dot0.x + Dot0.y) + (Dot0.z + Dot0.w);

			f32 OneOverDeterminant = 1.0f / Dot1;

			return Inverse * OneOverDeterminant;
		}

		static mat4_generic identity() { return mat4_generic(1.0f); }
	};

	export void operator*=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs * rhs; }

	export void operator/=(mat4_generic& lhs, const f32 rhs) { lhs = lhs / rhs; }

	export void operator+=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs + rhs; }

	export void operator-=(mat4_generic& lhs, const mat4_generic& rhs) { lhs = lhs - rhs; }

	export vec4 operator*(const vec4& lhs, const mat4_generic& rhs)
	{
		return vec4(rhs[0].x * lhs.x + rhs[0].y * lhs.y + rhs[0].z * lhs.z + rhs[0].w * lhs.w,
					rhs[1].x * lhs.x + rhs[1].y * lhs.y + rhs[1].z * lhs.z + rhs[1].w * lhs.w,
					rhs[2].x * lhs.x + rhs[2].y * lhs.y + rhs[2].z * lhs.z + rhs[2].w * lhs.w,
					rhs[3].x * lhs.x + rhs[3].y * lhs.y + rhs[3].z * lhs.z + rhs[3].w * lhs.w);
	}

	export vec4 operator*(const mat4_generic& lhs, const vec4& rhs)
	{
		vec4 const Mov0(rhs.x);
		vec4 const Mov1(rhs.y);
		vec4 const Mul0 = lhs[0] * Mov0;
		vec4 const Mul1 = lhs[1] * Mov1;
		vec4 const Add0 = Mul0 + Mul1;
		vec4 const Mov2(rhs.z);
		vec4 const Mov3(rhs.w);
		vec4 const Mul2 = lhs[2] * Mov2;
		vec4 const Mul3 = lhs[3] * Mov3;
		vec4 const Add1 = Mul2 + Mul3;
		vec4 const Add2 = Add0 + Add1;
		return Add2;
	}

	export mat4_generic transpose(const mat4_generic& mat)
	{
		const vec4 col0(mat[0].x, mat[1].x, mat[2].x, mat[3].x);
		const vec4 col1(mat[0].y, mat[1].y, mat[2].y, mat[3].y);
		const vec4 col2(mat[0].z, mat[1].z, mat[2].z, mat[3].z);
		const vec4 col3(mat[0].w, mat[1].w, mat[2].w, mat[3].w);

		return mat4_generic(col0, col1, col2, col3);
	}

	export mat4_generic scale(const mat4_generic& mat, const vec3& scale) { return mat.scale(scale); }

	export mat4_generic translate(const mat4_generic& mat, const vec3& translate) { return mat.translate(translate); }

	export mat4_generic rotate(const mat4_generic& m, f32 radians, const vec3& v) { return m.rotate(radians, v); }

	export mat4_generic lookat_rh(const vec3& eye, const vec3& center, const vec3& up)
	{
		//
		vec3 f = normalized(center - eye);
		vec3 s = normalized(cross(f, up));
		vec3 u = cross(s, f);

		return mat4_generic(
		  vec4(s.x, u.x, -f.x, 0.0f), //
		  vec4(s.y, u.y, -f.y, 0.0f), //
		  vec4(s.z, u.z, -f.z, 0.0f), //
		  vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f));
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

	export mat4_generic inverse(const mat4_generic& mat) { return mat.inverse(); }

	export vec3 project(const vec3& obj, const mat4_generic model, const mat4_generic& proj, const vec4& viewport)
	{
		vec4 tmp(obj, 1.0f);
		tmp = model * tmp;
		tmp = proj * tmp;

		tmp /= tmp.w;
		tmp   = tmp * 0.5f + 0.5f;
		tmp.x = tmp.x * viewport.z + viewport.x;
		tmp.y = tmp.y * viewport.w + viewport.y;

		return vec3(tmp);
	}

	export vec3 unproject(const vec3& win, const mat4_generic& model, const mat4_generic& proj, const vec4& viewport)
	{
		mat4_generic inv = inverse(proj * model);

		vec4 temp(win, 1.0f);

		temp.x = (temp.x - viewport.x) / viewport.z;
		temp.y = (temp.y - viewport.y) / viewport.w;

		temp = temp * 2.0f - 1.0f;

		vec4 obj = inv * temp;
		obj /= obj.w;

		return vec3{obj};
	}

	export mat4_generic frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
	{
		mat4_generic result(0);

		result[0].x = (2.0f * near) / (right - left);
		result[1].y = (2.0f * near) / (top - bottom);
		result[2].x = (right + left) / (right - left);
		result[2].y = (top + bottom) / (top - bottom);
		result[2].z = -(far + near) / (far - near);
		result[2].w = -1.0f;
		result[3].z = -(2.0f * far * near) / (far - near);

		return result;
	}

	export f32 determinant(const mat4_generic& mat) { return mat.determinant(); }

	export using mat4 = mat4_generic;


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

namespace std
{
	template<>
	struct hash<mat4_generic>
	{
		size_t operator()(const mat4_generic& value) const { return deckard::utils::hash_values(value); }
	};

	template<>
	struct formatter<mat4_generic>
	{
		// TODO: Parse single or multi row?
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const mat4_generic& m, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "mat4(({:.5f}, {:.5f}, {:.5f}, {:.5f}),\n", m[0].x, m[0].y, m[0].z, m[0].w);
			std::format_to(ctx.out(), "     ({:.5f}, {:.5f}, {:.5f}, {:.5f}),\n", m[1].x, m[1].y, m[1].z, m[1].w);
			std::format_to(ctx.out(), "     ({:.5f}, {:.5f}, {:.5f}, {:.5f}),\n", m[2].x, m[2].y, m[2].z, m[2].w);
			std::format_to(ctx.out(), "     ({:.5f}, {:.5f}, {:.5f}, {:.5f})", m[3].x, m[3].y, m[3].z, m[3].w);
			return std::format_to(ctx.out(), ")");
		}
	};
} // namespace std
