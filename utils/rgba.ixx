export module deckard.colors.rgba;
import std;
import deckard.types;
import deckard.colors;

namespace deckard
{


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
	
		/*
		operator std::array<u8,3> () const 
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

			return std::array{h, s, v};
		}
		*/
		
	};

	export struct rgba
	{
		u8 r = 0, g = 0, b = 0, a = 255;

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

		rgba(f32 red, f32 green, f32 blue, f32 alpha)
			: r(clamp_rgb(red))
			, g(clamp_rgb(green))
			, b(clamp_rgb(blue))
			, a(clamp_rgb(alpha))
		{
		}

		/*
		rgba(hsv hsv, u8 alpha)
		{
			rgb from_hsv = hsv;
			r            = from_hsv.r;
			g            = from_hsv.g;
			b            = from_hsv.b;
			a            = alpha;
		}
		*/

		// TODO: HSV
	};
}
