export module deckard.vec;

export import :vec2;
export import :vec3;
export import :vec4;

namespace deckard::math
{

	export using uvec2   = generic_vec2<u32>;
	export using uvec3   = generic_vec3<u32>;
	export using uvec4   = generic_vec4<u32>;
	export using u64vec2 = generic_vec2<u64>;
	export using u64vec3 = generic_vec3<u64>;
	export using u64vec4 = generic_vec4<u64>;

	export using ivec2   = generic_vec2<i32>;
	export using ivec3   = generic_vec3<i32>;
	export using ivec4   = generic_vec4<i32>;
	export using i64vec2 = generic_vec2<i64>;
	export using i64vec3 = generic_vec3<i64>;
	export using i64vec4 = generic_vec4<i64>;


	static_assert(sizeof(ivec2) == 2 * sizeof(i32));
	static_assert(sizeof(ivec3) == 3 * sizeof(i32));
	static_assert(sizeof(ivec4) == 4 * sizeof(i32));


	export using vec2 = generic_vec2<f32>;
	export using vec3 = generic_vec3<f32>;
	export using vec4 = generic_vec4<f32>;

	static_assert(sizeof(vec2) == 2 * sizeof(f32));
	static_assert(sizeof(vec3) == 3 * sizeof(f32));
	static_assert(sizeof(vec4) == 4 * sizeof(f32));


} // namespace deckard::math
