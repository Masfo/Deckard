export module deckard.image:qoi;

import std;
import deckard.types;
import deckard.file;
import deckard.assert;
import deckard.helpers;
import deckard.colors;

import :common;

namespace deckard::image
{
	template<typename T>
	concept Image = std::is_same_v<T, image_gray> or std::is_same_v<T, image_rgb> or std::is_same_v<T, image_rgba>;

	namespace detail
	{
		constexpr u8 qoi_hash(const rgba& p) { return (p.r * 3u + p.g * 5u + p.b * 7u + p.a * 11u) % 64u; }

		constexpr u8 qoi_op_index = 0x00; // 00xxxxxx
		constexpr u8 qoi_op_diff  = 0x40; // 01xxxxxx
		constexpr u8 qoi_op_luma  = 0x80; // 10xxxxxx
		constexpr u8 qoi_op_run   = 0xC0; // 11xxxxxx
		constexpr u8 qoi_op_rgb   = 0xFE;
		constexpr u8 qoi_op_rgba  = 0xFF;

		struct qoi_reader
		{
			const u8* ptr;
			const u8* end;

			constexpr size_t remaining() const { return static_cast<size_t>(end - ptr); }

			constexpr u8 read_u8()
			{
				if (ptr >= end)
					return 0;
				return *ptr++;
			}

			constexpr u32 read_be32()
			{
				if (ptr + 4 > end)
				{
					ptr = end;
					return 0;
				}
				u32 val = (static_cast<u32>(ptr[0]) << 24) | (static_cast<u32>(ptr[1]) << 16)
						  | (static_cast<u32>(ptr[2]) << 8) | static_cast<u32>(ptr[3]);
				ptr += 4;
				return val;
			}
		};

		void push_be32(std::vector<u8>& buffer, u32 val)
		{
			buffer.push_back(static_cast<u8>((val >> 24) & 0xFF));
			buffer.push_back(static_cast<u8>((val >> 16) & 0xFF));
			buffer.push_back(static_cast<u8>((val >> 8) & 0xFF));
			buffer.push_back(static_cast<u8>(val & 0xFF));
		}

		template<typename Px>
		constexpr rgba qoi_to_rgba(const Px& px)
		{
			if constexpr (std::is_same_v<Px, rgba>)
				return px;
			else
				return {px.r, px.g, px.b, 255u};
		}

		template<typename Px>
		void qoi_encode_pixels(std::vector<u8>& buffer, std::span<const Px> pixels)
		{
			std::array<rgba, 64> index{};
			rgba                 prev{0, 0, 0, 255};
			u8                   run = 0;

			for (size_t i = 0; i < pixels.size(); ++i)
			{
				const rgba px = qoi_to_rgba(pixels[i]);

				if (px == prev)
				{
					if (++run == 62 or i == pixels.size() - 1)
					{
						buffer.push_back(static_cast<u8>(qoi_op_run | (run - 1)));
						run = 0;
					}
					continue;
				}

				if (run > 0)
				{
					buffer.push_back(static_cast<u8>(qoi_op_run | (run - 1)));
					run = 0;
				}

				const u8 idx = qoi_hash(px);
				if (index[idx] == px)
				{
					buffer.push_back(static_cast<u8>(qoi_op_index | idx));
					prev = px;
					continue;
				}
				index[idx] = px;

				if (px.a == prev.a)
				{
					const i32 dr = static_cast<i32>(px.r) - static_cast<i32>(prev.r);
					const i32 dg = static_cast<i32>(px.g) - static_cast<i32>(prev.g);
					const i32 db = static_cast<i32>(px.b) - static_cast<i32>(prev.b);

					if (dr > -3 and dr < 2 and dg > -3 and dg < 2 and db > -3 and db < 2)
					{
						buffer.push_back(static_cast<u8>(qoi_op_diff | ((dr + 2) << 4) | ((dg + 2) << 2) | (db + 2)));
						prev = px;
						continue;
					}

					const i32 dg_l  = dg;
					const i32 dr_dg = dr - dg_l;
					const i32 db_dg = db - dg_l;
					if (dg_l > -33 and dg_l < 32 and dr_dg > -9 and dr_dg < 8 and db_dg > -9 and db_dg < 8)
					{
						buffer.push_back(static_cast<u8>(qoi_op_luma | (dg_l + 32)));
						buffer.push_back(static_cast<u8>(((dr_dg + 8) << 4) | (db_dg + 8)));
						prev = px;
						continue;
					}

					buffer.push_back(qoi_op_rgb);
					buffer.push_back(px.r);
					buffer.push_back(px.g);
					buffer.push_back(px.b);
					prev = px;
					continue;
				}

				buffer.push_back(qoi_op_rgba);
				buffer.push_back(px.r);
				buffer.push_back(px.g);
				buffer.push_back(px.b);
				buffer.push_back(px.a);
				prev = px;
			}
		}

		std::optional<std::pair<u32, u32>> qoi_read_header(qoi_reader& rd, u8& channels)
		{
			if (rd.remaining() < 14u)
				return {};

			if (rd.read_u8() != 'q' or rd.read_u8() != 'o' or rd.read_u8() != 'i' or rd.read_u8() != 'f')
				return {};

			const u32 width  = rd.read_be32();
			const u32 height = rd.read_be32();
			channels         = rd.read_u8();

			if (width == 0 or height == 0)
				return {};

			if (channels != 3 and channels != 4)
				return {};


			const u8 colorspace = rd.read_u8();
			if (colorspace > 1)
				return {};

			return std::make_pair(width, height);
		}


	} // namespace detail

	u64 bound_qoi(u32 width, u32 height, u8 channels)
	{
		if (width == 0 or height == 0)
			return 0;

		return 14 + static_cast<u64>(width) * static_cast<u64>(height) * static_cast<u64>(channels + 1) + 8;
	}

	export template<Image Img>
	std::expected<std::vector<u8>, std::string> encode_qoi(const Img& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("encode_qoi: image has zero dimensions");

		
		constexpr u8    channels = std::is_same_v<Img, image_rgb> ? 3 : 4;
		std::vector<u8> buffer;
		buffer.reserve(bound_qoi(width, height, channels));

		// Write header ('qoif' + width + height + channels + colorspace)
		buffer.push_back('q');
		buffer.push_back('o');
		buffer.push_back('i');
		buffer.push_back('f');
		detail::push_be32(buffer, width);
		detail::push_be32(buffer, height);
		buffer.push_back(channels);
		buffer.push_back(0); // colorspace

		// Encode pixels
		detail::qoi_encode_pixels(buffer, img.data());

		// Write end marker (7x 0x00 + 0x01)
		for (int i = 0; i < 7; ++i)
			buffer.push_back(0);
		buffer.push_back(1);

		return buffer;
	}

	export template<Image Img>
	std::expected<Img, std::string> decode_qoi(std::span<const u8> buffer)
	{

		if (buffer.size() < 14u + 8u)
			return std::unexpected("decode_qoi: buffer too small");

		detail::qoi_reader rd{buffer.data(), buffer.data() + buffer.size()};
		u8                 channels{0};

		auto dims = detail::qoi_read_header(rd, channels);
		if (not dims)
			return std::unexpected("decode_qoi: failed to read header");

		auto [width, height] = dims.value();

		if (width > Img::MAX_WIDTH or height > Img::MAX_HEIGHT)
			return std::unexpected("decode_qoi: image dimensions exceed maximum allowed size");

		Img            img(width, height);
		constexpr bool is_rgba = std::is_same_v<Img, image_rgba>;

		std::array<rgba, 64> index{};
		rgba                 prev{0, 0, 0, 255};


		if (width == 0 or height == 0)
			return std::unexpected("decode_qoi: image has zero dimensions");
		if (width > Img::MAX_WIDTH or height > Img::MAX_HEIGHT)
			return std::unexpected("decode_qoi: image dimensions exceed maximum allowed size");

		const u64 pixel_count = static_cast<u64>(width) * static_cast<u64>(height);
		auto*     out         = img.data().data();
		usize     pos         = 0;

		auto write_pixel = [&](const rgba& p)
		{
			if constexpr (is_rgba)
			{
				out[pos] = p;
			}
			else
			{
				out[pos] = rgb{p.r, p.g, p.b};
			}
			++pos;
		};

		for (u64 i = 0; i < pixel_count; ++i)
		{
			if (rd.remaining() < 1)
				return std::unexpected("decode_qoi: unexpected end of data");

			const u8 byte = rd.read_u8();

			if (byte == detail::qoi_op_rgba)
			{
				if (rd.remaining() < 4)
					return std::unexpected("decode_qoi: unexpected end of data");
				prev.r = rd.read_u8();
				prev.g = rd.read_u8();
				prev.b = rd.read_u8();
				prev.a = rd.read_u8();
			}
			else if (byte == detail::qoi_op_rgb)
			{
				if (rd.remaining() < 3)
					return std::unexpected("decode_qoi: unexpected end of data");
				prev.r = rd.read_u8();
				prev.g = rd.read_u8();
				prev.b = rd.read_u8();
			}
			else if ((byte & 0xC0) == 0xC0)
			{
				const u8 run = (byte & 0x3F) + 1;
				if (i + run > pixel_count)
					return std::unexpected("decode_qoi: run exceeds pixel count");


				for (u8 j = 1; j < run; ++j)
				{
					write_pixel(prev);
					++i;
				}
			}
			else if ((byte & 0xC0) == 0x80)
			{
				if (rd.remaining() < 1)
					return std::unexpected("decode_qoi: unexpected end of data");
				const u8  byte2 = rd.read_u8();
				const i32 dg    = (byte & 0x3F) - 32;
				const i32 drg   = ((byte2 >> 4) & 0x0F) - 8;
				const i32 dbg   = (byte2 & 0x0F) - 8;

				prev.r = static_cast<u8>(prev.r + drg + dg);
				prev.g = static_cast<u8>(prev.g + dg);
				prev.b = static_cast<u8>(prev.b + dbg + dg);
			}
			else if ((byte & 0xC0) == 0x40)
			{
				const i32 dr = ((byte >> 4) & 0x03) - 2;
				const i32 dg = ((byte >> 2) & 0x03) - 2;
				const i32 db = (byte & 0x03) - 2;

				prev.r = static_cast<u8>(prev.r + dr);
				prev.g = static_cast<u8>(prev.g + dg);
				prev.b = static_cast<u8>(prev.b + db);
			}
			else
			{
				prev = index[byte & 0x3F];
			}

			write_pixel(prev);
			index[detail::qoi_hash(prev)] = prev;
		}

		return img;
	}

	export template<Image Img = image_rgba>
	std::expected<Img, std::string> load_qoi(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_qoi<Img>(bytes);
	}

	export std::expected<u64, std::string> save_qoi(std::filesystem::path path, const image_rgb& img)
	{
		auto encoded = encode_qoi(img);
		if (not encoded)
			return std::unexpected(encoded.error());

		auto result = file::write(
		  {.filename = path, .buffer = std::span<const u8>{encoded.value().data(), encoded.value().size()}});
		if (not result.has_value() or *result != encoded.value().size())
			return std::unexpected("save_qoi: failed to write file");
		return *result;
	}

	export std::expected<u64, std::string> save_qoi(std::filesystem::path path, const image_rgba& img)
	{
		auto encoded = encode_qoi(img);
		if (not encoded)
			return std::unexpected(encoded.error());

		auto result = file::write(
		  {.filename = path, .buffer = std::span<const u8>{encoded.value().data(), encoded.value().size()}});
		if (not result.has_value() or *result != encoded.value().size())
			return std::unexpected("save_qoi: failed to write file");
		return *result;
	}
} // namespace deckard::image
