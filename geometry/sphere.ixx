export module deckard.geometry:sphere;

import deckard.types;
import deckard.vec;

namespace deckard::geometry
{
	using namespace deckard::math;

	export struct sphere
	{
		vec3 center{0.0f, 0.0f, 0.0f};
		f32  radius{1.0f};
		sphere() = default;

		sphere(const vec3& c, f32 r)
			: center(c)
			, radius(r)
		{
		}

		sphere(const vec3& c)
			: center(c)
		{
		}

		bool intersect(const sphere& s) const
		{
			f32 d = (center - s.center).length2();
			f32 r = (radius + s.radius);
			return d < r * r;
		}
	};

}; // namespace deckard::geometry
