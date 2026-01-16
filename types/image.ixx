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
