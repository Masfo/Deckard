export module deckard.math;

export import :utils;
export import :formatter;
export import :vec2;
export import :vec3;
export import :vec4;


export import :matrix;
export import :quaternion;
export import :easing;

import deckard.types;

namespace deckard::math
{

export using uvec2 = generic_vec2<u64>;
export using uvec3 = generic_vec3<u64>;
export using uvec4 = generic_vec4<u64>;

export using ivec2 = generic_vec2<i64>;
export using ivec3 = generic_vec3<i64>;
export using ivec4 = generic_vec4<i64>;


export using vec2  = generic_vec2<f32>;
export using vec3  = generic_vec3<f32>;
export using vec4  = generic_vec4<f32>;

//export using mat4 = deckard::math::mat4_generic;

//export using quat = deckard::math::quat;


} // namespace deckard::math

namespace deckard::math
{
	export auto grid_order = [](const ivec2& v1, const ivec2& v2) -> bool
	{
		//
		return (v1.y < v2.y) || (v1.y == v2.y && v1.x < v2.x);
	};

	export auto grid_order_reverse = [](const ivec2& v1, const ivec2& v2) -> bool
	{
		//
		return (v1.y > v2.y) || (v1.y == v2.y && v1.x > v2.x);
	};


} // namespace deckard::math
