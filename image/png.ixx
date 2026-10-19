export module deckard.image:png;

import std;
import deckard.as;
import deckard.assert;
import deckard.file;
import deckard.helpers;
import deckard.inflate;
import deckard.types;
import deckard.utils.hash;
import deckard.debug;

import :common;


namespace fs = std::filesystem;

namespace deckard::image
{
	namespace png_detail
	{


		// color indexed images: expand each pixel to RGBA using the palette
		template<u32 DEPTH>
		void expand_indexed(const u8* src, u8* dst, usize width, const u8 (*pal)[4]) noexcept
		{
			if constexpr (DEPTH == 8)
			{
				for (usize x = 0; x < width; ++x)
					std::memcpy(dst + x * 4, &pal[src[x]], 4);
			}
			else
			{
				constexpr u32 per_byte = 8 / DEPTH;
				constexpr u32 mask     = (1u << DEPTH) - 1;
				for (usize x = 0; x < width; ++x)
				{
					const u32 shift = 8 - DEPTH * (1 + u32(x % per_byte)); // leftmost pixel is the high bits
					const u32 idx   = (u32{src[x / per_byte]} >> shift) & mask;
					std::memcpy(dst + x * 4, pal[idx], 4);
				}
			}
		}

		template<u32 DEPTH>
		[[nodiscard]] bool decode_rows_indexed(u8* raw, usize width, usize height, u8* out, const u8 (*pal)[4]) noexcept
		{
			const usize n        = (width * DEPTH + 7) / 8;
			const usize row_size = n + 1;

			std::vector<u8> zero(n, 0);
			const u8*       prev = zero.data();

			for (usize y = 0; y < height; ++y)
			{
				u8* const row = raw + y * row_size;
				if (not unfilter<1>(row[0], row + 1, row + 1, prev, n))
					return false;

				expand_indexed<DEPTH>(row + 1, out + y * width * 4, width, pal);
				prev = row + 1;
			}
			return true;
		}

		template<usize BPP>
		void expand_rgba(std::span<const u8> src, std::span<u8> dst) noexcept
		{
			const usize width = dst.size() / 4;
			assert::check(src.size() == width * BPP, "expand_rgba: src size mismatch");

			const u8* s = src.data();
			u8*       d = dst.data();

			for (usize x = 0; x < width; ++x, s += BPP, d += 4)
			{
				if constexpr (BPP == 1) // gray
				{
					d[0] = d[1] = d[2] = s[0];
					d[3]               = 255;
				}
				else if constexpr (BPP == 2) // gray + alpha
				{
					d[0] = d[1] = d[2] = s[0];
					d[3]               = s[1];
				}
				else // RGB
				{
					d[0] = s[0];
					d[1] = s[1];
					d[2] = s[2];
					d[3] = 255;
				}
			}
		}

		template<usize BPP>
		[[nodiscard]] bool decode_rows(u8* raw, usize width, usize height, u8* out) noexcept
		{
			const usize n        = width * BPP;
			const usize row_size = n + 1;

			std::vector<u8> zero(n, 0);
			const u8*       prev = zero.data();

			for (usize y = 0; y < height; ++y)
			{
				u8* const row = raw + y * row_size;

				if constexpr (BPP == 4)
				{
					// RGBA: unfilter straight into the image. No expand pass, and `raw` stays untouched.
					u8* const dst = out + y * n;

					if (not unfilter<4>(row[0], row + 1, dst, prev, n))
						return false;
					prev = dst;
				}
				else
				{
					// unfilter in place (row is hot in cache), then expand into the image
					// if (not unfilter<BPP>(row[0], {row + 1, n}, {row + 1, n}, {prev, n}))
					//	return false;

					if (not unfilter<BPP>(row[0], row + 1, row + 1, prev, n))
						return false;

					expand_rgba<BPP>({row + 1, n}, {out + y * width * 4, width * 4});

					prev = row + 1;
				}
			}
			return true;
		}

		[[nodiscard]] constexpr u8 paeth(i32 a, i32 b, i32 c) noexcept
		{
			const i32 p  = a + b - c;
			const i32 pa = p > a ? p - a : a - p;
			const i32 pb = p > b ? p - b : b - p;
			const i32 pc = p > c ? p - c : c - p;
			return static_cast<u8>((pa <= pb and pa <= pc) ? a : (pb <= pc ? b : c));
		}

		template<usize BPP>
		[[nodiscard]] bool unfilter(u8 type, const u8* src, u8* dst, const u8* prev, usize n) noexcept
		{
			switch (type)
			{
				case 0: // None
					if (src != dst)
						std::memcpy(dst, src, n);
					return true;

				case 1: // Sub
				{
					std::array<u8, BPP> l{};
					for (usize i = 0; i < n; i += BPP)
						for (usize k = 0; k < BPP; ++k)
						{
							l[k]       = static_cast<u8>(src[i + k] + l[k]);
							dst[i + k] = l[k];
						}
					return true;
				}

				case 2: // Up
					for (usize i = 0; i < n; ++i)
						dst[i] = static_cast<u8>(src[i] + prev[i]);
					return true;

				case 3: // Average
				{
					std::array<u8, BPP> l{};
					for (usize i = 0; i < n; i += BPP)
						for (usize k = 0; k < BPP; ++k)
						{
							l[k]       = static_cast<u8>(src[i + k] + ((l[k] + prev[i + k]) >> 1));
							dst[i + k] = l[k];
						}
					return true;
				}

				case 4: // Paeth
				{
					std::array<u8, BPP> l{}, ul{};
					for (usize i = 0; i < n; i += BPP)
						for (usize k = 0; k < BPP; ++k)
						{
							const u8 up = prev[i + k];
							l[k]        = static_cast<u8>(src[i + k] + paeth(l[k], up, ul[k]));
							ul[k]       = up;
							dst[i + k]  = l[k];
						}
					return true;
				}

				default: return false;
			}
		}

	} // namespace png_detail

	[[nodiscard]] constexpr u32 tag(const char (&s)[5]) noexcept
	{
		auto tag = (u32{u8(s[0])} << 24) | (u32{u8(s[1])} << 16) | (u32{u8(s[2])} << 8) | u32{u8(s[3])};
		return tag;
	}

	struct image_info
	{
		u32 width{0};
		u32 height{0};
		u8  bitdepth{0};
		u8  color_format{0};
		u8  compression_method{0};
		u8  filter_method{0};
		u8  interlace_method{0};

		bool is_valid() const noexcept { return width > 0 and height > 0 and bitdepth > 0 and color_format > 0; }
	};

	export std::expected<image_info, std::string> load_png_info(std::span<const u8> bytes)
	{
		if (bytes.size() < 33)
			return std::unexpected("load_png_info: not enough data");
		
		std::array<u8, 8>   signature{137, 80, 78, 71, 13, 10, 26, 10};
		std::span<const u8> signature_span{signature.data(), signature.size()};
		
		if (not std::ranges::equal(bytes.first(8), signature))
			return std::unexpected("load_png_info: invalid PNG signature");
		
		const u32 length  = read_be<u32>(bytes, 8);
		const u32 type_id = read_be<u32>(bytes, 12);
		
		if (length != 13 or type_id != tag("IHDR"))
			return std::unexpected("load_png_info: invalid IHDR chunk");
	
		image_info info{};
		info.width              = read_be<u32>(bytes, 16);
		info.height             = read_be<u32>(bytes, 20);
		info.bitdepth           = read_be<u8>(bytes, 24);
		info.color_format       = read_be<u8>(bytes, 25);
		info.compression_method = read_be<u8>(bytes, 26);
		info.filter_method      = read_be<u8>(bytes, 27);
		info.interlace_method   = read_be<u8>(bytes, 28);

		return info;
	}

	export std::expected<image_rgba, std::string> load_png(std::span<const u8> bytes, bool validate_checksum = true)
	{
		if (bytes.empty())
			return std::unexpected("load_png: no data to load");

		auto info = load_png_info(bytes);
		if (not info)
			return std::unexpected(info.error());

		u32 width              = info->width;
		u32 height             = info->height;
		u8  bitdepth           = info->bitdepth;
		u8  color_format       = info->color_format;
		u8  filter_method      = info->filter_method;
		u8  compression_method = info->compression_method;
		u8  interlace_method   = info->interlace_method;

		// palette for indexed color images (color type 3)
		// u32 palette[256]{};
		u8  palette[256][4]{};
		u32 palette_entries{0};
		for (auto& p : palette)
		{
			p[0] = 0;
			p[1] = 0;
			p[2] = 0;
			p[3] = 255;
		}

		// ##########################################


		std::span<const u8> first_idat{};  // points into rawdata, valid for the whole function
		std::vector<u8>     idat_joined{}; // only filled when there are 2+ IDAT chunks
		u32                 idat_count{0};

		bool seen_IHDR{false};
		bool seen_IEND{false};
		bool seen_PLTE{false};

		usize offset = 8;
		while (bytes.size() - offset >= 12)
		{
			const u32 length  = read_be<u32>(bytes, offset);
			const u32 type_id = read_be<u32>(bytes, offset + 4);

			if (length > bytes.size() - offset - 12)
				return std::unexpected("load_png: chunk length too large");

			if (offset == 8 and type_id != tag("IHDR"))
				return std::unexpected("load_png: first chunk is not IHDR");

			const auto type_and_data = bytes.subspan(offset + 4, 4 + usize{length});
			const auto data          = type_and_data.subspan(4);
			const auto crc           = bytes.subspan(offset + 8 + usize{length}, 4);

			offset += 12 + usize{length};


			if (data.size() != length)
				return std::unexpected("load_png: invalid chunk length");

			if (validate_checksum)
			{
				if (std::byteswap(as<u32>(crc)) != utils::crc32(type_and_data))
					return std::unexpected("load_png: invalid CRC for chunk");
			}


			// IHDR
			if (type_id == tag("IHDR"))
			{
				if (seen_IHDR)
					return std::unexpected("load_png: multiple IHDR chunks found");

				seen_IHDR = true;

				if (length != 13)
					return std::unexpected("load_png: invalid IHDR chunk length");


				// 0 - Grayscale
				// 2 - RGB
				// 3 - Indexed-color
				// 4 - Grayscale with alpha
				// 6 - RGBA
				//
				// 1 - reserved/invalid
				// 5 - reserved/invalid
				if (color_format == 1 or color_format == 5 or color_format > 6)
					return std::unexpected(std::format("load_png: invalid color format: {}", color_format)); 

				const bool depth_ok = (color_format == 3 or color_format == 0)
										? (bitdepth == 1 or bitdepth == 2 or bitdepth == 4 or bitdepth == 8)
										: bitdepth == 8;

				if (not depth_ok)
					return std::unexpected(
					  std::format("load_png: unsupported bit depth {} for color type {}", bitdepth, color_format));

				if (color_format == 0 and bitdepth < 8) // gray ramp
				{
					const u32 levels = (1u << bitdepth) - 1;
					for (u32 i = 0; i <= levels; ++i)
					{
						const u8 g    = static_cast<u8>(i * 255 / levels);
						palette[i][0] = palette[i][1] = palette[i][2] = g;
						palette[i][3]                                 = 255;
					}
				}


				// 0 - deflate/inflate (only supported method)
				if (compression_method != 0)
					return std::unexpected(std::format("load_png: unsupported compression method: {}", compression_method));


				// 0 - no interlace
				// 1 - Adam7 interlace (not supported)
				if (interlace_method != 0)
					return std::unexpected(std::format("load_png: unsupported interlace method: {}", interlace_method));


				if (filter_method != 0)
					return std::unexpected(std::format("load_png: unsupported filter method: {}", filter_method));


				if (width == 0 or height == 0 or width > 16384 or height > 16384)
					return std::unexpected(std::format("load_png: invalid image dimensions: {}x{}", width, height));


				continue;
			}
			else if (type_id == tag("IDAT"))
			{
				if (idat_count++ == 0)
				{
					first_idat = data;
				}
				else
				{
					if (idat_joined.empty())
					{
						idat_joined.reserve(first_idat.size() + data.size());
						idat_joined.assign(first_idat.begin(), first_idat.end());
					}
					idat_joined.insert(idat_joined.end(), data.begin(), data.end());
				}
				continue;
			}
			else if (type_id == tag("PLTE"))
			{
				if (seen_PLTE or idat_count != 0)
					return std::unexpected("load_png: PLTE duplicated or after IDAT");
				if (length == 0 or length % 3 != 0 or length > 768)
					return std::unexpected("load_png: invalid PLTE chunk length");

				seen_PLTE       = true;
				palette_entries = length / 3;


				if (color_format == 3)
				{
					if (palette_entries > (1u << bitdepth))
						return std::unexpected("load_png: too many palette entries for bit depth");

					for (u32 i = 0; i < palette_entries; ++i)
					{
						palette[i][0] = data[i * 3 + 0];
						palette[i][1] = data[i * 3 + 1];
						palette[i][2] = data[i * 3 + 2];
						palette[i][3] = 255;
					}
				}
				continue;
			}
			else if (type_id == tag("tRNS") and color_format == 3)
			{
				if (not seen_PLTE or idat_count != 0 or length > palette_entries)
					return std::unexpected("load_png: invalid tRNS chunk");

				for (u32 i = 0; i < length; ++i)
					palette[i][3] = data[i];

				continue;
			}
			else if (type_id == tag("IEND"))
			{
				seen_IEND = true;
				break;
			}
		}

		const std::span<const u8> idat = idat_count > 1 ? std::span<const u8>{idat_joined} : first_idat;

		if (idat.empty())
			return std::unexpected("load_png: no IDAT chunk found");

		if (not seen_IHDR)
			return std::unexpected("load_png: missing IHDR chunk");
		if (not seen_IEND)
			return std::unexpected("load_png: missing IEND chunk");
		if (color_format == 3 and not seen_PLTE)
			return std::unexpected("load_png: missing PLTE chunk for indexed color image");

		//
		const u32 channels = [&]() -> u32
		{
			switch (color_format)
			{
				case 0:
				case 3: return 1;
				case 2: return 3;
				case 4: return 2;
				default: return 4;
			}
		}();

		u32 bpp = bitdepth * channels;
		bpp     = std::max<u32>(1, bpp / 8);

		dbg::println(" - {}x{}x{} color format {}", width, height, bpp, color_format);

		// decompress
		const u64 row_bytes     = (u64{width} * channels * bitdepth + 7) / 8;
		const u64 expected_size = u64{height} * (row_bytes + 1);

		if (expected_size > u64{idat.size()} * 1032 + 1024)
			return std::unexpected("load_png: IDAT too small for declared dimensions");

		auto raw = png_decompress(idat, expected_size);
		if (not raw)
			return std::unexpected(std::format("load_png: inflate failed: {}", raw.error()));


		// filter

		if (raw->size() != expected_size)
			return std::unexpected(
			  std::format("load_png: unexpected decompressed size: {} vs {}", raw->size(), expected_size));

		image_rgba img(width, height);

		// view
		u8* const out = reinterpret_cast<u8*>(img.data().data());

		bool ok = false;

		switch (color_format)
		{
			case 0:
				switch (bitdepth)
				{
					case 1: ok = png_detail::decode_rows_indexed<1>(raw->data(), width, height, out, palette); break;
					case 2: ok = png_detail::decode_rows_indexed<2>(raw->data(), width, height, out, palette); break;
					case 4: ok = png_detail::decode_rows_indexed<4>(raw->data(), width, height, out, palette); break;
					case 8: ok = png_detail::decode_rows<1>(raw->data(), width, height, out); break;
				}
				break;
			case 2: ok = png_detail::decode_rows<3>(raw->data(), width, height, out); break;
			case 3:
			{
				switch (bitdepth)
				{
					case 1: ok = png_detail::decode_rows_indexed<1>(raw->data(), width, height, out, palette); break;
					case 2: ok = png_detail::decode_rows_indexed<2>(raw->data(), width, height, out, palette); break;
					case 4: ok = png_detail::decode_rows_indexed<4>(raw->data(), width, height, out, palette); break;
					case 8: ok = png_detail::decode_rows_indexed<8>(raw->data(), width, height, out, palette); break;
				}
				break;
			}
			case 4: ok = png_detail::decode_rows<2>(raw->data(), width, height, out); break;
			case 6: ok = png_detail::decode_rows<4>(raw->data(), width, height, out); break;
		}

		if (not ok)
			return std::unexpected("load_png: unfilter failed");

		return img;
	}

	// TODO: loader to read header first then return image_gray, image_rgb, image_rgba

	//export template<typename T>
	//auto std::expected<T, std::string> load_png2(fs::path file, bool validate_checksum = true)
	//{
	//	return { };
	//}

	export std::expected<image_rgba, std::string> load_png(fs::path file, bool validate_checksum = true)
	{
		const auto rawdata = file::read(file);
		if (rawdata.empty())
			return std::unexpected(std::format("load_png: failed to read file: {}", file.string()));

		return load_png(rawdata, validate_checksum);
	}
} // namespace deckard::image
