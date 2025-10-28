export module deckard.colors;

import std;
import deckard.types;

namespace deckard
{
#ifdef __cpp_lib_constexpr_cmath
#error "Use constexpr round";
#endif
	constexpr u8 clamp_rgb(f32 color) { return static_cast<u8>((std::clamp(color, 0.0f, 1.0f) * 255.0f) + 0.5f); }

	struct rgb
	{
		u8 r = 0, g = 0, b = 0;
	};

	struct hsv
	{
		f32 h = 0.0f, s = 0.0f, v = 0.0f;

		hsv(f32 hh, f32 ss, f32 vv)
			: h(std::clamp(hh, 0.0f, 360.0f))
			, s(std::clamp(ss, 0.0f, 1.0f))
			, v(std::clamp(vv, 0.0f, 1.0f))
		{
		}

		operator rgb() const
		{


			const u8 c = clamp_rgb(v * s);
			const u8 x = clamp_rgb(c * (1 - std::fabs(std::fmodf((h / 60), 2) - 1)));
			const u8 m = clamp_rgb(v - c);

			f32 rp, gp, bp;

			rgb ret;
			if (h >= 0 and h < 60)
			{
				rp = c;
				gp = x;
				bp = 0;
			}
			else if (h >= 60 and h < 120)
			{
				rp = x;
				gp = c;
				bp = 0;
			}
			else if (h >= 120 and h < 180)
			{
				rp = 0;
				gp = c;
				bp = x;
			}
			else if (h >= 180 and h < 240)
			{
				rp = 0;
				gp = x;
				bp = c;
			}
			else if (h > 240 and h < 300)
			{
				rp = x;
				gp = 0;
				bp = c;
			}
			else
			{
				rp = c;
				gp = 0;
				bp = x;
			}

			return {clamp_rgb((rp + m) * 255), clamp_rgb((gp + m) * 255), clamp_rgb((bp + m) * 255)};
		}
	};

	struct rgba
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

		rgba(hsv hsv, u8 alpha)
		{
			rgb from_hsv = hsv;
			r = from_hsv.r;
			g = from_hsv.g;
			b = from_hsv.b;
			a = alpha;
		}

		// TODO: HSV
	};

}; // namespace deckard
