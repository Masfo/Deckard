export module deckard.geometry:frustum;

import :plane;

import deckard.types;
import deckard.math.utils;
import deckard.vec;


namespace deckard::geometry
{
	using namespace deckard::math;

	enum class frustum_plane
	{
		left   = 0,
		right  = 1,
		bottom = 2,
		top    = 3,
		near   = 4,
		far    = 5
	};

	struct frustum
	{
		std::array<plane, 6> planes; // left, right, bottom, top, near, far
		
		 [[nodiscard]] constexpr bool contains(vec3 const& point) const noexcept
		{
			for (const auto& p : planes)
			{
				if (p.distance_to(point) < 0.0f)
				{
					return false;
				}
			}
			return true;
		}
	};
}
