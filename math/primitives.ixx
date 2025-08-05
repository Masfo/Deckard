export module deckard.math:primitives;


import deckard.types;
import deckard.as;
import deckard.vec;
import deckard.math.utils;


//      - cube, cylinder, cone, pyramid, torus, prism, frustum
//		- curve (bezier, hermite, catmull-rom)

namespace deckard::math
{
	export struct intersection_result
	{
		bool hit{false};
		f32  distance{0.0f};
		vec3 point{0.0f, 0.0f, 0.0f};
	};


	

	export struct line
	{
		vec3 start{0.0f, 0.0f, 0.0f};
		vec3 end{0.0f, 0.0f, 1.0f};
		line() = default;

		line(const vec3& s, const vec3& e)
			: start(s)
			, end(e)
		{
		}

		line(const vec3& e)
			: end(e)
		{
			start = vec3(0.0f);
		}
	};

	export struct plane
	{
		vec3 normal{0.0f, 0.0f, 1.0f};
		f32  d{0.0f};
		plane() = default;

		plane(const vec3& n, f32 distance)
			: normal(n)
			, d(distance)
		{
		}
	};

	export struct ray
	{
		vec3 origin{0.0f, 0.0f, 0.0f};
		vec3 direction{0.0f, 0.0f, 1.0f};
		ray() = default;

		ray(const vec3& o, const vec3& d)
			: origin(o)
			, direction(d)
		{
			direction.normalize();
		}

		ray(const vec3& d)
			: direction(d)
		{
			origin = vec3(0.0f);
			direction.normalize();
		}

		vec3 at(f32 t) const { return origin + t * direction; }

		intersection_result intersect(const plane& p) const
		{
			intersection_result result;

			// Check if ray and plane are parallel
			f32 denom = dot(p.normal, direction);

			if (math::is_close_enough_zero(denom))
			{
				// Calculate distance along ray
				result.distance = -(dot(p.normal, origin) + p.d) / denom;

				// Check if intersection is in front of ray origin
				if (result.distance >= 0.0f)
				{
					result.hit   = true;
					result.point = at(result.distance);
				}
			}

			return result;
		}
	};


	export struct sphere
	{
		f32  radius{0.0f};
		vec3 center{0.0f, 0.0f, 0.0f};
		sphere() = default;

		sphere(float r, const vec3& c)
			: radius(r)
			, center(c)
		{
		}

		sphere(const vec3& c)
			: center(c)
		{
		}
	};

	export struct capsule
	{
		vec3 start{0.0f, 0.0f, 0.0f};
		vec3 end{0.0f, 0.0f, 1.0f};
		f32  radius{1.0f};
		capsule() = default;

		capsule(const vec3& s, const vec3& e, f32 r)
			: start(s)
			, end(e)
			, radius(r)
		{
		}

		capsule(const vec3& e, f32 r)
			: end(e)
			, radius(r)
		{
			start = vec3(0.0f);
		}
	};


} // namespace deckard::math
