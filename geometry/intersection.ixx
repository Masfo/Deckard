export module deckard.geometry:intersect;

import deckard.math.utils;

import :aabb;
import :ray;
import :plane;
import :sphere;

namespace deckard::geometry
{
	export struct [[nodiscard]] intersection_result
	{
		bool hit{false};
		f32  distance{0.0f};
		vec3 point{0.0f, 0.0f, 0.0f};
	};


	// ray - rect

	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
	// ray - sphere
	export intersection_result intersect(const ray3d& r, const sphere& s)
	{
		intersection_result result;
		vec3                oc           = r.origin - s.center;
		f32                 a            = dot(r.direction, r.direction);
		f32                 b            = 2.0f * dot(oc, r.direction);
		f32                 c            = dot(oc, oc) - s.radius * s.radius;
		f32                 discriminant = b * b - 4 * a * c;
		if (discriminant < 0)
			return result; // No intersection

		f32 sqrt_disc = std::sqrt(discriminant);
		f32 t1        = (-b - sqrt_disc) / (2.0f * a);
		f32 t2        = (-b + sqrt_disc) / (2.0f * a);
		if (t1 >= 0.0f)
		{
			result.hit      = true;
			result.distance = t1;
			result.point    = r.at(t1);
		}
		else if (t2 >= 0.0f)
		{
			result.hit      = true;
			result.distance = t2;
			result.point    = r.at(t2);
		}
		return result;
	}

	export auto intersect(const sphere& s, const ray3d& r) { return intersect(r, s); }

	// sphere - sphere
	export [[nodiscard]] bool intersect(const sphere& s1, const sphere& s2)
	{
		f32 distance_squared = (s1.center - s2.center).length2();
		f32 radius_sum       = s1.radius + s2.radius;
		return distance_squared <= radius_sum * radius_sum;
	}

	// ray - plane
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-intersection
	export intersection_result intersect(const ray3d& r, const plane& p)
	{
		intersection_result result;
		f32                 denom = dot(p.normal, r.direction);
		if (not math::is_close_enough_zero(denom))
		{
			result.distance = -(dot(p.normal, r.origin) + p.distance) / denom;
			if (result.distance >= 0.0f)
			{
				result.hit   = true;
				result.point = r.at(result.distance);
			}
		}
		return result;
	}

	export intersection_result intersect(const plane& p, const ray3d& r) { return intersect(r, p); }

	// ray - aabb
	export intersection_result intersect(const ray3d& r, const aabb& box)
	{
		intersection_result result;
		f32                 tmin = (box.min.x - r.origin.x) / r.direction.x;
		f32                 tmax = (box.max.x - r.origin.x) / r.direction.x;
		if (tmin > tmax)
			std::swap(tmin, tmax);
		f32 tymin = (box.min.y - r.origin.y) / r.direction.y;
		f32 tymax = (box.max.y - r.origin.y) / r.direction.y;
		if (tymin > tymax)
			std::swap(tymin, tymax);
		if ((tmin > tymax) || (tymin > tmax))
			return result;
		if (tymin > tmin)
			tmin = tymin;
		if (tymax < tmax)
			tmax = tymax;
		f32 tzmin = (box.min.z - r.origin.z) / r.direction.z;
		f32 tzmax = (box.max.z - r.origin.z) / r.direction.z;
		if (tzmin > tzmax)
			std::swap(tzmin, tzmax);
		if ((tmin > tzmax) || (tzmin > tmax))
			return result;
		if (tzmin > tmin)
			tmin = tzmin;
		if (tzmax < tmax)
			tmax = tzmax;
		result.hit      = true;
		result.distance = tmin >= 0.0f ? tmin : tmax;
		result.point    = r.at(result.distance);
		return result;
	}

} // namespace deckard::geometry
