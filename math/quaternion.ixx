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
static_assert(sizeof(quat) == 16);
