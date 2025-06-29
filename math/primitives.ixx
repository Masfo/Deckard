export module deckard.math:primitives;


import deckard.types;
import deckard.as;
import deckard.vec;


export namespace deckard::math
{
	struct ray
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
	};


	struct line
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

	struct plane
	{
		vec3 normal{0.0f, 0.0f, 1.0f};
		f32   d{0.0f}; // distance from origin
		plane() = default;
		plane(const vec3& n, f32 distance)
			: normal(n)
			, d(distance)
		{
		}
	};

	struct sphere
	{
		f32 radius{0.0f};
		vec3  center{0.0f, 0.0f, 0.0f};
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

	struct capsule
	{
		vec3 start{0.0f, 0.0f, 0.0f};
		vec3 end{0.0f, 0.0f, 1.0f};
		f32   radius{1.0f};
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
