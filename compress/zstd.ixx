module;
#include <zstd.h>

export module deckard.zstd;

import deckard.types;
import deckard.debug;
import deckard.file;
import deckard.helpers;
import std;

namespace fs = std::filesystem;

namespace deckard::zstd
{

	export [[nodiscard]] u64 bound(const u64 input_size) { return ZSTD_compressBound(input_size); }

	export [[nodiscard]] u64 bound(std::span<const u8> input) { return ZSTD_compressBound(input.size()); }

	export [[nodiscard]] std::optional<u64> decompressed_size(std::span<const u8> compressed_input)
	{

		u64 result = ZSTD_getFrameContentSize(compressed_input.data(), compressed_input.size());
		if (result == ZSTD_CONTENTSIZE_UNKNOWN or result == ZSTD_CONTENTSIZE_ERROR)
		{
			return {};
		}
		return result;
	}

	export [[nodiscard]] std::expected<u64, std::string>
	unbound_compress(std::span<const u8> input, std::span<u8> output, i32 compression_level = -1)
	{
		i32 level = ZSTD_maxCLevel();
		if (compression_level > 0)
			level = std::clamp(compression_level, ZSTD_minCLevel(), ZSTD_maxCLevel());

		u64 compressed_size = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), level);
		if (ZSTD_isError(compressed_size))
		{
			return std::unexpected(ZSTD_getErrorName(compressed_size));
		}

		return compressed_size;
	}

	export [[nodiscard]] std::expected<u64, std::string>
	compress(std::span<const u8> input, std::span<u8> output, i32 compression_level = -1)
	{
		if (output.size() < bound(input))
		{
			return std::unexpected(
			  std::format("ZSTD: output size too small({}), should be atleast {}", output.size(), bound(input)));
		}
		return unbound_compress(input, output, compression_level);
	}

	export [[nodiscard]] std::optional<u64> decompress(std::span<const u8> input, std::span<u8> output)
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

		u64 r = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
		if (ZSTD_isError(r))
		{
			dbg::println("ZSTD_decompress failed to decompress");
			return {};
		}
		return r;
	}

	export [[nodiscard]] std::optional<u64>
	compress_file_to(const fs::path& path1, const fs::path& path2, i32 compression_level = -1)
	{
		// read file and compress it, save to path2
		auto file_data = file::read_file(path1);
		if (file_data.empty())
		{
			dbg::println("compress_file_to: could not read file '{}'", path1.string());
			return {};
		}
		std::vector<u8> compressed_data;
		compressed_data.resize(bound(file_data.size()));
		auto compressed_size =
		  compress(std::span{file_data.data(), file_data.size()},
				   std::span{compressed_data.data(), compressed_data.size()},
				   compression_level);
		if (not compressed_size)
		{
			dbg::println("compress_file_to: compression failed for file '{}'", path1.string());
			return {};
		}

		compressed_data.resize(*compressed_size);

		auto write_result =
		  file::write({.filename = path2, .buffer = std::span{compressed_data.data(), compressed_data.size()}});
		if (!write_result || *write_result != compressed_data.size())
		{
			dbg::println("compress_file_to: could not write compressed data to file '{}'", path2.string());
			return {};
		}
		return *compressed_size;
	}

	// compress in buffer to out buffer only if compresses to smaller size
	// return: compressed size
	export [[nodiscard]] std::optional<u64>
	compress_if_smaller(std::span<const u8> in, std::span<u8> out, u64 threshold_bytes = 0, i32 compression_level=-1)
	{
		if (out.size() < in.size())
			return {};

		auto compressed_size = unbound_compress(in, out, compression_level);
		if (not compressed_size)
			return {};

		if (compressed_size and ((*compressed_size + threshold_bytes) >= in.size()))
		{
			std::ranges::fill(out, 0_u8);
			return {};
		}

		return *compressed_size;
	}


} // namespace deckard::zstd
