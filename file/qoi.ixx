export module deckard.qoi;

import deckard.file;
import deckard.types;
import std;

namespace deckard
{
	struct qoi_header
	{
		u8  magic[4];   // magic bytes "qoif"
		u32 width;      // image width in pixels (BE)
		u32 height;     // image height in pixels (BE)
		u8  channels;   // 3 = RGB, 4 = RGBA
		u8  colorspace; // 0 = sRGB with linear alpha
						// 1 = all channels linear
	};

	export class qoi
	{
	private:
	public:
	};

}; // namespace deckard
