module;
#include <array>
#include <xmmintrin.h>

export module deckard.math:matrix;
import :vec4_sse;

import deckard.assert;
import deckard.utils.hash;

namespace deckard::math
{

	export class mat3
	{
	};

	//  0  1  2  3
	//  4  5  6  7
	//  8  9 10 11
	// 12 13 14 15
	//
	//  0  4  8 12
	//  1  5  9 13
	//  2  6 10 14
	//  3  7 11 15
	//

	// TODO: benchmark mat4 transpose, using sse swaps/shuffle
	//		 vs. just indexing and rearrange

	export struct alignas(16) mat4
	{
		mat4() = default;

		mat4(float scalar) { m_data.fill(scalar); }

		// mat4(1.0) = identity

		float operator[](size_t index) const noexcept
		{
			// assert::check(index < 16, "mat4: indexing out-of-bounds");
			return m_data[index];
		}

		float& operator[](size_t index) noexcept
		{
			// assert::check(index < 16, "mat4: indexing out-of-bounds");
			return m_data[index];
		}

#ifdef __cpp_multidimensional_subscript
#error("use mdspan")
		// constexpr float& operator[](std::size_t z, std::size_t y, std::size_t x) noexcept { return 0.0f; }
#endif

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
		using m128 = __m128;

		export struct alignas(16) mat4
		{
			void operator+=(const mat4& lhs) noexcept
			{
				col[0] = _mm_add_ps(col[0], lhs.col[0]);
				col[1] = _mm_add_ps(col[1], lhs.col[1]);
				col[2] = _mm_add_ps(col[2], lhs.col[2]);
				col[3] = _mm_add_ps(col[3], lhs.col[3]);
			}

			void operator-=(const mat4& lhs) noexcept
			{
				col[0] = _mm_sub_ps(col[0], lhs.col[0]);
				col[1] = _mm_sub_ps(col[1], lhs.col[1]);
				col[2] = _mm_sub_ps(col[2], lhs.col[2]);
				col[3] = _mm_sub_ps(col[3], lhs.col[3]);
			}

			void operator*=(const mat4& lhs) noexcept
			{
				col[0] = _mm_mul_ps(col[0], lhs.col[0]);
				col[1] = _mm_mul_ps(col[1], lhs.col[1]);
				col[2] = _mm_mul_ps(col[2], lhs.col[2]);
				col[3] = _mm_mul_ps(col[3], lhs.col[3]);
			}

			void operator/=(const mat4& lhs) noexcept
			{
				col[0] = _mm_div_ps(col[0], lhs.col[0]);
				col[1] = _mm_div_ps(col[1], lhs.col[1]);
				col[2] = _mm_div_ps(col[2], lhs.col[2]);
				col[3] = _mm_div_ps(col[3], lhs.col[3]);
			}

			m128 col[4];
		};
	} // namespace sse


} // namespace deckard::math
