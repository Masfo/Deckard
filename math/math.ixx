export module deckard.math;


export import deckard.vec;
export import deckard.matrix;
export import deckard.math.utils;

export import :quaternion;
export import :easing;

import deckard.types;

// TODO:
//      - ray
//      - line
//      - sphere
//      - plane
//      - 




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

	//https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	/*
	std::optional<vec3> ray_intersects_triangle( const vec3 &ray_origin, const vec3 &ray_vector, const triangle3& triangle)
    {
        constexpr float epsilon = std::numeric_limits<float>::epsilon();

        vec3 edge1 = triangle.b - triangle.a;
        vec3 edge2 = triangle.c - triangle.a;
        vec3 ray_cross_e2 = cross(ray_vector, edge2);
        float det = dot(edge1, ray_cross_e2);

        if (det > -epsilon && det < epsilon)
            return {};    // This ray is parallel to this triangle.

        float inv_det = 1.0 / det;
        vec3 s = ray_origin - triangle.a;
        float u = inv_det * dot(s, ray_cross_e2);

        if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u-1) > epsilon))
            return {};

        vec3 s_cross_e1 = cross(s, edge1);
        float v = inv_det * dot(ray_vector, s_cross_e1);

        if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
            return {};

        // At this stage we can compute t to find out where the intersection point is on the line.
        float t = inv_det * dot(edge2, s_cross_e1);

        if (t > epsilon) // ray intersection
        {
            return  vec3(ray_origin + ray_vector * t);
        }
        else // This means that there is a line intersection but not a ray intersection.
            return {};
    }
	*/


} // namespace deckard::math
