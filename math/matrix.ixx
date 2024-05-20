
export module deckard.math:matrix;
import :vec.generic;

import std;
import deckard.assert;
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
		};

		inline static fill fill{};

		mat4_generic()
			: mat4_generic(1.0f)
		{
		}

		// identity
		mat4_generic(const float v) { data[0] = data[5] = data[10] = data[15] = v; }

		mat4_generic(const float v, struct fill) { data.fill(v); }

		mat4_generic(float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9, float f10, float f11,
					 float f12, float f13, float f14, float f15, float f16)
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

		mat4_generic(const std::array<float, 16>& v) { data = v; }

		mat4_generic(const float* v) { std::ranges::copy_n(v, 16, data.begin()); }

		const float& operator()(int i, int j) const { return data[j * 4 + i]; }

		const float& operator[](int index) const noexcept
		{
			assert::check(index < 16, "mat4: indexing out-of-bounds");
			return data[index];
		}

		float determinant() const
		{
			auto& m = *this;

			const float a0 = m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
			const float a1 = m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0);
			const float a2 = m(0, 0) * m(1, 3) - m(0, 3) * m(1, 0);
			const float a3 = m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1);
			const float a4 = m(0, 1) * m(1, 3) - m(0, 3) * m(1, 1);
			const float a5 = m(0, 2) * m(1, 3) - m(0, 3) * m(1, 2);
			const float b0 = m(2, 0) * m(3, 1) - m(2, 1) * m(3, 0);
			const float b1 = m(2, 0) * m(3, 2) - m(2, 2) * m(3, 0);
			const float b2 = m(2, 0) * m(3, 3) - m(2, 3) * m(3, 0);
			const float b3 = m(2, 1) * m(3, 2) - m(2, 2) * m(3, 1);
			const float b4 = m(2, 1) * m(3, 3) - m(2, 3) * m(3, 1);
			const float b5 = m(2, 2) * m(3, 3) - m(2, 3) * m(3, 2);

			return (a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0);
		}


#ifdef __cpp_multidimensional_subscript
#error("use mdspan")
		// constexpr float& operator[](std::size_t z, std::size_t y, std::size_t x) noexcept { return 0.0f; }
#endif

		float& operator[](int index) noexcept
		{
			assert::check(index < 16, "mat4_generic: indexing out-of-bounds");
			return data[index];
		}

		bool operator==(const mat4_generic& lhs) const noexcept { return data == lhs.data; }

		static mat4_generic identity() noexcept { return mat4_generic(1.0f); }

	private:
		std::array<float, 16> data{0.0f};
	};

	// multiply mat4_generic by mat4_generic
	export mat4_generic operator*(const mat4_generic& lhs, const mat4_generic& rhs) noexcept
	{
		std::array<float, 16> tmp{0.0f};

		for (int col = 0; col < 4; ++col)
		{
			for (int row = 0; row < 4; ++row)
			{
				float sum{0.0f};
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
		using vec3 = vec_n<float, 3>;

		if (is_close_enough(mat.determinant(), 0.000001f))
			return {};


		const vec3&  a = vec3(mat(0, 0), mat(1, 0), mat(2, 0));
		const vec3&  b = vec3(mat(0, 1), mat(1, 1), mat(2, 1));
		const vec3&  c = vec3(mat(0, 2), mat(1, 2), mat(2, 2));
		const vec3&  d = vec3(mat(0, 3), mat(1, 3), mat(2, 3));
		const float& x = mat(3, 0);
		const float& y = mat(3, 1);
		const float& z = mat(3, 2);
		const float& w = mat(3, 3);

		vec3 s = cross(a, b);
		vec3 t = cross(c, d);
		vec3 u = a * y - b * x;
		vec3 v = c * w - d * z;

		const float invDet = 1.0f / (dot(s, v) + dot(t, u));
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

	// TODO: benchmark mat4 transpose, using sse swaps/shuffle
	//		 vs. just indexing and rearrange
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
