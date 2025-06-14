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

	template<std::unsigned_integral T, std::integral U>
	constexpr generic_vec2<T> to_zorder(const generic_vec2<T>& v, const U width)
	{
		constexpr u32 S[] = {1, 2, 4, 8};
		constexpr u32 B[] = {0x5555'5555, 0x3333'3333, 0x0F0F'0F0F, 0x00FF'00FF};

		generic_vec2<T> ret{v};

		ret.x = (ret.x | (ret.x << S[3])) & B[3];
		ret.x = (ret.x | (ret.x << S[2])) & B[2];
		ret.x = (ret.x | (ret.x << S[1])) & B[1];
		ret.x = (ret.x | (ret.x << S[0])) & B[0];

		ret.y = (ret.y | (ret.y << S[3])) & B[3];
		ret.y = (ret.y | (ret.y << S[2])) & B[2];
		ret.y = (ret.y | (ret.y << S[1])) & B[1];
		ret.y = (ret.y | (ret.y << S[0])) & B[0];
		T index =  ret.x | (ret.y << 1);
		return from_index_to_2d(index, width);
	}

	
	template<std::unsigned_integral T>
	constexpr T to_zorder(const generic_vec2<T>& v)
	{
		constexpr u32 S[] = {1, 2, 4, 8};
		constexpr u32 B[] = {0x5555'5555, 0x3333'3333, 0x0F0F'0F0F, 0x00FF'00FF};

		T x = v.x;
		T y = v.y;

		x = (x | (x << S[3])) & B[3];
		x = (x | (x << S[2])) & B[2];
		x = (x | (x << S[1])) & B[1];
		x = (x | (x << S[0])) & B[0];

		y   = (y | (y << S[3])) & B[3];
		y   = (y | (y << S[2])) & B[2];
		y   = (y | (y << S[1])) & B[1];
		y   = (y | (y << S[0])) & B[0];
		return x | (y << 1);
	}

	template<std::unsigned_integral T>
	constexpr T to_zorder(const T x, const T y)                                                                        
	{
		return to_zorder(generic_vec2<T>{x, y});
	}
	/*
		00 01 04 05 16 17 20 21 
		02 03 06 07 18 19 22 23 
		08 09 12 13 24 25 28 29 
		10 11 14 15 26 27 30 31 
		32 33 36 37 48 49 52 53 
		34 35 38 39 50 51 54 55 
		40 41 44 45 56 57 60 61 
		42 43 46 47 58 59 62 63 

		for (const u32& y : upto(8))
		{
			for (const u32& x : upto(8))
			{
				dbg::print("{:02} ", math::to_zorder(math::uvec2{x, y}));
			}
			dbg::println();
		}
	*/

} // namespace deckard::math
