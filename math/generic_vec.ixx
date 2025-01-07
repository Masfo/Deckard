export module deckard.vec;

export import :vec2;
export import :vec3;
export import :vec4;

export namespace deckard::math
{

	using uvec2   = generic_vec2<u32>;
	using uvec3   = generic_vec3<u32>;
	using uvec4   = generic_vec4<u32>;
	using u64vec2 = generic_vec2<u64>;
	using u64vec3 = generic_vec3<u64>;
	using u64vec4 = generic_vec4<u64>;

	using ivec2   = generic_vec2<i32>;
	using ivec3   = generic_vec3<i32>;
	using ivec4   = generic_vec4<i32>;
	using i64vec2 = generic_vec2<i64>;
	using i64vec3 = generic_vec3<i64>;
	using i64vec4 = generic_vec4<i64>;


	static_assert(sizeof(ivec2) == 2 * sizeof(i32));
	static_assert(sizeof(ivec3) == 3 * sizeof(i32));
	static_assert(sizeof(ivec4) == 4 * sizeof(i32));


	using vec2 = generic_vec2<f32>;
	using vec3 = generic_vec3<f32>;
	using vec4 = generic_vec4<f32>;

	static_assert(sizeof(vec2) == 2 * sizeof(f32));
	static_assert(sizeof(vec3) == 3 * sizeof(f32));
	static_assert(sizeof(vec4) == 4 * sizeof(f32));

	template<std::integral T, std::integral U>
	constexpr T from_2d_to_index(const generic_vec2<T>& v, U width)
	{
		return index_from_2d(v.x, v.y, width);
	}

	
	template<std::integral T, std::integral U, std::integral I>
	constexpr T from_3d_to_index(const generic_vec3<T>& v, U width, I height)
	{
		return index_from_3d(v.x, v.y, width, height);
	}

	template<std::integral T, std::integral U>
	constexpr generic_vec2<T> from_index_to_2d(const T& index, U width)
	{
		return {index % width, index / width};
	}

	template<std::integral T, std::integral U, std::integral I>
	constexpr generic_vec3<T> from_index_to_3d(const T& index, U width, I height)
	{
		return {index % width, mod((index / width), height), index / (width * height)};
	}

} // namespace deckard::math
