export module deckard.image;

import std;
import deckard.types;
import deckard.colors;
import deckard.assert;
import deckard.math.utils;
import deckard.file;
import deckard.serializer;

namespace deckard
{
	template<u64 Channels>
	class image_channels final
	{
		static_assert(Channels == 3 or Channels == 4, "Channels must be either 3 (RGB) or 4 (RGBA)");

	public:
		using color_type = std::conditional_t<Channels == 4, rgba, rgb>;

	private:
		// conditional<Channels == 1, gray, std::conditional<Channels == 3, rgb, std::conditional<Channels == 4, rgba, ....>>
		using ColorType = std::conditional<Channels == 4, rgba, rgb>;


		std::vector<color_type> m_data;
		u16                     m_width{1};
		u16                     m_height{1};

	public:
		explicit image_channels(u32 w, u32 h)
		{
			m_width  = static_cast<u16>(std::clamp(w, 1u, static_cast<u32>(limits::max<u16>)));
			m_height = static_cast<u16>(std::clamp(h, 1u, static_cast<u32>(limits::max<u16>)));

			m_data.resize(m_width * m_height);
		}

		explicit image_channels(u32 w, u32 h, std::span<const color_type> pixels)
			: image_channels(w, h)
		{
			assign(pixels);
		}

		u16 width() const { return m_width; }

		u16 height() const { return m_height; }

		std::span<color_type> data() { return m_data; }

		std::span<const color_type> data() const { return m_data; }

		void assign(std::span<const color_type> pixels)
		{
			assert::check(pixels.size() == m_data.size(), "pixel span size must match width*height");
			std::ranges::copy(pixels, m_data.begin());
		}

		color_type& at(u16 x, u16 y)
		{ 
			assert::check(x < m_width, "x-coordinate out-of-bounds");
			assert::check(y < m_height, "y-coordinate out-of-bounds");

			auto idx = math::index_from_2d(x, y, m_width);
			assert::check(idx < m_data.size(), "index out of bounds");
			return m_data[idx];
		}

		const color_type& at(u16 x, u16 y) const
		{ 
			assert::check(x < m_width, "x-coordinate out-of-bounds");
			assert::check(y < m_height, "y-coordinate out-of-bounds");

			auto idx = math::index_from_2d(x, y, m_width);
			assert::check(idx < m_data.size(), "index out of bounds");
			return m_data[idx];
		}

		color_type& operator[](u16 x, u16 y) { return at(x, y); }

		const color_type& operator[](u16 x, u16 y) const { return at(x, y); }
	};

	export using image_gray = image_channels<1>;
	export using image_rgb  = image_channels<3>;
	export using image_rgba = image_channels<4>;

	export bool save_bmp(std::filesystem::path path, const image_rgb& img)
	{
		using byte       = std::uint8_t;
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return false;

		constexpr u32 bytes_per_pixel = 3;
		const u32     row_stride      = width * bytes_per_pixel;
		const u32     row_padding     = (4u - (row_stride % 4u)) % 4u;
		const u32     pixel_bytes     = (row_stride + row_padding) * height;
		const u32     header_bytes    = 14u + 40u;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(file_bytes);

		// BITMAPFILEHEADER (14 bytes)
		ser.write<u8>(static_cast<u8>('B'));
		ser.write<u8>(static_cast<u8>('M'));
		ser.write_le<u32>(file_bytes);
		ser.write_le<u16>(0);
		ser.write_le<u16>(0);
		ser.write_le<u32>(header_bytes);

		// BITMAPINFOHEADER (40 bytes)
		ser.write_le<u32>(40u);
		ser.write_le<i32>(static_cast<i32>(width));
		ser.write_le<i32>(static_cast<i32>(height));
		ser.write_le<u16>(1);
		ser.write_le<u16>(24);
		ser.write_le<u32>(0);
		ser.write_le<u32>(pixel_bytes);
		ser.write_le<i32>(2835);
		ser.write_le<i32>(2835);
		ser.write_le<u32>(0);
		ser.write_le<u32>(0);

		// pixel data: bottom-up, BGR
		constexpr std::uint8_t pad[3] = {0, 0, 0};
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
		{
			const image_rgb::color_type& c = img[static_cast<u16>(x), static_cast<u16>(y)];
			ser.write<u8>(static_cast<u8>(c.b));
			ser.write<u8>(static_cast<u8>(c.g));
			ser.write<u8>(static_cast<u8>(c.r));
		}
		for (u32 k = 0; k < row_padding; ++k)
			ser.write<u8>(pad[0]);
	}

	auto out_span = ser.data();
	auto result   = file::write({.file = path, .buffer = out_span});
	return result.has_value() && *result == out_span.size();
}



	// Load a 24-bit uncompressed BMP (BI_RGB).
	// Supports bottom-up DIBs with 4-byte-aligned rows (as produced by `save_bmp`).
	// Returns empty optional on failure.
	export std::optional<image_rgb> load_bmp(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		if (bytes.empty())
			return {};
		if (bytes.size() < 14u + 40u)
			return {};

		auto u16_le = [&](size_t off) -> u16
		{
			return static_cast<u16>(static_cast<u16>(bytes[off + 0]) | (static_cast<u16>(bytes[off + 1]) << 8));
		};
		auto u32_le = [&](size_t off) -> u32
		{
			return static_cast<u32>(static_cast<u32>(bytes[off + 0]) | (static_cast<u32>(bytes[off + 1]) << 8)
			                       | (static_cast<u32>(bytes[off + 2]) << 16) | (static_cast<u32>(bytes[off + 3]) << 24));
		};
		auto i32_le = [&](size_t off) -> i32 { return static_cast<i32>(u32_le(off)); };

		// BITMAPFILEHEADER
		if (bytes[0] != static_cast<u8>('B') || bytes[1] != static_cast<u8>('M'))
			return {};
		const u32 file_size = u32_le(2);
		if (file_size != 0 && file_size > bytes.size())
			return {};
		const u32 pixel_offset = u32_le(10);
		if (pixel_offset >= bytes.size())
			return {};

		// BITMAPINFOHEADER
		const u32 dib_size = u32_le(14);
		if (dib_size < 40u)
			return {};

		const i32 width_i  = i32_le(18);
		const i32 height_i = i32_le(22);
		if (width_i <= 0 || height_i == 0)
			return {};

		const bool top_down = height_i < 0;
		const u32  width    = static_cast<u32>(width_i);
		const u32  height   = static_cast<u32>(top_down ? -height_i : height_i);

		const u16 planes    = u16_le(26);
		const u16 bpp       = u16_le(28);
		const u32 compress  = u32_le(30);
		if (planes != 1u)
			return {};
		if (bpp != 24u)
			return {};
		if (compress != 0u) // BI_RGB
			return {};

		constexpr u32 bytes_per_pixel = 3;
		const u32     row_stride      = width * bytes_per_pixel;
		const u32     row_padding     = (4u - (row_stride % 4u)) % 4u;
		const u32     row_bytes       = row_stride + row_padding;
		const u64     needed          = static_cast<u64>(pixel_offset) + static_cast<u64>(row_bytes) * height;
		if (needed > bytes.size())
			return {};

		image_rgb img(width, height);
		for (u32 y = 0; y < height; ++y)
		{
			const u32 src_y = top_down ? y : (height - 1 - y);
			const size_t row_off = static_cast<size_t>(pixel_offset) + static_cast<size_t>(src_y) * row_bytes;
			for (u32 x = 0; x < width; ++x)
			{
				const size_t p = row_off + static_cast<size_t>(x) * bytes_per_pixel;
				rgb c;
				c.b = bytes[p + 0];
				c.g = bytes[p + 1];
				c.r = bytes[p + 2];
				img[static_cast<u16>(x), static_cast<u16>(y)] = c;
			}
		}

		return img;
	}

	// Load an uncompressed true-color TGA (type 2), 24bpp (BGR) or 32bpp (BGRA).
	// Returns empty optional on failure.
	// Note: TGA origin is controlled by descriptor bit 5 (top-left if set, bottom-left if clear).
	export std::optional<image_rgb> load_tga(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		if (bytes.empty())
			return {};
		if (bytes.size() < 18u)
			return {};

		auto u16_le = [&](size_t off) -> u16
		{
			return static_cast<u16>(static_cast<u16>(bytes[off + 0]) | (static_cast<u16>(bytes[off + 1]) << 8));
		};

		const u8  id_length      = bytes[0];
		const u8  color_map_type = bytes[1];
		const u8  image_type     = bytes[2];
		const u16 cmap_length    = u16_le(5);
		const u8  cmap_depth     = bytes[7];
		const u16 width          = u16_le(12);
		const u16 height         = u16_le(14);
		const u8  pixel_depth    = bytes[16];
		const u8  descriptor     = bytes[17];

		if (width == 0 || height == 0)
			return {};
		if (color_map_type != 0)
			return {};
		if (cmap_length != 0 || cmap_depth != 0)
			return {};
		if (image_type != 2)
			return {};
		if (pixel_depth != 24 && pixel_depth != 32)
			return {};

		const bool origin_top = (descriptor & 0b0010'0000) != 0;
		const u8   alpha_bits = descriptor & 0b0000'1111;
		if (pixel_depth == 32 && alpha_bits != 8)
		{
			// tolerate but for our writer we expect 8
		}

		const size_t header_bytes = 18u + static_cast<size_t>(id_length);
		if (bytes.size() < header_bytes)
			return {};

		const size_t bytes_per_pixel = static_cast<size_t>(pixel_depth / 8);
		const size_t pixel_bytes     = static_cast<size_t>(width) * static_cast<size_t>(height) * bytes_per_pixel;
		if (bytes.size() < header_bytes + pixel_bytes)
			return {};

		auto pixel_at = [&](u16 x, u16 y) -> const u8*
		{
			const u16 src_y = origin_top ? y : static_cast<u16>((height - 1) - y);
			const size_t idx = (static_cast<size_t>(src_y) * width + x) * bytes_per_pixel;
			return &bytes[header_bytes + idx];
		};

		if (pixel_depth == 24)
		{
			image_rgb img(width, height);
			for (u16 y = 0; y < height; ++y)
			{
				for (u16 x = 0; x < width; ++x)
				{
					const u8* p = pixel_at(x, y);
					rgb c;
					c.b = p[0];
					c.g = p[1];
					c.r = p[2];
					img.at(x, y) = c;
				}
			}
			return image_rgb{std::move(img)};
		}

		if (pixel_depth == 32)
		{
			image_rgb img(width, height);
			for (u16 y = 0; y < height; ++y)
			{
				for (u16 x = 0; x < width; ++x)
				{
					const u8* p = pixel_at(x, y);
					rgb c;
					c.b = p[0];
					c.g = p[1];
					c.r = p[2];
					img[x, y] = c;
				}
			}
			return image_rgb{std::move(img)};
		}

		return {};
	}
	auto out_span = ser.data();
	auto result   = file::write({.file = path, .buffer = out_span});
	return result.has_value() && *result == out_span.size();
}

	export bool save_tga(std::filesystem::path path, const image_rgba& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return false;

		constexpr u8  bytes_per_pixel = 4;
		constexpr u32 header_bytes    = 18;
		const u32     pixel_bytes     = width * height * bytes_per_pixel;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(file_bytes);

		// TGA header (18 bytes)
		ser.write<u8>(0);                 // id length
		ser.write<u8>(0);                 // color map type (none)
		ser.write<u8>(2);                 // image type (2 = uncompressed true-color)
		ser.write_le<u16>(0);             // color map first entry index
		ser.write_le<u16>(0);             // color map length
		ser.write<u8>(0);                 // color map entry size
		ser.write_le<u16>(0);             // x-origin
		ser.write_le<u16>(0);             // y-origin
		ser.write_le<u16>(static_cast<u16>(width));
		ser.write_le<u16>(static_cast<u16>(height));
		ser.write<u8>(32);                // pixel depth
		ser.write<u8>(0b0000'1000);       // image descriptor (origin bottom-left, 8 alpha bits)

		// Pixel data (BGRA), bottom-left origin => write bottom-up rows
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
			{
							const image_rgba::color_type& c = img[static_cast<u16>(x), static_cast<u16>(y)];
				ser.write<u8>(static_cast<u8>(c.b));
				ser.write<u8>(static_cast<u8>(c.g));
				ser.write<u8>(static_cast<u8>(c.r));
				ser.write<u8>(static_cast<u8>(c.a));
			}
		}

		auto out_span = ser.data();
		auto result   = file::write({.file = path, .buffer = out_span, .mode = file::filemode::overwrite});
		return result.has_value() && *result == out_span.size();
	}



}; // namespace deckard
