export module deckard.colors;


import std;
import deckard.types;
import deckard.math.utils;
import deckard.math;
import deckard.utils.hash;

namespace deckard
{
#ifdef __cpp_lib_constexpr_cmath
#error "Use constexpr round";
#endif

	export constexpr u8 clamp_rgb(f32 color) { return static_cast<u8>((std::clamp(color, 0.0f, 1.0f) * 255.0f) + 0.5f); }

	export constexpr u8 clamp_rgb(i32 color) { return static_cast<u8>(std::clamp(color, 0, 255)); }

	export std::array<u8, 3> to_rgb(f32 h, f32 s, f32 v)
	{
		f32 hh = h;

		if (s == 0.0)
		{
			auto vt = clamp_rgb(v);
			return {vt, vt, vt};
		}
		else
		{
			hh = fmod(hh, 360.0f);
			hh /= 60.0f;
			int i = static_cast<int>(std::floor(hh));
			f32 f = hh - i;
			f32 p = v * (1.0f - s);
			f32 q = v * (1.0f - s * f);
			f32 t = v * (1.0f - s * (1.0f - f));

			switch (i)
			{
				case 0: return {clamp_rgb(v), clamp_rgb(t), clamp_rgb(p)}; break;
				case 1: return {clamp_rgb(q), clamp_rgb(v), clamp_rgb(p)}; break;
				case 2: return {clamp_rgb(p), clamp_rgb(v), clamp_rgb(t)}; break;
				case 3: return {clamp_rgb(p), clamp_rgb(q), clamp_rgb(v)}; break;
				case 4: return {clamp_rgb(t), clamp_rgb(p), clamp_rgb(v)}; break;
				default: return {clamp_rgb(v), clamp_rgb(p), clamp_rgb(q)}; break;
			}
		}
	}

	export auto to_rgb(const std::array<f32, 3>& hsv) { return to_rgb(hsv[0], hsv[1], hsv[2]); }

	export std::array<u32, 4> to_rgba(const std::array<f32, 3>& hsv)
	{
		auto to = to_rgb(hsv);
		return {to[0], to[1], to[2], 255_u8};
	}

	export std::array<f32, 3> to_hsv(u8 r, u8 g, u8 b)
	{
		f32 rr = r;
		f32 gg = g;
		f32 bb = b;

		f32 cmax  = std::max({rr, gg, bb});
		f32 cmin  = std::min({rr, gg, bb});
		f32 delta = cmax - cmin;

		f32 h = 0.0;

		if (delta == 0)
			h = 0;
		else if (cmax == rr)
			h = fmod(((gg - bb) / delta), 6.0f);
		else if (cmax == gg)
			h = ((bb - rr) / delta) + 2.0f;
		else
			h = ((rr - gg) / delta) + 4.0f;

		h *= 60.0f;
		if (h < 0)
			h += 360.0f;

		f32 s = (cmax == 0) ? 0 : (delta / cmax);
		f32 v = cmax;

		return {h, s, v};
	}

	constexpr u8 mul_clamp(u8 x, u8 y)
	{
		i32 prod = (static_cast<i32>(x) * static_cast<i32>(y)) / 255;
		return static_cast<u8>(prod);
	}

	constexpr u8 div_clamp(u8 x, u8 y)
	{
		if (y == 0)
			return 255;

		i32 value = static_cast<i32>((255.0f * x) / y);
		return static_cast<u8>(std::clamp(value, 0, 255));
	}

	constexpr u8 add_clamp(u8 x, u8 y)
	{
		i32 sum = static_cast<i32>(x) + static_cast<i32>(y);
		return static_cast<u8>(std::min(sum, 255));
	}

	constexpr u8 sub_clamp(u8 x, u8 y)
	{
		i32 diff = static_cast<i32>(x) - static_cast<i32>(y);
		return static_cast<u8>(std::max(diff, 0));
	}

	export struct hsv
	{
		f32 h = 0.0f, s = 0.0f, v = 0.0f;

		hsv(f32 hh, f32 ss, f32 vv)
			: h(std::clamp(hh, 0.0f, 360.0f))
			, s(std::clamp(ss / 100.0f, 0.0f, 1.0f))
			, v(std::clamp(vv / 100.0f, 0.0f, 1.0f))
		{
		}

		hsv(u8 red, u8 green, u8 blue) { auto to = to_hsv(red, green, blue); }

		std::array<f32, 3> data() const { return {h, s, v}; }

		// RGB(0,0,0)		-> HSV(0,0,0)
		// RGB(255,0,0)		-> HSV(0,100,100)
		// RGB(0,255,0)		-> HSV(120,100,100)
		// RGB(0,0,255)		-> HSV(240,100,100)
		// RGB(255,0,255)	-> HSV(300,100,100)
		// RGB(255,255,255) -> HSV(0,0,100)

		// RGB(0,128,196) -> HSV(200.8, 100, 76.9)
	};

	export struct rgb
	{
		u8 r = 0, g = 0, b = 0;
		rgb() = default;

		rgb(u8 red, u8 green, u8 blue)
			: r(red)
			, g(green)
			, b(blue)
		{
		}

		bool operator==(const rgb& rhs) const { return r == rhs.r and g == rhs.g and b == rhs.b; }
	};

	export struct rgba
	{
		u8 r = 0, g = 0, b = 0, a = 255;

		rgba() = default;

		rgba(std::array<u8, 3>& color, u8 alpha)
			: r(color[0])
			, g(color[1])
			, b(color[2])
			, a(alpha)
		{
		}

		rgba(rgb color, u8 alpha)
			: r(color.r)
			, g(color.g)
			, b(color.b)
			, a(alpha)
		{
		}

		rgba(u8 red, u8 green, u8 blue, u8 alpha)
			: r(red)
			, g(green)
			, b(blue)
			, a(alpha)
		{
		}

		rgba(i32 red, i32 green, i32 blue, i32 alpha)
			: r(clamp_rgb(red))
			, g(clamp_rgb(green))
			, b(clamp_rgb(blue))
			, a(clamp_rgb(alpha))
		{
		}

		rgba(i32 red, i32 green, i32 blue)
			: r(clamp_rgb(red))
			, g(clamp_rgb(green))
			, b(clamp_rgb(blue))
			, a(255)
		{
		}

		rgba(f32 red, f32 green, f32 blue, f32 alpha)
			: r(clamp_rgb(red))
			, g(clamp_rgb(green))
			, b(clamp_rgb(blue))
			, a(clamp_rgb(alpha))
		{
		}

		rgba(rgb rgb, f32 alpha)
			: r(clamp_rgb(rgb.r))
			, g(clamp_rgb(rgb.g))
			, b(clamp_rgb(rgb.b))
			, a(clamp_rgb(alpha))
		{
		}

		rgba operator*(const rgba& rhs) const
		{
			rgba ret;
			ret.r = mul_clamp(r, rhs.r);
			ret.g = mul_clamp(g, rhs.g);
			ret.b = mul_clamp(b, rhs.b);
			ret.a = mul_clamp(a, rhs.a);

			return ret;
		}

		rgba operator/(const rgba& rhs) const
		{
			rgba ret;
			ret.r = div_clamp(r, rhs.r);
			ret.g = div_clamp(g, rhs.g);
			ret.b = div_clamp(b, rhs.b);
			ret.a = div_clamp(a, rhs.a);

			return ret;
		}

		rgba operator+(const rgba& rhs) const
		{
			return rgba(add_clamp(r, rhs.r), add_clamp(g, rhs.g), add_clamp(b, rhs.b), add_clamp(a, rhs.a));
		}

		rgba operator-(const rgba& rhs) const
		{
			return {sub_clamp(r, rhs.r), sub_clamp(g, rhs.g), sub_clamp(b, rhs.b), sub_clamp(a, rhs.a)};
		}

		bool operator==(const rgba& rhs) const { return r == rhs.r and g == rhs.g and b == rhs.b and a == rhs.a; }
	};

	u8 mix(u8 a, u8 b, f32 t)
	{
		//
		return static_cast<u8>(a + t * (b - a));
		// return static_cast<u8>((std::round(1.0f - t) * a + t * b));
	}

	export rgba lerp(const rgba& a, const rgba& b, f32 t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		return {mix(a.r, b.r, t), mix(a.g, b.g, t), mix(a.b, b.b, t), mix(a.a, b.a, t)};
	}

	bool equal(const hsv& lhs, const rgb& rhs)
	{
		const auto c = to_rgb(lhs.data());
		return rhs.r == c[0] and rhs.g == c[1] and rhs.b == c[2];
	}

	bool equal(const hsv& lhs, const rgba& rhs)
	{
		const auto c = to_rgba(lhs.data());
		return rhs.r == c[0] and rhs.g == c[1] and rhs.b == c[2] and rhs.a == c[3];
	}

	//

	export bool operator==(const hsv& lhs, const rgb& rhs) { return equal(lhs, rhs); }

	export bool operator==(const hsv& lhs, const rgba& rhs) { return equal(lhs, rhs); }


}; // namespace deckard

export namespace std
{
	using namespace deckard;

	template<>
	struct hash<rgb>
	{
		size_t operator()(const rgb& value) const { return deckard::utils::hash_values(value.r, value.g, value.b); }
	};

	template<>
	struct hash<rgba>
	{
		size_t operator()(const rgba& value) const { return deckard::utils::hash_values(value.r, value.g, value.b, value.a); }
	};

	template<>
	struct hash<hsv>
	{
		size_t operator()(const hsv& value) const { return deckard::utils::hash_values(value.h, value.s, value.v); }
	};

	template<>
	struct formatter<rgb>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const rgb& v, std::format_context& ctx) const { return std::format_to(ctx.out(), "rgb({}, {}, {})", v.r, v.g, v.b); }
	};

	template<>
	struct formatter<rgba>
	{
		bool show_as_float = false;
		bool show_as_hex   = false;
		bool show_upper    = false;

		constexpr auto parse(std::format_parse_context& ctx)
		{
			auto pos = ctx.begin();

			while (pos != ctx.end() and *pos != '}')
			{
				if (*pos == 'f' or *pos == 'F')
					show_as_float = true;
				else if (*pos == 'x')
					show_as_hex = true;
				else if (*pos == 'X')
				{
					show_as_hex = true;
					show_upper  = true;
				}

				pos++;
			}

			if (pos != ctx.end() and *pos != '}')
				throw std::format_error("Invalid format");


			return pos;
		}

		auto format(const rgba& v, std::format_context& ctx) const
		{
			if (show_as_float)
			{
				std::format_to(ctx.out(), "rgba(");

				f32 r = v.r / 255.0f;
				f32 g = v.g / 255.0f;
				f32 b = v.b / 255.0f;
				f32 a = v.a / 255.0f;


				// r
				if (math::is_close_enough_zero(r) or math::is_close_enough_one(r))
					std::format_to(ctx.out(), "{:.1f}, ", r);
				else
					std::format_to(ctx.out(), "{:.3f}, ", r);

				// g
				if (math::is_close_enough_zero(g) or math::is_close_enough_one(g))
					std::format_to(ctx.out(), "{:.1f}, ", g);
				else
					std::format_to(ctx.out(), "{:.3f}, ", g);

				// b
				if (math::is_close_enough_zero(b) or math::is_close_enough_one(b))
					std::format_to(ctx.out(), "{:.1f}, ", b);
				else
					std::format_to(ctx.out(), "{:.3f}, ", b);

				// a
				if (math::is_close_enough_zero(a) or math::is_close_enough_one(a))
					std::format_to(ctx.out(), "{:.1f}", a);
				else
					std::format_to(ctx.out(), "{:.3f}", a);

				return std::format_to(ctx.out(), ")");
			}

			if (show_as_hex)
			{
				if (show_upper)
					return std::format_to(ctx.out(), "rgba(#{:02X}{:02X}{:02X}{:02X})", v.r, v.g, v.b, v.a);

				return std::format_to(ctx.out(), "rgba(#{:02x}{:02x}{:02x}{:02x})", v.r, v.g, v.b, v.a);
			}


			return std::format_to(ctx.out(), "rgba({}, {}, {}, {})", v.r, v.g, v.b, v.a);
		}
	};

	template<>
	struct formatter<hsv>
	{
		// TODO: Parse width
		constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

		auto format(const hsv& v, std::format_context& ctx) const { return std::format_to(ctx.out(), "hsv({}, {}, {})", v.h, v.s, v.v); }
	};
} // namespace std
