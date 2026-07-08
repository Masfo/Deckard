export module deckard.geometry:plane;

import deckard.types;
import deckard.vec;

namespace deckard::geometry
{
	using namespace deckard::math;

	export struct plane
	{

		vec3 normal{0.0f, 0.0f, 1.0f};
		f32  distance{0.0f};


		plane() = default;

		plane(const vec3& n, f32 distance)
			: normal(n)
			, distance(distance)
		{
		}

		plane(const vec3& n, const vec3& point)
			: normal(n)
			, distance(-dot(normal, point))
		{
		}

		plane& normalize()
		{
			f32 length = normal.length();
			if (length > 0.0f)
			{
				normal /= length;
				distance /= length;
			}
			return *this;
		}

		[[nodiscard]] constexpr float distance_to(vec3 const& point) const noexcept { return normal.dot(point) + distance; }
	};

	static_assert(sizeof(plane) == 4 * sizeof(f32), "plane should be tightly packed");


} // namespace deckard::geometry
