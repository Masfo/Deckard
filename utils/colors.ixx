export module deckard.colors;


import std;
import deckard.types;
import deckard.math.utils;

namespace deckard
{
#ifdef __cpp_lib_constexpr_cmath
#error "Use constexpr round";
#endif


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

	export constexpr u8 clamp_rgb(f32 color) { return static_cast<u8>((std::clamp(color, 0.0f, 1.0f) * 255.0f) + 0.5f); }

	export constexpr u8 clamp_rgb(i32 color) { return static_cast<u8>(std::clamp(color, 0, 255)); }

	export struct hsv
	{
		f32 h = 0.0f, s = 0.0f, v = 0.0f;

		hsv(f32 hh, f32 ss, f32 vv)
			: h(std::clamp(hh, 0.0f, 360.0f))
			, s(std::clamp(ss / 100.0f, 0.0f, 1.0f))
			, v(std::clamp(vv / 100.0f, 0.0f, 1.0f))
		{
		}

		hsv(u8 red, u8 green, u8 blue)
		{ 
			auto to = to_hsv({red, green, blue });

		}

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
	};

	export struct rgba
	{
		u8 r = 0, g = 0, b = 0, a = 255;

		rgba() = default;

		rgba(rgb color, u8 alpha)
			: r(color.r)
			, g(color.g)
			, b(color.b)
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

		rgba operator*(const rgba& rhs) const
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

	export rgb to_rgb(const hsv& hsv)
	{
		f32 hh = hsv.h;

		if (hsv.s == 0.0)
		{
			auto vt = clamp_rgb(hsv.v);
			return {vt, vt, vt};
		}
		else
		{
			hh = fmod(hh, 360.0f);
			hh /= 60.0f;
			int i = static_cast<int>(std::floor(hh));
			f32 f = hh - i;
			f32 p = hsv.v * (1.0f - hsv.s);
			f32 q = hsv.v * (1.0f - hsv.s * f);
			f32 t = hsv.v * (1.0f - hsv.s * (1.0f - f));

			switch (i)
			{
				case 0: return {clamp_rgb(hsv.v), clamp_rgb(t), clamp_rgb(p)}; break;
				case 1: return {clamp_rgb(q), clamp_rgb(hsv.v), clamp_rgb(p)}; break;
				case 2: return {clamp_rgb(p), clamp_rgb(hsv.v), clamp_rgb(t)}; break;
				case 3: return {clamp_rgb(p), clamp_rgb(q), clamp_rgb(hsv.v)}; break;
				case 4: return {clamp_rgb(t), clamp_rgb(p), clamp_rgb(hsv.v)}; break;
				default: return {clamp_rgb(hsv.v), clamp_rgb(p), clamp_rgb(q)}; break;
			}
		}
	}

	export rgba to_rgba(const hsv& hsv) { return rgba(to_rgb(hsv), 255_u8); }

	export hsv to_hsv(const rgb& rgb)
	{
		f32 rr = rgb.r;
		f32 gg = rgb.g;
		f32 bb = rgb.b;

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

	bool equal(const hsv& lhs, const rgb& rhs)
	{
		const rgb c = to_rgb(lhs);
		return rhs.b == c.b and rhs.g == c.g and rhs.r == c.r;
	}

	export bool operator==(const hsv& lhs, const rgb& rhs) { return equal(lhs, rhs); }

	bool equal(const hsv& lhs, const rgba& rhs)
	{
		const rgba c = to_rgba(lhs);
		return rhs.b == c.b and rhs.g == c.g and rhs.r == c.r;
	}

	export bool operator==(const hsv& lhs, const rgba& rhs) { return equal(lhs, rhs); }


}; // namespace deckard
