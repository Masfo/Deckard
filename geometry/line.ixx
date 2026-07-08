export module deckard.geometry:line;


import deckard.types;
import deckard.vec;

namespace deckard::geometry
{
	using namespace deckard::math;

	export struct line2d
	{
		vec2 start{0.0f, 0.0f};
		vec2 end{0.0f, 1.0f};
		line2d() = default;

		line2d(const vec2& s, const vec2& e)
			: start(s)
			, end(e)
		{
		}

		line2d(const vec2& e)
			: line2d({0.0f, 0.0f}, e)
		{
		}
	};

	export struct line3d
	{
		vec3 start{0.0f, 0.0f, 0.0f};
		vec3 end{0.0f, 0.0f, 1.0f};
		line3d() = default;

		line3d(const vec3& s, const vec3& e)
			: start(s)
			, end(e)
		{
		}

		line3d(const vec3& e)
			: line3d({0.0f, 0.0f, 0.0f}, e)
		{
		}
	};
} // namespace deckard::geometry
