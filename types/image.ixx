export module deckard.image;

import std;
import deckard.types;
import deckard.colors;
import deckard.assert;
import deckard.math.utils;

namespace deckard
{
	template<u64 Channels>
	class image_channels final
	{
		static_assert(Channels == 1 or Channels == 3 or Channels == 4, "Channels must be either 1(gray), 3 (RGB) or 4 (RGBA)");

	private:
		// conditional<Channels == 1, gray, std::conditional<Channels == 3, rgb, std::conditional<Channels == 4, rgba, ....>>
		using ColorType = std::conditional<Channels == 4, rgba, rgb>;


		std::vector<ColorType> data;
		u16                    width{0};
		u16                    height{0};

	public:
		explicit image_channels(u16 w, i16 h)
			: width(w)
			, height(h)
		{
			data.resize(width * height);
		}

		ColorType operator[](u16 x, u16 y) const
		{
			assert::check(x < width, "x-coordinate out-of-bounds");
			assert::check(y < height, "y-coordinate out-of-bounds");
			assert::check(x * y < data.size(), "index out of bounds");

			return data[math::index_from_2d(x, y, width)];
		}
	};

	export using image_gray = image_channels<1>;
	export using image_rgb  = image_channels<3>;
	export using image_rgba = image_channels<4>;


}; // namespace deckard
