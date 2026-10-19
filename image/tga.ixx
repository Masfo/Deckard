export module deckard.image:tga;

import std;
import deckard.types;
import deckard.file;
import deckard.assert;
import deckard.helpers;

import :common;

namespace deckard::image
{
	namespace fs = std::filesystem;

	// ###############################################################################################
	// TGA ###########################################################################################


	// Defaults to image_rgba so 32-bit files keep their alpha channel, matching load_qoi /
	// load_png. Pass image_rgb explicitly to force a 24-bit result.
	export template<typename Img = image_rgba>
	std::expected<Img, std::string> decode_tga(std::span<const u8> buffer)
	{
		static_assert(std::is_same_v<Img, image_rgb> or std::is_same_v<Img, image_rgba>,
					  "decode_tga supports image_rgb or image_rgba");

		if (buffer.empty())
			return std::unexpected("decode_tga: buffer is empty");
		if (buffer.size() < 18u)
			return std::unexpected("decode_tga: buffer too small");

		auto u16_le = [&](size_t off) -> u16
		{ return static_cast<u16>(static_cast<u16>(buffer[off + 0]) | (static_cast<u16>(buffer[off + 1]) << 8)); };

		const u8  id_length      = buffer[0];
		const u8  color_map_type = buffer[1];
		const u8  image_type     = buffer[2];
		const u16 cmap_length    = u16_le(5);
		const u8  cmap_depth     = buffer[7];
		const u16 width          = u16_le(12);
		const u16 height         = u16_le(14);
		const u8  pixel_depth    = buffer[16];
		const u8  descriptor     = buffer[17];


		if (width == 0 or height == 0)
			return std::unexpected("decode_tga: image has zero dimensions");
		
		if (width > Img::MAX_WIDTH or height > Img::MAX_HEIGHT)
			return std::unexpected("decode_tga: image dimensions exceed maximum allowed size");

		if (color_map_type != 0)
			return std::unexpected("decode_tga: color map type not supported");

		if (cmap_length != 0 or cmap_depth != 0)
			return std::unexpected("decode_tga: color map length or depth not supported");

		if (image_type != 2)
			return std::unexpected("decode_tga: only uncompressed true-color images are supported");

		if (pixel_depth != 24 and pixel_depth != 32)
			return std::unexpected("decode_tga: only 24-bit and 32-bit pixel depth are supported");

		const bool origin_top = (descriptor & 0b0010'0000) != 0;


		const size_t header_bytes = 18u + static_cast<size_t>(id_length);
		if (buffer.size() < header_bytes)
			return std::unexpected("decode_tga: buffer too small for header and ID field");

		const size_t bytes_per_pixel = static_cast<size_t>(pixel_depth / 8);
		const size_t pixel_bytes     = static_cast<size_t>(width) * static_cast<size_t>(height) * bytes_per_pixel;
		if (buffer.size() < header_bytes + pixel_bytes)
			return std::unexpected("decode_tga: buffer too small for pixel data");

		constexpr bool is_rgba = std::is_same_v<Img, image_rgba>;

		Img   img(width, height);
		auto* out = img.data().data();

		const u8*   src    = buffer.data() + header_bytes;
		const usize stride = static_cast<usize>(width) * bytes_per_pixel;
		const bool  flip   = not origin_top;

		for (u16 y = 0; y < height; ++y)
		{
			const usize src_y = flip ? static_cast<usize>(height - 1 - y) : static_cast<usize>(y);
			const u8*   s     = src + src_y * stride;
			auto*       d     = out + static_cast<usize>(y) * static_cast<usize>(width);

			if constexpr (is_rgba)
			{
				if (pixel_depth == 32)
				{
					for (u16 x = 0; x < width; ++x, s += 4)
						*d++ = rgba{s[2], s[1], s[0], s[3]};
				}
				else
				{
					for (u16 x = 0; x < width; ++x, s += 3)
						*d++ = rgba{s[2], s[1], s[0], 255u};
				}
			}
			else
			{
				if (pixel_depth == 32)
				{
					for (u16 x = 0; x < width; ++x, s += 4)
						*d++ = rgb{s[2], s[1], s[0]}; // drop alpha
				}
				else
				{
					for (u16 x = 0; x < width; ++x, s += 3)
						*d++ = rgb{s[2], s[1], s[0]};
				}
			}
		}

		return img;
	}

	export template<typename Img = image_rgba>
	std::expected<Img, std::string> load_tga(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_tga<Img>(bytes);
	}

	// Named helpers mirroring dif.ixx's load_dif_rgb / load_dif_rgba when a concrete image
	// type is wanted without spelling out the template argument.
	export std::expected<image_rgb, std::string> load_tga_rgb(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_tga<image_rgb>(bytes);
	}

	export std::expected<image_rgba, std::string> load_tga_rgba(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_tga<image_rgba>(bytes);
	}


	export template<typename Img>
	std::expected<std::vector<u8>, std::string> encode_tga(const Img& img)
	{
		static_assert(std::is_same_v<Img, image_rgb> or std::is_same_v<Img, image_rgba>,
					  "encode_tga supports image_rgb or image_rgba");

		constexpr bool is_rgba = std::is_same_v<Img, image_rgba>;

		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("encode_tga: image has zero dimensions");

		if (width > Img::MAX_WIDTH or height > Img::MAX_HEIGHT)
			return std::unexpected("encode_tga: image dimensions exceed maximum allowed size");

		constexpr u8 bytes_per_pixel = is_rgba ? 4 : 3;
		constexpr u8 pixel_depth     = is_rgba ? 32 : 24;
		constexpr u8 descriptor      = is_rgba ? 0b0000'1000 : 0b0000'0000; // 8 alpha bits only for RGBA

		std::vector<u8> buffer;
		buffer.reserve(18u + static_cast<usize>(width) * static_cast<usize>(height) * bytes_per_pixel);

		auto push_le16 = [&](u16 value)
		{
			buffer.push_back(static_cast<u8>(value & 0xFF));
			buffer.push_back(static_cast<u8>((value >> 8) & 0xFF));
		};

		// TGA header (18 bytes)
		buffer.push_back(0);           // id length
		buffer.push_back(0);           // color map type (none)
		buffer.push_back(2);           // image type (2 = uncompressed true-color)
		push_le16(0);                  // color map first entry index
		push_le16(0);                  // color map length
		buffer.push_back(0);           // color map entry size
		push_le16(0);                  // x-origin
		push_le16(0);                  // y-origin
		push_le16(static_cast<u16>(width));
		push_le16(static_cast<u16>(height));
		buffer.push_back(pixel_depth); // pixel depth (24 = BGR, 32 = BGRA)
		buffer.push_back(descriptor);  // image descriptor (origin bottom-left)

		// BGR(A), bottom-left origin => write bottom-up rows
		const auto* src = img.data().data();

		for (u32 y = 0; y < height; ++y)
		{
			const auto* row = src + static_cast<usize>(height - 1 - y) * static_cast<usize>(width);

			for (u32 x = 0; x < width; ++x)
			{
				buffer.push_back(row[x].b);
				buffer.push_back(row[x].g);
				buffer.push_back(row[x].r);
				if constexpr (is_rgba)
					buffer.push_back(row[x].a);
			}
		}

		return buffer;
	}

	export std::expected<void, std::string> save_tga(std::filesystem::path path, const image_rgb& img)
	{
		auto encoded = encode_tga(img);
		if (not encoded)
			return std::unexpected(encoded.error());

		const auto& out = encoded.value();
		auto        result
		  = file::write({.filename = path, .buffer = std::span<const u8>{out.data(), out.size()}, .mode = file::filemode::overwrite});
		if (not result.has_value() or *result != out.size())
			return std::unexpected("save_tga: failed to write image to file");
		return {};
	}

	export std::expected<void, std::string> save_tga(fs::path path, std::optional<image_rgb> img)
	{
		if (not img.has_value())
			return std::unexpected("save_tga: image is empty");
		return save_tga(path, *img);
	}

	export std::expected<void, std::string> save_tga(std::filesystem::path path, const image_rgba& img)
	{
		auto encoded = encode_tga(img);
		if (not encoded)
			return std::unexpected(encoded.error());

		const auto& out = encoded.value();
		auto        result
		  = file::write({.filename = path, .buffer = std::span<const u8>{out.data(), out.size()}, .mode = file::filemode::overwrite});
		if (not result.has_value() or *result != out.size())
			return std::unexpected("save_tga: failed to write image to file");
		return {};
	}

	export std::expected<void, std::string> save_tga(fs::path path, std::optional<image_rgba> img)
	{
		if (not img.has_value())
			return std::unexpected("save_tga: image is empty");
		return save_tga(path, *img);
	}
} // namespace deckard::image
