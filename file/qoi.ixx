export module deckard.qoi;

import deckard.file;
import deckard.types;
import deckard.enums;
import deckard.helpers;
import std;

namespace deckard
{
	// #pragma pack(1)
	struct qoi_header
	{
		u8  magic[4];   // magic bytes "qoif"
		u32 width;      // image width in pixels (BE)
		u32 height;     // image height in pixels (BE)
		u8  channels;   // 3 = RGB, 4 = RGBA
		u8  colorspace; // 0 = sRGB with linear alpha
						// 1 = all channels linear
	};

	constexpr u32 QOI_HEADER_SIZE = 14;

	using qoi_rgba_t = union
	{
		struct
		{
			unsigned char r, g, b, a;
		} rgba;

		unsigned int v;
	};


	enum class chunk_tag : u32
	{
		index = 0b0000'0000u,
		diff  = 0b0100'0000u,
		luma  = 0b1000'0000u,
		run   = 0b1100'0000u,
		rgb   = 0b1111'1110u,
		rgba  = 0b1111'1111u,
	};

	std::vector<u8> test_qoi()
	{
		// clang-format off
			std::vector<u8> test = make_vector<u8>(
			  0x71, 0x6F, 0x69, 0x66, // magic "qoif"
			  0x00, 0x00, 0x00, 0x03, // width: 3
			  0x00, 0x00, 0x00, 0x03, // height: 3
			  0x04,                   // channels: 4 (RGBA)
			  0x00,                   // colorspace: 0 (sRGB with linear alpha)

			  0xff, 0x00, 0x00, 0xff,  // QOI_OP_RGBA: Red   (255,   0,   0, 255)
			  0x00, 0xff, 0x00, 0xff,  // QOI_OP_RGBA: Green (  0, 255,   0, 255)
			  0x00, 0x00, 0xff, 0xff,  // QOI_OP_RGBA: Blue  (  0,   0, 255, 255)
  
			  0xff, 0xff, 0x00, 0xff,  // QOI_OP_RGBA: Yellow  (255,255,0,255)
			  0xff, 0x00, 0xff, 0xff,  // QOI_OP_RGBA: Magenta (255,0,255,255)
			  0x00, 0xff, 0xff, 0xff,  // QOI_OP_RGBA: Cyan    (0,255,255,255)
  			  
			  0xff, 0xff, 0xff, 0xff,  // QOI_OP_RGBA: White (255,255,255,255)
			  0x80, 0x80, 0x80, 0xff,  // QOI_OP_RGBA: Gray (128,128,128,255)
			  0x00, 0x00, 0x00, 0xff,  // QOI_OP_RGBA: Black (0,0,0,255)
  			  
			  // End marker (8 bytes)
			  0x00, 0x00, 0x00, 0x00,
			  0x00, 0x00, 0x00, 0x01
			);
		// clang-format on
		return test;
	}

	// static_assert(14 == sizeof(qoi_header));


	struct image
	{
	};

	export class qoi
	{
	private:
		qoi_header         header{};
		std::array<u8, 64> running_window{0};

		u8 index_position(u8 r, u8 g, u8 b, u8 a) { return (r * 3 + g * 5 + b * 7 + a * 11) % 64; }

	public:
		std::expected<bool, std::string> read(const std::span<u8> buffer)
		{
			if (buffer.size() < QOI_HEADER_SIZE)
				return std::unexpected("Invalid QOI");

			if (not std::ranges::equal(buffer.subspan(0, 4), std::array{'q', 'o', 'i', 'f'}))
				return std::unexpected("Invalid QOI");

			std::memcpy(&header, buffer.data(), QOI_HEADER_SIZE);


			header.width      = std::byteswap(header.width);
			header.height     = std::byteswap(header.height);
			header.channels   = header.channels;
			header.colorspace = header.colorspace;
			_                 = 0;


			return {};
		}

		void test()
		{
			auto header = test_qoi();
			read(to_span(header));


			qoi_rgba_t r;

			_ = 0;


			//
		}
	};

	// pgm to qoi
	// array2d to qoi

}; // namespace deckard
