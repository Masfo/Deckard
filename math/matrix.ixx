
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

		mat4_generic(const std::array<f32, 16>& v) { data = v; }

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


#ifdef __cpp_multidimensional_subscript
#error("use multisubscript")
		// const f32& operator[](std::size_t z, std::size_t y) const noexcept { return 0.0f; }
#endif


		const f32& operator()(u32 i, u32 j) const
		{
			assert::check(i < data.size() or j < data.size(), "mat4: indexing out-of-bounds");

			return data[j * 4 + i];
		}

		f32& operator[](int index) noexcept
		{
			assert::check(index < data.size(), "mat4: indexing out-of-bounds");
			return data[index];
		}

		const f32& operator[](u32 index) const noexcept
		{
			assert::check(index < data.size(), "mat4_generic: indexing out-of-bounds");
			return data[index];
		}

		mat4_generic operator*(const mat4_generic& rhs) noexcept
		{
			std::array<f32, 16> tmp{0.0f};

			for (int col = 0; col < 4; ++col)
			{
				for (int row = 0; row < 4; ++row)
				{
					f32 sum{0.0f};
					for (int i = 0; i < 4; ++i)
						sum += this->operator()(row, i) * rhs(i, col);

					tmp[row + col * 4] = sum;
				}
			}
			return mat4_generic(tmp.data());
		}

		f32 determinant() const
		{
			auto& m = *this;

			const f32 a0 = m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
			const f32 a1 = m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0);
			const f32 a2 = m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0);
			const f32 a3 = m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1);
			const f32 a4 = m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1);
			const f32 a5 = m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2);
			const f32 b0 = m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0);
			const f32 b1 = m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0);
			const f32 b2 = m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0);
			const f32 b3 = m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1);
			const f32 b4 = m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1);
			const f32 b5 = m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2);

			return (a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0);
		}

		mat4_generic operator-() const noexcept
		{
			mat4_generic m(*this);

			m = m * mat4_generic(-1.0f);
			return m;
		}

		mat4_generic operator+() const noexcept { return *this; }

		bool operator==(const mat4_generic& lhs) const noexcept { return data == lhs.data; }

		static mat4_generic identity() noexcept { return mat4_generic(1.0f); }

	private:
		std::array<f32, 16> data{0.0f};
	};

	// multiply mat4_generic by mat4_generic
	export mat4_generic operator*(const mat4_generic& lhs, const mat4_generic& rhs) noexcept
	{
		std::array<f32, 16> tmp{0.0f};

		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				f32 sum{0.0f};
				for (int i = 0; i < 4; ++i)
					sum += lhs(row, i) * rhs(i, col);

				tmp[row + col * 4] = sum;
			}
		}
		return mat4_generic(tmp.data());
	}

	export mat4_generic operator+(const mat4_generic& lhs, const mat4_generic& rhs) noexcept
	{
		mat4_generic result(1.0f);
		for (int i = 0; i < 16; ++i)
			result[i] = lhs[i] + rhs[i];
		return result;
	}

	export mat4_generic operator-(const mat4_generic& lhs, const mat4_generic& rhs) noexcept
	{
		mat4_generic result(1.0f);
		for (int i = 0; i < 16; ++i)
			result[i] = lhs[i] - rhs[i];
		return result;
	}

	export mat4_generic inverse(const mat4_generic& mat) noexcept;

	export mat4_generic operator/(const mat4_generic& lhs, const mat4_generic& rhs) noexcept { return lhs * inverse(rhs); }

	export void operator*=(mat4_generic& lhs, const mat4_generic& rhs) noexcept { lhs = lhs * rhs; }

	export void operator/=(mat4_generic& lhs, const mat4_generic& rhs) noexcept { lhs = lhs / rhs; }

	export void operator+=(mat4_generic& lhs, const mat4_generic& rhs) noexcept { lhs = lhs + rhs; }

	export void operator-=(mat4_generic& lhs, const mat4_generic& rhs) noexcept { lhs = lhs - rhs; }

	export mat4_generic inverse(const mat4_generic& mat) noexcept
	{

		if (is_close_enough(mat.determinant(), 0.0f))
			return {};

		const vec3& a = vec3(mat(0, 0), mat(1, 0), mat(2, 0));
		const vec3& b = vec3(mat(0, 1), mat(1, 1), mat(2, 1));
		const vec3& c = vec3(mat(0, 2), mat(1, 2), mat(2, 2));
		const vec3& d = vec3(mat(0, 3), mat(1, 3), mat(2, 3));
		const f32&  x = mat(3, 0);
		const f32&  y = mat(3, 1);
		const f32&  z = mat(3, 2);
		const f32&  w = mat(3, 3);

		vec3 s = cross(a, b);
		vec3 t = cross(c, d);
		vec3 u = a * y - b * x;
		vec3 v = c * w - d * z;

		const f32 invDet = 1.0f / (dot(s, v) + dot(t, u));
		s *= invDet;
		t *= invDet;
		u *= invDet;
		v *= invDet;

		const vec3 r0 = cross(b, v) + t * y;
		const vec3 r1 = cross(v, a) - t * x;
		const vec3 r2 = cross(d, u) + s * w;
		const vec3 r3 = cross(u, c) - s * z;

		return (mat4_generic(
		  r0[0],
		  r0[1],
		  r0[2],
		  -dot(b, t),
		  r1[0],
		  r1[1],
		  r1[2],
		  dot(a, t),
		  r2[0],
		  r2[1],
		  r2[2],
		  -dot(d, s),
		  r3[0],
		  r3[1],
		  r3[2],
		  dot(c, s)));
	}

	export mat4_generic transpose(const mat4_generic& mat) noexcept
	{
		return mat4_generic(
		  vec4(mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3)),
		  vec4(mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3)),
		  vec4(mat(2, 0), mat(2, 1), mat(2, 2), mat(2, 3)),
		  vec4(mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3)));
	}

	export mat4_generic lookat_rh(const vec3& eye, const vec3& center, const vec3& up) noexcept
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

	export mat4_generic scale(const mat4_generic& mat, const vec3& scale) noexcept
	{
		return mat4_generic(
		  vec4(mat(0, 0) * scale[0], mat(1, 0) * scale[0], mat(2, 0) * scale[0], mat(3, 0) * scale[0]),
		  vec4(mat(0, 1) * scale[1], mat(1, 1) * scale[1], mat(2, 1) * scale[1], mat(3, 1) * scale[1]),
		  vec4(mat(0, 2) * scale[2], mat(1, 2) * scale[2], mat(2, 2) * scale[2], mat(3, 2) * scale[2]),
		  vec4(mat(0, 3), mat(1, 3), mat(2, 3), mat(3, 3)));
	}

	export mat4_generic translate(const mat4_generic& mat, const vec3& translate) noexcept
	{

		vec4 v(mat(0, 0), mat(1, 0), mat(2, 0), mat(3, 0));

		v *= translate[0];
		v += vec4(mat(0, 1), mat(1, 1), mat(2, 1), mat(3, 1)) * translate[1];
		v += vec4(mat(0, 2), mat(1, 2), mat(2, 2), mat(3, 2)) * translate[2];
		v += vec4(mat(0, 3), mat(1, 3), mat(2, 3), mat(3, 3));


		return mat4_generic(
		  vec4(mat(0, 0), mat(1, 0), mat(2, 0), mat(3, 0)),
		  vec4(mat(0, 1), mat(1, 1), mat(2, 1), mat(3, 1)),
		  vec4(mat(0, 2), mat(1, 2), mat(2, 2), mat(3, 2)),
		  v);
	}

	export mat4_generic perspective(f32 fov, f32 aspect, f32 near, f32 far) noexcept
	{
		f32 const tanHalfFovy = std::tan(fov / 2.0f);

		return mat4_generic(
		  vec4(1.0f / (aspect * tanHalfFovy), 0.0f, 0.0f, 0.0f),
		  vec4(0.0f, 1.0f / tanHalfFovy, 0.0f, 0.0f),
		  vec4(0.0f, 0.0f, -(far + near) / (far - near), -1.0f),
		  vec4(0.0f, 0.0f, -(2.0f * far * near) / (far - near), 0.0f));
	}

	export mat4_generic ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) noexcept
	{
		return mat4_generic(
		  vec4(2 / (right - left), 0.0f, 0.0f, 0.0f),
		  vec4(0.0f, 2 / (top - bottom), 0.0f, 0.0f),
		  vec4(0.0f, 0.0f, -(2 / (far - near)), 0.0f),
		  vec4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(far + near) / (far - near), 1.0f));
	}

	// rotate around unit vector
	// rotate x,y,z

	// reflection

	/*
	export struct alignas(16) sse_mat4
	{
		mat4() = default;

		mat4(float scalar) { m_data.fill(scalar); }

		// mat4(1.0) = identity

		float operator[](size_t index) const noexcept
		{
			assert::check(index < 16, "mat4: indexing out-of-bounds");
			return m_data[index];
		}

		float& operator[](size_t index) noexcept
		{
			assert::check(index < 16, "mat4: indexing out-of-bounds");
			return m_data[index];
		}



		// mdspan
		std::array<float, 16> m_data{0};

		static mat4 identity() noexcept
		{
			mat4 ret;
			ret[0]  = 1.0f;
			ret[5]  = 1.0f;
			ret[10] = 1.0f;
			ret[15] = 1.0f;

			return ret;
		}
	};

	static_assert(sizeof(mat4) == 16 * sizeof(float), "Matrix 4x4 should be 16x4 bytes");

	namespace sse
	{
		/*
		 inline void transpose4x4_SSE(float *A, float *B, const int lda, const int ldb) {
	__m128 row1 = _mm_load_ps(&A[0*lda]);
	__m128 row2 = _mm_load_ps(&A[1*lda]);
	__m128 row3 = _mm_load_ps(&A[2*lda]);
	__m128 row4 = _mm_load_ps(&A[3*lda]);
	 _MM_TRANSPOSE4_PS(row1, row2, row3, row4);
	 _mm_store_ps(&B[0*ldb], row1);
	 _mm_store_ps(&B[1*ldb], row2);
	 _mm_store_ps(&B[2*ldb], row3);
	 _mm_store_ps(&B[3*ldb], row4);
}

inline void transpose_block_SSE4x4(float *A, float *B, const int n, const int m, const int lda, const int ldb ,const int block_size) {
	#pragma omp parallel for
	for(int i=0; i<n; i+=block_size) {
		for(int j=0; j<m; j+=block_size) {
			int max_i2 = i+block_size < n ? i + block_size : n;
			int max_j2 = j+block_size < m ? j + block_size : m;
			for(int i2=i; i2<max_i2; i2+=4) {
				for(int j2=j; j2<max_j2; j2+=4) {
					transpose4x4_SSE(&A[i2*lda +j2], &B[j2*ldb + i2], lda, ldb);
				}
			}
		}
	}
}
*/


} // namespace deckard::math

export using mat4 = deckard::math::mat4_generic;
