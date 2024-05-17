module;
#include <array>
#include <xmmintrin.h>

export module deckard.math.matrix;

import deckard.assert;
import deckard.utils.hash;
import deckard.math.vec4.sse;

namespace deckard::math
{

	export class mat3
	{
	};

	export struct alignas(16) mat4
	{
		mat4() = default;

		mat4(float scalar) { m_data.fill(scalar); }

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
