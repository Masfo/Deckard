export module deckard.image;

import std;
import deckard.types;
import deckard.colors;
import deckard.assert;
import deckard.math.utils;
import deckard.file;
import deckard.serializer;
import deckard.zstd;



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

		std::span<const u8> raw_data() const
		{
			return std::span<const u8>{reinterpret_cast<const u8*>(m_data.data()), m_data.size() * sizeof(color_type)};
		}

		u64 size_in_bytes() const { return raw_data().size_bytes(); }

		void assign(std::span<const color_type> pixels)
		{
			assert::check(pixels.size() == m_data.size(), "pixel span size must match width*height");
			std::ranges::copy(pixels, m_data.begin());
		}

		color_type& at(u64 x, u64 y)
		{
			assert::check(x < m_width, "x-coordinate out-of-bounds");
			assert::check(y < m_height, "y-coordinate out-of-bounds");

			auto idx = math::index_from_2d(x, y, m_width);
			assert::check(idx < m_data.size(), "index out of bounds");
			return m_data[idx];
		}

		const color_type& at(u64 x, u64 y) const
		{
			assert::check(x < m_width, "x-coordinate out-of-bounds");
			assert::check(y < m_height, "y-coordinate out-of-bounds");

			auto idx = math::index_from_2d(x, y, m_width);
			assert::check(idx < m_data.size(), "index out of bounds");
			return m_data[idx];
		}

		color_type& operator[](u64 x, u64 y) { return at(x, y); }

		const color_type& operator[](u64 x, u64 y) const { return at(x, y); }

		bool operator==(const image_channels& other) const
		{
			if (m_width != other.m_width or m_height != other.m_height)
				return false;
			return std::ranges::equal(m_data, other.m_data);
		}
	};

	export using image_rgb  = image_channels<3>;
	export using image_rgba = image_channels<4>;

	// ###############################################################################################
	// PNG - Based on uPNG ###########################################################################






	// ###############################################################################################
	// BMP ###########################################################################################


	export std::expected<bool, std::string> encode_bmp(const image_rgb& img, std::span<u8> buffer)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return std::unexpected("encode_bmp: image has zero dimensions");

		constexpr u32 bytes_per_pixel = 3;
		const u32     row_stride      = width * bytes_per_pixel;
		const u32     row_padding     = (4u - (row_stride % 4u)) % 4u;
		const u32     pixel_bytes     = (row_stride + row_padding) * height;
		const u32     header_bytes    = 14u + 40u;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		if (buffer.size() < file_bytes)
			return std::unexpected(std::format("encode_bmp: buffer too small (need {}, got {})", file_bytes, buffer.size()));

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(header_bytes);

		ser.write<u8>(static_cast<u8>('B'));
		ser.write<u8>(static_cast<u8>('M'));
		ser.write_le<u32>(file_bytes);
		ser.write_le<u16>(0);
		ser.write_le<u16>(0);
		ser.write_le<u32>(header_bytes);

		// DIB header
		ser.write_le<u32>(40u);
		ser.write_le<i32>(static_cast<i32>(width));
		ser.write_le<i32>(static_cast<i32>(height));
		ser.write_le<u16>(1);
		ser.write_le<u16>(24);   // BPP
		ser.write_le<u32>(0);    // No compression (BI_RGB)
		ser.write_le<u32>(pixel_bytes);
		ser.write_le<i32>(2835); // Print res. horizontal
		ser.write_le<i32>(2835); // Print res. vertical
		ser.write_le<u32>(0);
		ser.write_le<u32>(0);

		// Write header to buffer
		auto header_span = ser.data();
		std::memcpy(buffer.data(), header_span.data(), header_bytes);

		// Write pixels directly to buffer
		u8* pixel_ptr = buffer.data() + header_bytes;
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const auto& c = img.at(static_cast<u16>(x), static_cast<u16>(y));
				*pixel_ptr++  = static_cast<u8>(c.b);
				*pixel_ptr++  = static_cast<u8>(c.g);
				*pixel_ptr++  = static_cast<u8>(c.r);
			}
			// Row padding
			for (u32 k = 0; k < row_padding; ++k)
				*pixel_ptr++ = 0;
		}

		auto out_span = ser.data();
		if (out_span.size() != file_bytes)
			return std::unexpected(std::format("encode_bmp: internal size mismatch (expected {}, got {})", file_bytes, out_span.size()));

		std::memcpy(buffer.data(), out_span.data(), file_bytes);
		return true;
	}

	export bool save_bmp(std::filesystem::path path, const image_rgb& img)
	{
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

		std::vector<u8> out;
		out.resize(file_bytes);
		auto encoded = encode_bmp(img, out);
		if (not encoded)
			return false;

		auto result = file::write({.file = path, .buffer = out});
		return result.has_value() and *result == out.size();
	}


	export std::optional<image_rgb> decode_bmp(std::span<const u8> buffer)
	{
		if (buffer.empty())
			return {};
		if (buffer.size() < 14u + 40u)
			return {};

		deckard::serializer ser(std::span<const u8>{buffer.data(), buffer.size()});

		if (ser.remaining() < 14u)
			return {};
		if (ser.read<u8>() != static_cast<u8>('B') || ser.read<u8>() != static_cast<u8>('M'))
			return {};

		const u32 file_size = ser.read_le<u32>();
		ser.read_le<u16>();
		ser.read_le<u16>();
		const u32 pixel_offset = ser.read_le<u32>();
		if (file_size != 0 && file_size > buffer.size())
			return {};
		if (pixel_offset >= buffer.size())
			return {};

		if (ser.remaining() < 40u)
			return {};
		const u32 dib_size = ser.read_le<u32>();
		if (dib_size < 40u)
			return {};

		const i32 width_i  = ser.read_le<i32>();
		const i32 height_i = ser.read_le<i32>();
		if (width_i <= 0 || height_i == 0)
			return {};

		const bool top_down = height_i < 0;
		const u32  width    = static_cast<u32>(width_i);
		const u32  height   = static_cast<u32>(top_down ? -height_i : height_i);

		const u16 planes   = ser.read_le<u16>();
		const u16 bpp      = ser.read_le<u16>();
		const u32 compress = ser.read_le<u32>();
		if (planes != 1u)
			return {};
		if (bpp != 24u)
			return {};
		if (compress != 0u) // BI_RGB
			return {};

		ser.read_le<u32>(); // image size
		ser.read_le<i32>(); // x ppm
		ser.read_le<i32>(); // y ppm
		ser.read_le<u32>(); // colors used
		ser.read_le<u32>(); // important colors

		constexpr u32 bytes_per_pixel = 3;
		const u32     row_stride      = width * bytes_per_pixel;
		const u32     row_padding     = (4u - (row_stride % 4u)) % 4u;
		const u32     row_bytes       = row_stride + row_padding;
		const u64     needed          = static_cast<u64>(pixel_offset) + static_cast<u64>(row_bytes) * height;
		if (needed > buffer.size())
			return {};

		image_rgb img(width, height);
		for (u32 y = 0; y < height; ++y)
		{
			const u32    src_y   = top_down ? y : (height - 1 - y);
			const size_t row_off = static_cast<size_t>(pixel_offset) + static_cast<size_t>(src_y) * row_bytes;
			const u8*    row_ptr = buffer.data() + row_off;

			for (u32 x = 0; x < width; ++x)
			{
				const size_t pixel_off = x * bytes_per_pixel;
				rgb&         pixel     = img.at(static_cast<u16>(x), static_cast<u16>(y));
				pixel.b                = row_ptr[pixel_off + 0];
				pixel.g                = row_ptr[pixel_off + 1];
				pixel.r                = row_ptr[pixel_off + 2];
			}
		}

		return img;
	}

	export std::optional<image_rgb> load_bmp(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_bmp(bytes);
	}

	// ###############################################################################################
	// TGA ###########################################################################################


	export std::optional<image_rgb> decode_tga(std::span<const u8> buffer)
	{
		if (buffer.empty())
			return {};
		if (buffer.size() < 18u)
			return {};

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
			return {};
		if (color_map_type != 0)
			return {};
		if (cmap_length != 0 or cmap_depth != 0)
			return {};
		if (image_type != 2)
			return {};
		if (pixel_depth != 24 and pixel_depth != 32)
			return {};

		const bool origin_top = (descriptor & 0b0010'0000) != 0;
		const u8   alpha_bits = descriptor & 0b0000'1111;
		if (pixel_depth == 32 and alpha_bits != 8)
		{
			// tolerate but for our writer we expect 8
		}

		const size_t header_bytes = 18u + static_cast<size_t>(id_length);
		if (buffer.size() < header_bytes)
			return {};

		const size_t bytes_per_pixel = static_cast<size_t>(pixel_depth / 8);
		const size_t pixel_bytes     = static_cast<size_t>(width) * static_cast<size_t>(height) * bytes_per_pixel;
		if (buffer.size() < header_bytes + pixel_bytes)
			return {};

		auto pixel_at = [&](u16 x, u16 y) -> const u8*
		{
			const u16    src_y = origin_top ? y : static_cast<u16>((height - 1) - y);
			const size_t idx   = (static_cast<size_t>(src_y) * width + x) * bytes_per_pixel;
			return &buffer[header_bytes + idx];
		};

		if (pixel_depth == 24)
		{
			image_rgb img(width, height);
			for (u16 y = 0; y < height; ++y)
			{
				for (u16 x = 0; x < width; ++x)
				{
					const u8* p  = pixel_at(x, y);
					rgb&      px = img.at(x, y);
					px.b         = p[0];
					px.g         = p[1];
					px.r         = p[2];
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
					const u8* p  = pixel_at(x, y);
					rgb&      px = img.at(x, y);
					px.b         = p[0];
					px.g         = p[1];
					px.r         = p[2];
					// Ignore alpha channel
				}
			}
			return image_rgb{std::move(img)};
		}

		return {};
	}

	export std::optional<image_rgb> load_tga(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_tga(bytes);
	}

	export std::expected<bool, std::string> encode_tga(const image_rgb& img, std::span<u8> buffer)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("encode_tga: image has zero dimensions");

		constexpr u8  bytes_per_pixel = 3;
		constexpr u32 header_bytes    = 18;
		const u32     pixel_bytes     = width * height * bytes_per_pixel;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		if (buffer.size() < file_bytes)
			return std::unexpected(std::format("encode_tga: buffer too small (need {}, got {})", file_bytes, buffer.size()));

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(header_bytes);

		// TGA header (18 bytes)
		ser.write<u8>(0);           // id length
		ser.write<u8>(0);           // color map type (none)
		ser.write<u8>(2);           // image type (2 = uncompressed true-color)
		ser.write_le<u16>(0);       // color map first entry index
		ser.write_le<u16>(0);       // color map length
		ser.write<u8>(0);           // color map entry size
		ser.write_le<u16>(0);       // x-origin
		ser.write_le<u16>(0);       // y-origin
		ser.write_le<u16>(static_cast<u16>(width));
		ser.write_le<u16>(static_cast<u16>(height));
		ser.write<u8>(24);          // pixel depth
		ser.write<u8>(0b0000'0000); // image descriptor (origin bottom-left, no alpha)

		// Write header to buffer
		auto header_span = ser.data();
		std::memcpy(buffer.data(), header_span.data(), header_bytes);

		// Write pixels directly to buffer (BGR), bottom-left origin => write bottom-up rows
		u8* pixel_ptr = buffer.data() + header_bytes;
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const auto& c = img.at(static_cast<u16>(x), static_cast<u16>(y));
				*pixel_ptr++  = static_cast<u8>(c.b);
				*pixel_ptr++  = static_cast<u8>(c.g);
				*pixel_ptr++  = static_cast<u8>(c.r);
			}
		}

		return true;
	}

	export std::expected<bool, std::string> encode_tga(const image_rgba& img, std::span<u8> buffer)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("encode_tga: image has zero dimensions");

		constexpr u8  bytes_per_pixel = 4;
		constexpr u32 header_bytes    = 18;
		const u32     pixel_bytes     = width * height * bytes_per_pixel;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		if (buffer.size() < file_bytes)
			return std::unexpected(std::format("encode_tga: buffer too small (need {}, got {})", file_bytes, buffer.size()));

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(header_bytes);

		// TGA header (18 bytes)
		ser.write<u8>(0);           // id length
		ser.write<u8>(0);           // color map type (none)
		ser.write<u8>(2);           // image type (2 = uncompressed true-color)
		ser.write_le<u16>(0);       // color map first entry index
		ser.write_le<u16>(0);       // color map length
		ser.write<u8>(0);           // color map entry size
		ser.write_le<u16>(0);       // x-origin
		ser.write_le<u16>(0);       // y-origin
		ser.write_le<u16>(static_cast<u16>(width));
		ser.write_le<u16>(static_cast<u16>(height));
		ser.write<u8>(32);          // pixel depth
		ser.write<u8>(0b0000'1000); // image descriptor (origin bottom-left, 8 alpha bits)

		// Write header to buffer
		auto header_span = ser.data();
		std::memcpy(buffer.data(), header_span.data(), header_bytes);

		// Write pixels directly to buffer (BGRA), bottom-left origin => write bottom-up rows
		u8* pixel_ptr = buffer.data() + header_bytes;
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const auto& c = img.at(static_cast<u16>(x), static_cast<u16>(y));
				*pixel_ptr++  = static_cast<u8>(c.b);
				*pixel_ptr++  = static_cast<u8>(c.g);
				*pixel_ptr++  = static_cast<u8>(c.r);
				*pixel_ptr++  = static_cast<u8>(c.a);
			}
		}

		auto out_span = ser.data();
		if (out_span.size() != file_bytes)
			return std::unexpected(std::format("encode_tga: internal size mismatch (expected {}, got {})", file_bytes, out_span.size()));

		std::memcpy(buffer.data(), out_span.data(), file_bytes);
		return true;
	}

	export bool save_tga(std::filesystem::path path, const image_rgb& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return false;

		constexpr u8  bytes_per_pixel = 3;
		constexpr u32 header_bytes    = 18;
		const u32     pixel_bytes     = width * height * bytes_per_pixel;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		std::vector<u8> out;
		out.resize(file_bytes);
		auto encoded = encode_tga(img, out);
		if (not encoded)
			return false;

		auto result = file::write({.file = path, .buffer = out, .mode = file::filemode::overwrite});
		return result.has_value() and *result == out.size();
	}

	export bool save_tga(fs::path path, std::optional<image_rgb> img)
	{
		if (not img.has_value())
			return false;
		return save_tga(path, *img);
	}

	export bool save_tga(std::filesystem::path path, const image_rgba& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return false;

		constexpr u8  bytes_per_pixel = 4;
		constexpr u32 header_bytes    = 18;
		const u32     pixel_bytes     = width * height * bytes_per_pixel;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		std::vector<u8> out;
		out.resize(file_bytes);
		auto encoded = encode_tga(img, out);
		if (not encoded)
			return false;

		auto result = file::write({.file = path, .buffer = out, .mode = file::filemode::overwrite});
		return result.has_value() and *result == out.size();
	}

	export bool save_tga(fs::path path, std::optional<image_rgba> img)
	{
		if (not img.has_value())
			return false;
		return save_tga(path, *img);
	}

	// ###############################################################################################
	// QOI ###########################################################################################

	namespace detail
	{
		struct qoi_px
		{
			u8             r{0}, g{0}, b{0}, a{255};
			constexpr bool operator==(const qoi_px&) const = default;
		};

		constexpr u8 qoi_hash(const qoi_px& p) { return (p.r * 3u + p.g * 5u + p.b * 7u + p.a * 11u) % 64u; }

		constexpr u8 qoi_op_index = 0x00; // 00xxxxxx
		constexpr u8 qoi_op_diff  = 0x40; // 01xxxxxx
		constexpr u8 qoi_op_luma  = 0x80; // 10xxxxxx
		constexpr u8 qoi_op_run   = 0xC0; // 11xxxxxx
		constexpr u8 qoi_op_rgb   = 0xFE;
		constexpr u8 qoi_op_rgba  = 0xFF;

		inline void qoi_write_header(deckard::serializer& ser, u32 width, u32 height, u8 channels, u8 colorspace)
		{
			ser.write<u8>('q');
			ser.write<u8>('o');
			ser.write<u8>('i');
			ser.write<u8>('f');
			ser.write_be<u32>(width); // QOI stores width/height as big-endian
			ser.write_be<u32>(height);
			ser.write<u8>(channels);
			ser.write<u8>(colorspace);
		}

		inline void qoi_write_end(deckard::serializer& ser)
		{
			// 7x 0x00 + 0x01
			for (int i = 0; i < 7; ++i)
				ser.write<u8>(0);
			ser.write<u8>(1);
		}

		inline void qoi_encode_pixels(deckard::serializer& ser, std::span<const qoi_px> pixels)
		{
			std::array<qoi_px, 64> index{};
			qoi_px                 prev{0, 0, 0, 255};
			u8                     run = 0;

			for (size_t i = 0; i < pixels.size(); ++i)
			{
				const qoi_px px = pixels[i];

				if (px == prev)
				{
					if (++run == 62 || i == pixels.size() - 1)
					{
						ser.write<u8>(static_cast<u8>(qoi_op_run | (run - 1)));
						run = 0;
					}
					continue;
				}

				if (run > 0)
				{
					ser.write<u8>(static_cast<u8>(qoi_op_run | (run - 1)));
					run = 0;
				}

				const u8 idx = qoi_hash(px);
				if (index[idx] == px)
				{
					ser.write<u8>(static_cast<u8>(qoi_op_index | idx));
					prev = px;
					continue;
				}
				index[idx] = px;

				if (px.a == prev.a)
				{
					const i32 dr = static_cast<i32>(px.r) - static_cast<i32>(prev.r);
					const i32 dg = static_cast<i32>(px.g) - static_cast<i32>(prev.g);
					const i32 db = static_cast<i32>(px.b) - static_cast<i32>(prev.b);

					if (dr > -3 && dr < 2 && dg > -3 && dg < 2 && db > -3 && db < 2)
					{
						ser.write<u8>(static_cast<u8>(qoi_op_diff | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2)));
						prev = px;
						continue;
					}

					const i32 dg_l  = dg;
					const i32 dr_dg = dr - dg_l;
					const i32 db_dg = db - dg_l;
					if (dg_l > -33 && dg_l < 32 && dr_dg > -9 && dr_dg < 8 && db_dg > -9 && db_dg < 8)
					{
						ser.write<u8>(static_cast<u8>(qoi_op_luma | (dg_l + 32)));
						ser.write<u8>(static_cast<u8>(((dr_dg + 8) << 4) | (db_dg + 8)));
						prev = px;
						continue;
					}

					ser.write<u8>(qoi_op_rgb);
					ser.write<u8>(px.r);
					ser.write<u8>(px.g);
					ser.write<u8>(px.b);
					prev = px;
					continue;
				}

				ser.write<u8>(qoi_op_rgba);
				ser.write<u8>(px.r);
				ser.write<u8>(px.g);
				ser.write<u8>(px.b);
				ser.write<u8>(px.a);
				prev = px;
			}
		}
	} // namespace detail

	// Save as QOI (Quite OK Image) per https://qoiformat.org.
	export bool save_qoi(std::filesystem::path path, const image_rgb& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return false;

		std::vector<detail::qoi_px> pixels;
		pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
		for (u32 y = 0; y < height; ++y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const auto& c                              = img[static_cast<u16>(x), static_cast<u16>(y)];
				pixels[static_cast<size_t>(y) * width + x] = detail::qoi_px{c.r, c.g, c.b, 255};
			}
		}

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(14 + pixels.size() * 5 + 8);
		detail::qoi_write_header(ser, width, height, 3, 0);
		detail::qoi_encode_pixels(ser, std::span<const detail::qoi_px>{pixels.data(), pixels.size()});
		detail::qoi_write_end(ser);

		auto out_span = ser.data();
		auto result   = file::write({.file = path, .buffer = out_span});
		return result.has_value() && *result == out_span.size();
	}

	export bool save_qoi(std::filesystem::path path, const image_rgba& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return false;

		std::vector<detail::qoi_px> pixels;
		pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
		for (u32 y = 0; y < height; ++y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const auto& c                              = img[static_cast<u16>(x), static_cast<u16>(y)];
				pixels[static_cast<size_t>(y) * width + x] = detail::qoi_px{c.r, c.g, c.b, c.a};
			}
		}


		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(14 + pixels.size() * 5 + 8);
		detail::qoi_write_header(ser, width, height, 4, 0);
		detail::qoi_encode_pixels(ser, std::span<const detail::qoi_px>{pixels.data(), pixels.size()});
		detail::qoi_write_end(ser);

		auto out_span = ser.data();
		auto result   = file::write({.file = path, .buffer = out_span, .mode = file::filemode::overwrite});
		return result.has_value() && *result == out_span.size();
	}

	// Save as uncompressed TGA (type 2).
	// Note: TGA stores pixels as BGR(A). This writer uses bottom-left origin (descriptor bit 5 = 0).
	export bool save_tga(std::filesystem::path path, const image_rgb& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 || height == 0)
			return false;

		constexpr u8  bytes_per_pixel = 3;
		constexpr u32 header_bytes    = 18;
		const u32     pixel_bytes     = width * height * bytes_per_pixel;
		const u32     file_bytes      = header_bytes + pixel_bytes;

		deckard::serializer ser(deckard::padding::yes);
		ser.reserve(file_bytes);

		// TGA header (18 bytes)
		ser.write<u8>(0);           // id length
		ser.write<u8>(0);           // color map type (none)
		ser.write<u8>(2);           // image type (2 = uncompressed true-color)
		ser.write_le<u16>(0);       // color map first entry index
		ser.write_le<u16>(0);       // color map length
		ser.write<u8>(0);           // color map entry size
		ser.write_le<u16>(0);       // x-origin
		ser.write_le<u16>(0);       // y-origin
		ser.write_le<u16>(static_cast<u16>(width));
		ser.write_le<u16>(static_cast<u16>(height));
		ser.write<u8>(24);          // pixel depth
		ser.write<u8>(0b0000'0000); // image descriptor (origin bottom-left, no alpha)

		// Pixel data (BGR), bottom-left origin => write bottom-up rows
		for (i32 y = static_cast<i32>(height) - 1; y >= 0; --y)
		{
			for (u32 x = 0; x < width; ++x)
			{
				const image_rgb::color_type& c = img[static_cast<u16>(x), static_cast<u16>(y)];
				ser.write<u8>(static_cast<u8>(c.b));
				ser.write<u8>(static_cast<u8>(c.g));
				ser.write<u8>(static_cast<u8>(c.r));
			}
		}

		auto out_span = ser.data();
		auto result   = file::write({.file = path, .buffer = out_span, .mode = file::filemode::overwrite});
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
		ser.write<u8>(0);           // id length
		ser.write<u8>(0);           // color map type (none)
		ser.write<u8>(2);           // image type (2 = uncompressed true-color)
		ser.write_le<u16>(0);       // color map first entry index
		ser.write_le<u16>(0);       // color map length
		ser.write<u8>(0);           // color map entry size
		ser.write_le<u16>(0);       // x-origin
		ser.write_le<u16>(0);       // y-origin
		ser.write_le<u16>(static_cast<u16>(width));
		ser.write_le<u16>(static_cast<u16>(height));
		ser.write<u8>(32);          // pixel depth
		ser.write<u8>(0b0000'1000); // image descriptor (origin bottom-left, 8 alpha bits)

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
