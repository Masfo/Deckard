export module deckard.math;

export import :utils;
export import :formatter;

export import :vec.generic;
export import :vec2_sse;
export import :vec3_sse;
export import :vec4_sse;
export import :vec_sse_generic;
export import :matrix;

import deckard.types;

namespace deckard::math
{

#if 0
	// TODO: swich to generic SSE vec when compiler is happy
	export using vec4 = sse::vec4; // sse::vec_n_sse<4>;
	export using vec3 = sse::vec3; // sse::vec_n_sse<3>;
	export using vec2 = sse::vec2; // sse::vec_n_sse<2>;
#else
	export using vec4 = vec_n<float, 4>;
	export using vec3 = vec_n<float, 3>;
	export using vec2 = vec_n<float, 2>;
#endif

	export using uvec2 = vec_n<u32, 2>;
	export using uvec3 = vec_n<u32, 3>;
	export using uvec4 = vec_n<u32, 4>;

	export using ivec2 = vec_n<i32, 2>;
	export using ivec3 = vec_n<i32, 3>;
	export using ivec4 = vec_n<i32, 4>;


} // namespace deckard::math
