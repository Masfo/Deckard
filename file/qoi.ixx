export module deckard.qoi;

import deckard.file;
import deckard.types;
import deckard.enums;
import std;

namespace deckard
{
	struct alignas(8) qoi_header
	{
		u8  magic[4];   // magic bytes "qoif"
		u32 width;      // image width in pixels (BE)
		u32 height;     // image height in pixels (BE)
		u8  channels;   // 3 = RGB, 4 = RGBA
		u8  colorspace; // 0 = sRGB with linear alpha
						// 1 = all channels linear
	};

	static_assert(16 == sizeof(qoi_header));

	constexpr auto qoi_size = sizeof(qoi_header);

	// Quite Ok Modified: QOM

	enum class QOMFlags : u8
	{
		RGB  = BIT(0),
		RGBA = BIT(1),

		UNUSED1 = BIT(2),
		UNUSED2 = BIT(3),
		UNUSED3 = BIT(4),
		UNUSED4 = BIT(5),
		UNUSED5 = BIT(6),
		UNUSED6 = BIT(7),

	};
	consteval void enable_bitmask_operations(QOMFlags);

	struct alignas(8) qom_header
	{
		u8       magic[3]; // QOM
		QOMFlags flags[1]; //

		u16 width;         // 16384 max
		u16 height;
	};

	static_assert(8 == sizeof(qom_header));
	constexpr auto qom_size = sizeof(qom_header);

	// TODO: modified QOI,
	//	* combine magic with flags
	//		QOI[f] - f for 8-bit flag
	//
	//  * only sRGB
	//  * u16 extent (16384 max resolution)


	export class qoi
	{
	private:
	public:
	};

	// pgm to qoi
	// array2d to qoi

}; // namespace deckard
