export module deckard.geometry:aabb;

import deckard.vec;

namespace deckard::geometry
{
	using namespace deckard::math;

	export struct aabb
	{
		vec3 min{0};
		vec3 max{0};
		constexpr aabb() = default;

		constexpr aabb(const vec3& min, const vec3& max)
			: min(min)
			, max(max)
		{
		}

		constexpr bool contains(const vec3& point) const
		{
			return point.x >= min.x and point.x <= max.x and point.y >= min.y and point.y <= max.y and point.z >= min.z and
				   point.z <= max.z;
		}

		constexpr bool intersects(const aabb& other) const
		{
			return (min.x <= other.max.x and max.x >= other.min.x) and (min.y <= other.max.y and max.y >= other.min.y) and
				   (min.z <= other.max.z and max.z >= other.min.z);
		}

		constexpr vec3 center() const { return (min + max) * 0.5f; }

		constexpr auto area() const -> f32
		{
			vec3 size = max - min;
			return 2.0f * (size.x * size.y + size.x * size.z + size.y * size.z);
		}

		constexpr auto volume() const -> f32
		{
			vec3 size = max - min;
			return size.x * size.y * size.z;
		}

		constexpr auto merge(const aabb& other) const -> aabb
		{
			return aabb{vec3{std::min(min.x, other.min.x), std::min(min.y, other.min.y), std::min(min.z, other.min.z)},
						vec3{std::max(max.x, other.max.x), std::max(max.y, other.max.y), std::max(max.z, other.max.z)}};
		}


		constexpr auto operator+(const aabb& other) const -> aabb { return merge(other); }

		constexpr auto operator+=(const aabb& other) -> aabb&
		{
			*this = merge(other);
			return *this;
		}
	};
} // namespace deckard::geometry

