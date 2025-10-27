export module deckard.colors;

import std;
import deckard.types;

namespace deckard
{
	struct rgb
	{
		u8 r = 0, g = 0, b = 0;
	};

	 constexpr u8 clamp_color(f32 color) { return static_cast<u8>(std::clamp(color, 0.0f, 1.0f) * 255.0f + 0.5f); }

	struct rgba
	{
		u8 r = 0, g = 0, b = 0, a = 255;

		rgba(u8 red, u8 green, u8 blue, u8 alpha = 255)
			: r(red)
			, g(green)
			, b(blue)
			, a(alpha)
		{
		}

		rgba(f32 red, f32 green, f32 blue, f32 alpha = 1.0f)
			: r(clamp_color(red))
			, g(clamp_color(green))
			, b(clamp_color(blue))
			, a(clamp_color(alpha))
		{
		}

		// TODO: HSV
	};

}; // namespace deckard
