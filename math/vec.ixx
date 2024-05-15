export module deckard.math.vec;

import std;
import deckard.as;
import deckard.assert;
import deckard.types;
import deckard.debug;
import deckard.math.utility;
import deckard.helpers;

import deckard.math.vec.sse;
import deckard.math.vec.x86;

template<typename T>
concept arithmetic = std::integral<T> or std::floating_point<T>;

namespace deckard::math
{


	export using float4 = sse::vec4;
	export using vec4   = sse::vec4; // vec_n<float, 4>;

	export using vec3 = vec_n<float, 3>;
	export using vec2 = vec_n<float, 2>;

	export using uvec2 = vec_n<u32, 2>;
	export using uvec3 = vec_n<u32, 3>;
	export using uvec4 = vec_n<u32, 4>;

	export using ivec2 = vec_n<i32, 2>;
	export using ivec3 = vec_n<i32, 3>;
	export using ivec4 = vec_n<i32, 4>;


	static_assert(sizeof(vec4) == 4 * 4, "vec4 size should be 4*4 bytes");
	static_assert(sizeof(vec3) == 4 * 3, "vec3 size should be 4*3 bytes");
	static_assert(sizeof(vec2) == 4 * 2, "vec2 size should be 4*2 bytes");


} // namespace deckard::math

// STD specials
export namespace std
{
	using namespace deckard::math;

	template<arithmetic T, size_t N>
	struct hash<vec_n<T, N>>
	{
		size_t operator()(const vec_n<T, N>& value) const { return deckard::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<arithmetic T, size_t N>
	struct formatter<vec_n<T, N>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const vec_n<T, N>& vec, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "vec{}(", N);

			if constexpr (std::is_integral_v<T>)
			{
				for (size_t i = 0; i < N; ++i)
					std::format_to(ctx.out(), "{}{}", vec[i], i < N - 1 ? "," : "");
			}
			else
			{
				for (size_t i = 0; i < N; ++i)
					std::format_to(ctx.out(), "{:.3f}{}", vec[i], i < N - 1 ? ", " : "");
			}

			return std::format_to(ctx.out(), ")");
		}
	};

	template<>
	struct hash<sse::vec4>
	{
		size_t operator()(const vec4& value) const { return deckard::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<>
	struct formatter<sse::vec4>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const sse::vec4& vec, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "vec4(");

			for (int i = 0; i < 4; ++i)
				std::format_to(ctx.out(), "{:.3f}{}", vec[i], i < 4 - 1 ? ", " : "");

			return std::format_to(ctx.out(), ")");
		}
	};


} // namespace std
