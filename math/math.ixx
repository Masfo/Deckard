export module deckard.math;

export import deckard.math.utils;
export import deckard.vec;
export import deckard.matrix;



export import :quaternion;
export import :easing;

import deckard.types;


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
