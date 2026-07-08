export module deckard.geometry:ray;

import deckard.types;
import deckard.vec;

namespace deckard::geometry
{
	using namespace deckard::math;

	export struct ray2d
	{
		vec2 origin{0.0f, 0.0f};
		vec2 direction{1.0f, 0.0f};

		ray2d() = default;

		ray2d(const vec2& o, const vec2& d)
			: origin(o)
			, direction(d.normalized())
		{
		}

		explicit ray2d(const vec2& d)
			: direction(d.normalized())
		{
		}

		[[nodiscard]] static ray2d from_origin(vec2 o, vec2 d) noexcept { return {o, d}; }

		[[nodiscard]] vec2 at(f32 t) const noexcept { return origin + t * direction; }
	};

	export struct ray3d
	{
		vec3 origin{0.0f, 0.0f, 0.0f};
		vec3 direction{0.0f, 0.0f, 1.0f};
		ray3d() = default;

		ray3d(const vec3& o, const vec3& d)
			: origin(o)
			, direction(d)
		{
			direction.normalize();
		}

		ray3d(const vec3& d)
			: direction(d)
		{
			origin = vec3(0.0f);
			direction.normalize();
		}

		[[nodiscard]] static ray3d from_origin(vec3 o, vec3 d) noexcept { return {o, d.normalized()}; }

		vec3 at(f32 t) const { return origin + t * direction; }
	};
} // namespace deckard::geometry
