module;
#include <immintrin.h>
export module deckard.math:quaternion;

import deckard.assert;
import deckard.types;
import deckard.debug;

import :vec3_sse;
import std;

namespace deckard::math::sse
{
	using m128 = __m128;

	union QuatData
	{
		struct xyz
		{
			f32 x, y, z, w;
		} c;

		f32 element[4]{0.0f};

		m128 SSE;
	};

	static_assert(sizeof(QuatData) == 16);

	class quat
	{
	private:
		QuatData data;

	public:
		quat()
		{
			data.c.x = data.c.y = data.c.z = 0.0f;
			data.c.w                       = 1.0f;
		}

		quat(const vec3& v)
		{
			vec3 c = cos(v * 0.5f);
			vec3 s = sin(v * 0.5f);

			data.c.w = c[0] * c[1] * c[2] + s[0] * s[1] * s[2];
			data.c.x = s[0] * c[1] * c[2] - c[0] * s[1] * s[2];
			data.c.y = c[0] * s[1] * c[2] + s[0] * c[1] * s[2];
			data.c.z = c[0] * c[1] * s[2] - s[0] * s[1] * c[2];
		}

		float operator[](const int index) const noexcept
		{
			switch (index)
			{
				case 0: return data.c.x;
				case 1: return data.c.y;
				case 2: return data.c.z;
				case 3: return data.c.w;
				default:
				{
					dbg::trace();
					dbg::panic("quat: indexing out-of-bound");
				}
			}
		}

		float& operator[](int index)
		{
			assert::check(index < 4, "out-of-bounds, vec4 has 4 elements");
			return *(reinterpret_cast<float*>(&data.element[0]) + index);
		}

		void test()
		{
			//

			data.c.x = 1.0f;
			data.c.w = 2.0f;


			int x = 0;
		}
	};


} // namespace deckard::math::sse

export using quat = deckard::math::sse::quat;
static_assert(sizeof(quat) == 16, "quat sse should be 16-bytes");
