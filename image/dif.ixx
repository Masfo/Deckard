export module deckard.image:dif;

import std;
import deckard.types;
import deckard.file;
import deckard.assert;
import deckard.helpers;
import deckard.zstd;

import :common;

namespace deckard::image
{
	// ###############################################################################################
	// DIF - Deckard Image Format ####################################################################
	// QOI + ZSTD


	struct DIF_Header
	{
		std::array<u8, 4> magic{'D', 'I', 'F', '0'};
		u16               width{0};
		u16               height{0};
		u8                channels{3};
		u8                reserved{0};
	};

	constexpr u32 DIF_HEADER_SIZE = sizeof(DIF_Header);

	constexpr u32 DEFAULT_COMPRESSION_LEVEL = 5;

	export std::expected<u64, std::string> encode_dif_rgb(const image_rgb& img, std::span<u8> buffer, u32 compression_level=DEFAULT_COMPRESSION_LEVEL)
	{
		const u16 width  = img.width();
		const u16 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("encode_dif: image has zero dimensions");

		const u32 pixel_bytes  = width * height * 3u;
		const u32 header_bytes = DIF_HEADER_SIZE;

		// Compress image using deckard::zstd
		std::vector<u8> compressed_data;
		compressed_data.resize(zstd::bound(pixel_bytes));
		auto compressed_size = zstd::compress(img.raw_data(), std::span{compressed_data.data(), compressed_data.size()}, compression_level);
		if (not compressed_size)
			return std::unexpected("encode_dif: compression failed");

		compressed_data.resize(*compressed_size);

		const u32 file_bytes = header_bytes + static_cast<u32>(compressed_data.size());
		if (buffer.size() < file_bytes)
			return std::unexpected(std::format("encode_dif: buffer too small (need {}, got {})", file_bytes, buffer.size()));

		// Create and write DIF header directly
		DIF_Header header;
		header.magic    = {'D', 'I', 'F', '1'};
		header.width    = width;
		header.height   = height;
		header.channels = 3;
		header.reserved = 0;

		static_assert(std::endian::native == std::endian::little, "DIF header assumes little-endian platform");

		// Write header to buffer (assumes little-endian platform or uses std::bit_cast if needed)
		std::memcpy(buffer.data(), &header, header_bytes);

		// Write compressed data directly to buffer
		std::memcpy(buffer.data() + header_bytes, compressed_data.data(), compressed_data.size());

		return file_bytes;
	}

	export std::expected<image_rgb, std::string> decode_dif_rgb(std::span<const u8> buffer)
	{
		if (buffer.size() < DIF_HEADER_SIZE)
			return std::unexpected("decode_dif_rgb: buffer too small");

		// Read header directly from buffer
		const DIF_Header* header = reinterpret_cast<const DIF_Header*>(buffer.data());

		// Validate magic
		if (header->magic[0] != 'D' or header->magic[1] != 'I' or header->magic[2] != 'F' or header->magic[3] != '1')
			return std::unexpected("decode_dif_rgb: invalid magic");

		const u16 width    = header->width;
		const u16 height   = header->height;
		const u8  channels = header->channels;

		if (width == 0 or height == 0)
			return std::unexpected("decode_dif_rgb: image has zero dimensions");
		if (channels != 3u)
			return std::unexpected("decode_dif_rgb: invalid number of channels");

		// Compressed data starts after header
		const u8*       compressed_ptr  = buffer.data() + DIF_HEADER_SIZE;
		const size_t    compressed_size = buffer.size() - DIF_HEADER_SIZE;
		std::vector<u8> compressed_data(compressed_ptr, compressed_ptr + compressed_size);

		const u64       expected_size = static_cast<u64>(width) * static_cast<u64>(height) * 3u;
		std::vector<u8> decompressed(expected_size);
		auto            result = zstd::decompress(
		  std::span{compressed_data.data(), compressed_data.size()}, std::span{decompressed.data(), decompressed.size()});
		if (not result)
			return std::unexpected("decode_dif_rgb: decompression failed");
		if (*result != expected_size)
			return std::unexpected("decode_dif_rgb: decompressed size does not match expected size");

		image_rgb img(width, height);
		std::memcpy(const_cast<u8*>(img.raw_data().data()), decompressed.data(), expected_size);
		return img;
	}

	export std::expected<void, std::string> save_dif_rgb(std::filesystem::path path, const image_rgb& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("save_dif_rgb: image has zero dimensions");

		// Estimate max size: header (10) + compressed data (zstd bound)
		std::vector<u8> out;
		out.resize(DIF_HEADER_SIZE + zstd::bound(width * height * 3u));
		auto encoded = encode_dif_rgb(img, std::span{out.data(), out.size()});
		if (not encoded)
			return std::unexpected("save_dif_rgb: encoding failed");

		out.resize(*encoded);
		auto result = file::write(
		  {.filename = path, .buffer = std::span{out.data(), out.size()}, .mode = file::filemode::overwrite});
		if (not result)
			return std::unexpected("save_dif_rgb: file write failed");
		if (*result != out.size())
			return std::unexpected("save_dif_rgb: incomplete file write");
		return {};
	}

	export std::expected<u64, std::string> encode_dif_rgba(const image_rgba& img, std::span<u8> buffer, u32 compression_level=DEFAULT_COMPRESSION_LEVEL)
	{
		const u16 width  = img.width();
		const u16 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("encode_dif: image has zero dimensions");

		const u32 pixel_bytes  = width * height * 4u;
		const u32 header_bytes = DIF_HEADER_SIZE;

		// Compress image using deckard::zstd
		std::vector<u8> compressed_data;
		compressed_data.resize(zstd::bound(pixel_bytes));
		auto compressed_size = zstd::compress(img.raw_data(), std::span{compressed_data.data(), compressed_data.size()}, compression_level);
		if (not compressed_size)
			return std::unexpected("encode_dif: compression failed");

		compressed_data.resize(*compressed_size);

		const u32 file_bytes = header_bytes + static_cast<u32>(compressed_data.size());
		if (buffer.size() < file_bytes)
			return std::unexpected(std::format("encode_dif: buffer too small (need {}, got {})", file_bytes, buffer.size()));

		// Create and write DIF header directly
		DIF_Header header;
		header.magic    = {'D', 'I', 'F', '1'};
		header.width    = width;
		header.height   = height;
		header.channels = 4;
		header.reserved = 0;

		// Write header to buffer
		std::memcpy(buffer.data(), &header, header_bytes);

		// Write compressed data directly to buffer
		std::memcpy(buffer.data() + header_bytes, compressed_data.data(), compressed_data.size());

		return file_bytes;
	}

	export std::expected<image_rgba, std::string> decode_dif_rgba(std::span<const u8> buffer)
	{
		if (buffer.size() < DIF_HEADER_SIZE)
			return std::unexpected("decode_dif_rgba: buffer too small");

		// Read header directly from buffer
		const DIF_Header* header = reinterpret_cast<const DIF_Header*>(buffer.data());

		// Validate magic
		if (header->magic[0] != 'D' or header->magic[1] != 'I' or header->magic[2] != 'F' or header->magic[3] != '1')
			return std::unexpected("decode_dif_rgba: invalid magic");

		const u16 width    = header->width;
		const u16 height   = header->height;
		const u8  channels = header->channels;

		if (width == 0 or height == 0)
			return std::unexpected("decode_dif_rgba: image has zero dimensions");
		if (channels != 4u)
			return std::unexpected("decode_dif_rgba: invalid number of channels");

		// Compressed data starts after header
		const u8*       compressed_ptr  = buffer.data() + DIF_HEADER_SIZE;
		const size_t    compressed_size = buffer.size() - DIF_HEADER_SIZE;
		std::vector<u8> compressed_data(compressed_ptr, compressed_ptr + compressed_size);

		const u64       expected_size = static_cast<u64>(width) * static_cast<u64>(height) * 4u;
		std::vector<u8> decompressed(expected_size);
		auto            result = zstd::decompress(
		  std::span{compressed_data.data(), compressed_data.size()}, std::span{decompressed.data(), decompressed.size()});
		if (not result)
			return std::unexpected("decode_dif_rgba: decompression failed");
		if (*result != expected_size)
			return std::unexpected("decode_dif_rgba: decompressed size does not match expected size");

		image_rgba img(width, height);
		std::memcpy(const_cast<u8*>(img.raw_data().data()), decompressed.data(), expected_size);
		return img;
	}

	export std::expected<void, std::string> save_dif_rgba(std::filesystem::path path, const image_rgba& img)
	{
		const u32 width  = img.width();
		const u32 height = img.height();
		if (width == 0 or height == 0)
			return std::unexpected("save_dif_rgba: image has zero dimensions");

		// Estimate max size: header (10) + compressed data (zstd bound)
		std::vector<u8> out;
		out.resize(DIF_HEADER_SIZE + zstd::bound(width * height * 4u));
		auto encoded = encode_dif_rgba(img, std::span{out.data(), out.size()});
		if (not encoded)
			return std::unexpected("save_dif_rgba: encoding failed");

		out.resize(*encoded);
		
		auto result = file::write(
		  {.filename = path, .buffer = std::span{out.data(), out.size()}, .mode = file::filemode::overwrite});
		if (not result)
			return std::unexpected("save_dif_rgba: file write failed");
		if (*result != out.size())
			return std::unexpected("save_dif_rgba: incomplete file write");
		return {};
	}

	export std::expected<image_rgb, std::string> load_dif_rgb(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_dif_rgb(std::span<const u8>{bytes.data(), bytes.size()});
	}

	export std::expected<image_rgba, std::string> load_dif_rgba(const std::filesystem::path& path)
	{
		const auto bytes = file::read(path);
		return decode_dif_rgba(std::span<const u8>{bytes.data(), bytes.size()});
	}

	export template<typename T = image_rgba>
	auto load_dif(const std::filesystem::path& path) -> std::expected<T, std::string>
	{
		if constexpr (std::is_same_v<T, image_rgb>)
		{
			return load_dif_rgb(path);
		}
		else if constexpr (std::is_same_v<T, image_rgba>)
		{
			return load_dif_rgba(path);
		}
		else
		{
			static_assert(false, "Unsupported type for load_dif");
		}
	}

	export template<typename T = image_rgba>
	auto save_dif(std::filesystem::path path, const T& img) -> std::expected<void, std::string>
	{

		if constexpr (std::is_same_v<T, image_rgb>)
		{
			return save_dif_rgb(path, img);
		}
		else if constexpr (std::is_same_v<T, image_rgba>)
		{
			return save_dif_rgba(path, img);
		}
		else
		{
			static_assert(false, "Unsupported type for save_dif");
		}
	}

	export template<typename T = image_rgba>
	auto save_dif(std::filesystem::path path, const std::optional<T> img) -> std::expected<void, std::string>
	{
		if (not img.has_value())
			return std::unexpected("save_dif: no image given");

		return save_dif<T>(path, *img);
	}
}
