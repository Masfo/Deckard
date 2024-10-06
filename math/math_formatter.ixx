export module deckard.math:formatter;

import :vec.generic;
import :vec2_sse;
import :vec3_sse;
import :vec4_sse;
import :vec_sse_generic;
import :matrix;
import :quaternion;


import std;


import deckard.utils.hash;

namespace std
{
	using namespace deckard::math;

	// vec 2 formatter
	template<>
	struct hash<sse::vec2>
	{
		size_t operator()(const sse::vec2& value) const { return deckard::utils::hash_values(value[0], value[1]); }
	};

	template<>
	struct formatter<sse::vec2>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const sse::vec2& vec, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "vec2({:.5f}, {:.5f})", vec.data.c.x, vec.data.c.y);
		}
	};

	// vec 3 formatter
	template<>
	struct hash<sse::vec3>
	{
		size_t operator()(const sse::vec3& value) const { return deckard::utils::hash_values(value[0], value[1], value[2]); }
	};

	template<>
	struct formatter<sse::vec3>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const sse::vec3& vec, std::format_context& ctx) const
		{
			return std::format_to(ctx.out(), "vec3({:.5f}, {:.5f}, {:.5f})", vec.data.c.x, vec.data.c.y, vec.data.c.z);
		}
	};

	// vec 4 formatter
	template<>
	struct hash<sse::vec4>
	{
		size_t operator()(const sse::vec4& value) const { return deckard::utils::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<>
	struct formatter<sse::vec4>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const sse::vec4& vec, std::format_context& ctx) const
		{
			return std::format_to(
			  ctx.out(), "vec4({:.5f}, {:.5f}, {:.5f}, {:.5f})", vec.data.c.x, vec.data.c.y, vec.data.c.z, vec.data.c.w);
		}
	};

	// quat formatter
	template<>
	struct hash<quat>
	{
		size_t operator()(const quat& value) const { return deckard::utils::hash_values(value[0], value[1], value[2], value[3]); }
	};

	template<>
	struct formatter<quat>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const quat& vec, std::format_context& ctx) const
		{
			return std::format_to(
			  ctx.out(), "quat({:.5f}, {:.5f}, {:.5f}, {:.5f})", vec.data.c.w, vec.data.c.x, vec.data.c.y, vec.data.c.z);
		}
	};

#if 0
	// vec_n_sse formatter
	template<size_t N>
	struct hash<sse::vec_n_sse<N>>
	{
		size_t operator()(const sse::vec_n_sse<N>& value) const
		{
			return deckard::utils::hash_values(value[0], value[1], value[2], value[3]);
		}
	};

	template<size_t N>
	struct formatter<sse::vec_n_sse<N>>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const sse::vec_n_sse<N>& vec, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "vec{}(", N);

			for (size_t i = 0; i < N; ++i)
				std::format_to(ctx.out(), "{:.3f}{}", vec[i], i < N - 1 ? ", " : "");

			return std::format_to(ctx.out(), ")");
		}
	};
#endif


	template<>
	struct hash<mat4_generic>
	{
		size_t operator()(const mat4_generic& value) const { return deckard::utils::hash_values(value); }
	};

	template<>
	struct formatter<mat4_generic>
	{
		// TODO: Parse single or multi row?
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const mat4_generic& m, std::format_context& ctx) const
		{
			std::format_to(ctx.out(), "mat4(({:.5f}, {:.5f}, {:.5f}, {:.5f}),\n", m[0], m[1], m[2], m[3]);
			std::format_to(ctx.out(), "     ({:.5f}, {:.5f}, {:.5f}, {:.5f}),\n", m[4], m[5], m[6], m[7]);
			std::format_to(ctx.out(), "     ({:.5f}, {:.5f}, {:.5f}, {:.5f}),\n", m[8], m[9], m[10], m[11]);
			std::format_to(ctx.out(), "     ({:.5f}, {:.5f}, {:.5f}, {:.5f})", m[12], m[13], m[14], m[15]);
			return std::format_to(ctx.out(), ")");
		}
	};

	// Big num

	template<>
	struct hash<deckard::math::bignum>
	{
		size_t operator()(const deckard::math::bignum& value) const { return deckard::utils::hash_values(value); }
	};

	template<>
	struct formatter<deckard::math::bignum>
	{
		// TODO: Parse leading zeros
		constexpr auto parse(std::format_parse_context& ctx) const { return ctx.begin(); }

		auto format(const deckard::math::bignum& bn, std::format_context& ctx)
		{
			(ctx);
			return bn.print();

#if 0
			for (int i = bn.size() - 1; i >= 0; i--)
			{
				if (leading)
				{
					std::format_to(ctx.out(), "{}", bn[i]);
					leading = 0;
				}
				else
				{
					std::format_to(ctx.out(), "{:09}", bn[i]);
				}
			}
			if (leading)
			{
				std::format_to(ctx.out(), "0");
			}
			return std::format_to(ctx.out(), "");
#endif
		}

		bool leading{true};
	};


} // namespace std
