export module deckard.colors.hsv;

import deckard.colors;
import deckard.colors.rgba;

import std;
import deckard.types;


namespace deckard
{

	
	export struct hsv
	{
		f32 h = 0.0f, s = 0.0f, v = 0.0f;

		hsv(f32 hh, f32 ss, f32 vv)
			: h(std::clamp(hh, 0.0f, 360.0f))
			, s(std::clamp(ss / 100.0f, 0.0f, 1.0f))
			, v(std::clamp(vv / 100.0f, 0.0f, 1.0f))
		{
		}

		operator std::array<u8, 3>() const
		{
			f32 hh = h;

			if (s == 0.0)
			{
				return {clamp_rgb(v), clamp_rgb(v), clamp_rgb(v)};
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

			return {};
		}

		bool operator==(const hsv& rhs) const { return h == rhs.h and s == rhs.s and v == rhs.v; }

		// RGB(0,0,0)		-> HSV(0,0,0)
		// RGB(255,0,0)		-> HSV(0,100,100)
		// RGB(0,255,0)		-> HSV(120,100,100)
		// RGB(0,0,255)		-> HSV(240,100,100)
		// RGB(255,0,255)	-> HSV(300,100,100)
		// RGB(255,255,255) -> HSV(0,0,100)

		// RGB(0,128,196) -> HSV(200.8, 100, 76.9)
	};
}
