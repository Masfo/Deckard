export module deckard.math.vec;

import std;
import deckard.as;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.math.utils;
import deckard.utils.hash;

import deckard.math.vec.generic;
import deckard.math.vec.sse.generic;

import deckard.math.vec2.sse;
import deckard.math.vec3.sse;
import deckard.math.vec4.sse;


template<typename T>
concept arithmetic = std::integral<T> or std::floating_point<T>;

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

// STD specials
namespace std
{
	using namespace deckard::math;


} // namespace std
