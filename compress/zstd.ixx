module;
#include <zstd.h>

export module deckard.zstd;

import deckard.types;
import deckard.debug;
import deckard.file;
import std;

namespace fs = std::filesystem;

namespace deckard::zstd
{

	export u64 bound(const u64 input_size) { return ZSTD_compressBound(input_size); }

	export u64 bound(const std::span<const u8> input) { return ZSTD_compressBound(input.size()); }

	export std::optional<u64> decompressed_size(const std::span<const u8> compressed_input)
	{

		u64 result = ZSTD_getFrameContentSize(compressed_input.data(), compressed_input.size());
		if (result == ZSTD_CONTENTSIZE_UNKNOWN or result == ZSTD_CONTENTSIZE_ERROR)
		{
			return {};
		}
		return result;
	}

	export [[nodiscard]] std::optional<u64> compress(const std::span<const u8> input, std::span<u8> output, i32 compression_level = -1)
	{
		if (output.size() < bound(input))
		{
			dbg::println("ZSTD: output size too small({}), should be atleast {}", output.size(), bound(input));
			return {};
		}
		i32 level = ZSTD_maxCLevel();
		if (compression_level > 0)
			level = std::clamp(compression_level, ZSTD_minCLevel(), ZSTD_maxCLevel());

		u64 r = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), level);
		if (ZSTD_isError(r))
			return {};

		return r;
	}

	export [[nodiscard]] std::optional<u64> decompress(const std::span<u8> input, std::span<u8> output)
	{
		auto content_size = decompressed_size(input);

		if (!content_size)
		{
			dbg::println("ZSTD_decompress: failed to get decompressed size");
			return {};
		}

		if (output.size() < *content_size)
		{
			dbg::println("ZSTD_decompress: output buffer too small({}), should be atleast {}", output.size(), *content_size);
			return {};
		}

		u64 r = r = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
		if (ZSTD_isError(r))
		{
			dbg::println("ZSTD_decompress failed to decompress");
			return {};
		}
		return r;
	}

	export [[nodiscard]] std::optional<u64> compress_file_to(const fs::path path1, const fs::path path2, i32 compression_level = -1)
	{
		// read file and compress it, save to path2
		auto file_data = file::read_file(path1);
		if (file_data.empty())
		{
			dbg::println("recompress_file: could not read file '{}'", path1.string());
			return {};
		}
		std::vector<u8> compressed_data;
		compressed_data.resize(bound(file_data.size()));
		auto compressed_size = compress(
		  std::span{file_data.data(), file_data.size()}, std::span{compressed_data.data(), compressed_data.size()}, compression_level);
		if (!compressed_size)
		{
			dbg::println("recompress_file: compression failed for file '{}'", path1.string());
			return {};
		}
		compressed_data.resize(*compressed_size);
		auto write_result = file::write({.file = path2, .buffer = std::span{compressed_data.data(), compressed_data.size()}});
		if (!write_result || *write_result != compressed_data.size())
		{
			dbg::println("recompress_file: could not write compressed data to file '{}'", path2.string());
			return {};
		}
		return *compressed_size;
	}

}; // namespace deckard::zstd
