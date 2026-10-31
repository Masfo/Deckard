export module deckard.vec;

export import :vec2;
export import :vec3;
export import :vec4;

export namespace deckard::vec
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

	// vec2 sum/product
	template<typename T>
	auto sum(const generic_vec2<T>& v)
	{
		return v.x + v.y;
	}

	template<typename T>
	auto product(const generic_vec2<T>& v)
	{
		return v.x * v.y;
	}

	// vec3 sum/product
	template<typename T>
	auto sum(const generic_vec3<T>& v)
	{
		return v.x + v.y + v.z;
	}

	template<typename T>
	auto product(const generic_vec3<T>& v)
	{
		return v.x * v.y * v.z;
	}

	// vec4 sum/product
	template<typename T>
	auto sum(const generic_vec4<T>& v)
	{
		return v.x + v.y + v.z + v.w;
	}

	template<typename T>
	auto product(const generic_vec4<T>& v)
	{
		return v.x * v.y * v.z * v.w;
	}

	//

	template<std::integral T, std::integral U>
	constexpr T index_from_2d(const generic_vec2<T>& v, U width)
	{
		return math::index_from_2d(v.x, v.y, width);
	}

	template<std::integral T, std::integral U, std::integral I>
	constexpr T index_from_3d(const generic_vec3<T>& v, U width, I height)
	{
		return math::index_from_3d(v.x, v.y, width, height);
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

	//
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

		ret.y   = (ret.y | (ret.y << S[3])) & B[3];
		ret.y   = (ret.y | (ret.y << S[2])) & B[2];
		ret.y   = (ret.y | (ret.y << S[1])) & B[1];
		ret.y   = (ret.y | (ret.y << S[0])) & B[0];
		T index = ret.x | (ret.y << 1);
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

		y = (y | (y << S[3])) & B[3];
		y = (y | (y << S[2])) & B[2];
		y = (y | (y << S[1])) & B[1];
		y = (y | (y << S[0])) & B[0];
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
				dbg::print("{:02} ", vec::to_zorder(math::uvec2{x, y}));
			}
			dbg::println();
		}
	*/


	/* Zig-Zag order
	
		00 01 05 06 14 15 27 28
		02 04 07 13 16 26 29 42
		03 08 12 17 25 30 41 43
		09 11 18 24 31 40 44 53
		10 19 23 32 39 45 52 54
		20 22 33 38 46 51 55 60
		21 34 37 47 50 56 59 61
		35 36 48 49 57 58 62 63
		
		for (const u32& y : upto(8))
		{
		for (const u32& x : upto(8))
		{
			dbg::print("{:02} ", vec::to_zigzag(uvec2{x, y}, 8, 8));
		}
		dbg::println();
		}
*/



	namespace detail
	{
		constexpr u64 isqrt(u64 n) noexcept
		{
			u64 res = 0;
			u64 bit = u64{1} << 62;
			while (bit > n)
				bit >>= 2;
			while (bit != 0)
			{
				if (n >= res + bit)
				{
					n -= res + bit;
					res = (res >> 1) + bit;
				}
				else
					res >>= 1;
				bit >>= 2;
			}
			return res;
		}

		constexpr u64 tri(u64 n) noexcept { return n * (n + 1) / 2; }

		constexpr u64 zz_before(u64 d, u64 w, u64 h) noexcept
		{
			const u64 m = std::min(w, h);
			const u64 M = std::max(w, h);
			const u64 n = w + h - 1;

			if (d < m)
				return tri(d);
			if (d <= M)
				return tri(m) + (d - m) * m;
			return w * h - tri(n - d);
		}

		constexpr auto zz_rows(u64 d, u64 w, u64 h) noexcept -> std::pair<u64, u64>
		{
			return {d >= w ? d - w + 1 : u64{0}, std::min(d, h - 1)};
		}
	} // namespace detail

	template<std::unsigned_integral T>
	[[nodiscard]] constexpr u64 to_zigzag(const generic_vec2<T>& v, u32 w, u32 h) noexcept
	{
		const u64 x = v.x, y = v.y;
		const u64 d         = x + y;
		const auto [lo, hi] = detail::zz_rows(d, w, h);

		return detail::zz_before(d, w, h) + ((d & 1) ? y - lo : hi - y);
	}

	template<std::unsigned_integral T>
	[[nodiscard]] constexpr auto from_zigzag(u64 i, u32 w, u32 h) noexcept -> generic_vec2<T>
	{
		const u64 m     = std::min(w, h);
		const u64 n     = u64{w} + h - 1;
		const u64 total = u64{w} * h;

		u64 d;
		if (i < detail::tri(m))
		{
			d = (detail::isqrt(8 * i + 1) - 1) / 2;
		}
		else if (i < total - detail::tri(m - 1))
		{
			d = m + (i - detail::tri(m)) / m;
		}
		else
		{
			const u64 j = total - 1 - i;
			d           = n - 1 - (detail::isqrt(8 * j + 1) - 1) / 2;
		}

		const u64 off       = i - detail::zz_before(d, w, h);
		const auto [lo, hi] = detail::zz_rows(d, w, h);
		const u64 y         = (d & 1) ? lo + off : hi - off;

		return {static_cast<T>(d - y), static_cast<T>(y)};
	}

} // namespace deckard::vec

export namespace std
{

	template<>
	struct std::hash<ivec2>
	{
		auto operator()(const ivec2& a) const { return utils::hash_values(a.x, a.y); }
	};

	template<>
	struct std::hash<ivec3>
	{
		auto operator()(const ivec3& a) const { return utils::hash_values(a.x, a.y, a.z); }
	};

	template<>
	struct std::hash<ivec4>
	{
		auto operator()(const ivec4& a) const { return utils::hash_values(a.x, a.y, a.z, a.w); }
	};
} // namespace std
