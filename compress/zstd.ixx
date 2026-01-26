module;
#include <zstd.h>

export module deckard.zstd;

import deckard.types;
import deckard.debug;
import std;

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

	export [[nodiscard]] std::optional<u64> compress(const std::span<u8> input, std::span<u8> output)
	{
		if (output.size() < bound(input))
		{
			dbg::println("ZSTD: output size too small({}), should be atleast {}", output.size(), bound(input));
			return {};
		}
		i32 level = ZSTD_maxCLevel();
		if (compression_level > 0)
			level = std::clamp(compression_level, ZSTD_minCLevel(), ZSTD_maxCLevel());

		const int level = ZSTD_maxCLevel();

		u64 r = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), level);
		if (ZSTD_isError(r))
			return {};

		return r;
	}

	export [[nodiscard]] std::optional<u64> uncompress(const std::span<u8> input, std::span<u8> output)
	{
		auto content_size = uncompressed_size(input);

		if (!content_size)
		{
			dbg::println("ZSTD_decompress: failed to get uncompressed size");
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


}; // namespace deckard::zstd
