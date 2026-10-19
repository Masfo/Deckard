export module deckard.image:bmp;

import std;
import deckard.types;
import deckard.file;
import deckard.assert;
import deckard.helpers;
import :common;

namespace deckard::image
{
	// ###############################################################################################
	// BMP ###########################################################################################


	export template<typename Img=image_rgba>
	std::expected<std::vector<u8>, std::string> encode_bmp(const Img& img)
	{
		static_assert(std::is_same_v<Img, image_rgb> or std::is_same_v<Img, image_rgba>,
					  "encode_bmp supports image_rgb or image_rgba");

		constexpr bool is_rgba         = std::is_same_v<Img, image_rgba>;
		constexpr u32  bytes_per_pixel = is_rgba ? 4 : 3;
		constexpr u16  bpp             = is_rgba ? 32 : 24;

		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return std::unexpected("encode_bmp: image has zero dimensions");

		const u32     row_stride  = width * bytes_per_pixel;
		const u32     row_padding = (4u - (row_stride % 4u)) % 4u; // always 0 for 32-bit
		const u32     pixel_bytes = (row_stride + row_padding) * height;
		constexpr u32 header_bytes = 14u + 40u;
		const u32     file_bytes   = header_bytes + pixel_bytes;

		std::vector<u8> buffer;
		buffer.reserve(file_bytes);

		auto push_le = [&](u64 value, u32 byte_count)
		{
			for (u32 i = 0; i < byte_count; ++i)
				buffer.push_back(static_cast<u8>((value >> (i * 8u)) & 0xFFu));
		};

		// BMP file header (14 bytes)
		buffer.push_back(static_cast<u8>('B'));
		buffer.push_back(static_cast<u8>('M'));
		push_le(file_bytes, 4);             // file size
		push_le(0u, 2);                     // reserved 1
		push_le(0u, 2);                     // reserved 2
		push_le(header_bytes, 4);           // pixel data offset

		// DIB header (BITMAPINFOHEADER, 40 bytes)
		push_le(40u, 4);                    // header size
		push_le(width, 4);                  // width
		push_le(height, 4);                 // height (positive => bottom-up)
		push_le(1u, 2);                     // planes
		push_le(bpp, 2);                    // bits per pixel (24 = BGR, 32 = BGRA)
		push_le(0u, 4);                     // compression (BI_RGB)
		push_le(pixel_bytes, 4);            // image size
		push_le(2835u, 4);                  // x pixels per meter
		push_le(2835u, 4);                  // y pixels per meter
		push_le(0u, 4);                     // colors used
		push_le(0u, 4);                     // important colors

		// Pixels, bottom-up default orientation, BGR(A) order, rows padded to 4 bytes
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const auto& c = img.at(static_cast<u16>(x), static_cast<u16>(y));
				buffer.push_back(static_cast<u8>(c.b));
				buffer.push_back(static_cast<u8>(c.g));
				buffer.push_back(static_cast<u8>(c.r));
				if constexpr (is_rgba)
					buffer.push_back(static_cast<u8>(c.a));
			}
			for (u32 k = 0; k < row_padding; ++k)
				buffer.push_back(0);
		}

		return buffer;
	}

	export template<typename Img = image_rgba>
	std::expected<void, std::string> save_bmp(std::filesystem::path path, const Img& img)
	{
		auto encoded = encode_bmp(img);
		if (not encoded)
			return std::unexpected(encoded.error());

		const auto& out    = encoded.value();
		auto        result = file::write({.filename = path, .buffer = std::span<const u8>{out.data(), out.size()}});
		if (not result)
			return std::unexpected("save_bmp: failed to write file");
		if (*result != out.size())
			return std::unexpected("save_bmp: written size mismatch");
		return {};
	}

	export template<typename Img = image_rgba>
	std::expected<Img, std::string> decode_bmp(std::span<const u8> buffer)
	{
		static_assert(std::is_same_v<Img, image_rgb> or std::is_same_v<Img, image_rgba>,
					  "decode_bmp supports image_rgb or image_rgba");

		if (buffer.empty())
			return std::unexpected("decode_bmp: buffer is empty");
		if (buffer.size() < 14u + 40u)
			return std::unexpected("decode_bmp: buffer too small for BMP header");

		auto u16_le = [&](size_t off) -> u16
		{ return static_cast<u16>(static_cast<u16>(buffer[off]) | (static_cast<u16>(buffer[off + 1]) << 8)); };
		auto u32_le = [&](size_t off) -> u32
		{
			return static_cast<u32>(buffer[off]) | (static_cast<u32>(buffer[off + 1]) << 8)
				   | (static_cast<u32>(buffer[off + 2]) << 16) | (static_cast<u32>(buffer[off + 3]) << 24);
		};
		auto i32_le = [&](size_t off) -> i32 { return static_cast<i32>(u32_le(off)); };

		if (buffer[0] != static_cast<u8>('B') || buffer[1] != static_cast<u8>('M'))
			return std::unexpected("decode_bmp: invalid BMP file header");

		const u32 file_size    = u32_le(2);
		const u32 pixel_offset = u32_le(10);
		if (file_size != 0 && file_size > buffer.size())
			return std::unexpected("decode_bmp: file size mismatch");
		if (pixel_offset >= buffer.size())
			return std::unexpected("decode_bmp: pixel offset out of bounds");

		const u32 dib_size = u32_le(14);
		if (dib_size < 40u)
			return std::unexpected("decode_bmp: DIB header too small");

		const i32 width_i  = i32_le(18);
		const i32 height_i = i32_le(22);
		if (width_i <= 0 || height_i == 0)
			return std::unexpected("decode_bmp: invalid image dimensions");

		const bool top_down = height_i < 0;
		const u32  width    = static_cast<u32>(width_i);
		const u32  height   = static_cast<u32>(top_down ? -height_i : height_i);

		const u16 planes   = u16_le(26);
		const u16 bpp      = u16_le(28);
		const u32 compress = u32_le(30);
		if (planes != 1u)
			return std::unexpected("decode_bmp: invalid number of planes");
		if (bpp != 24u and bpp != 32u)
			return std::unexpected("decode_bmp: only 24-bit and 32-bit BMP are supported");
		if (compress != 0u) // BI_RGB
			return std::unexpected("decode_bmp: unsupported compression method");

		const u32 bytes_per_pixel = static_cast<u32>(bpp / 8u);
		const u32 row_stride      = width * bytes_per_pixel;
		const u32 row_padding     = (4u - (row_stride % 4u)) % 4u; // always 0 for 32-bit
		const u32 row_bytes       = row_stride + row_padding;
		const u64 needed          = static_cast<u64>(pixel_offset) + static_cast<u64>(row_bytes) * height;
		if (needed > buffer.size())
			return std::unexpected("decode_bmp: buffer too small for pixel data");

		constexpr bool is_rgba = std::is_same_v<Img, image_rgba>;

		Img   img(width, height);
		auto* out = img.data().data();

		for (u32 y = 0; y < height; ++y)
		{
			const u32 src_y   = top_down ? y : (height - 1 - y);
			const u8* row_ptr = buffer.data() + static_cast<size_t>(pixel_offset) + static_cast<size_t>(src_y) * row_bytes;
			auto*     dst     = out + static_cast<usize>(y) * static_cast<usize>(width);

			for (u32 x = 0; x < width; ++x)
			{
				const u8* p = row_ptr + x * bytes_per_pixel;
				if constexpr (is_rgba)
				{
					if (bpp == 32u)
						dst[x] = rgba{p[2], p[1], p[0], p[3]};
					else
						dst[x] = rgba{p[2], p[1], p[0], 255u};
				}
				else
				{
					dst[x] = rgb{p[2], p[1], p[0]}; // drop alpha when 32-bit
				}
			}
		}

		return img;
	}

	export template<typename Img = image_rgba>
	std::expected<Img, std::string> load_bmp(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_bmp<Img>(bytes);
	}
}
