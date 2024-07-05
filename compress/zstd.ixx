module;
#include <zstd.h>

export module deckard.zstd;

import deckard.types;
import deckard.debug;
import std;

namespace deckard::zstd
{

	export size_t bound(std::span<u8> input) { return ZSTD_compressBound(input.size()); }

	export std::optional<size_t> uncompressed_size(std::span<u8> compressed_input)
	{

		size_t result = ZSTD_getFrameContentSize(compressed_input.data(), compressed_input.size());
		if (result == ZSTD_CONTENTSIZE_UNKNOWN or result == ZSTD_CONTENTSIZE_ERROR)
		{
			return {};
		}
		return result;
	}

	export [[nodiscard]] std::optional<size_t> compress(std::span<u8> input, std::span<u8> output) noexcept
	{
		if (output.size() < bound(input))
		{
			dbg::println("ZSTD: output size too small({}), should be atleast {}", output.size(), bound(input));
			return {};
		}

		const int level = ZSTD_maxCLevel();

		size_t r = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), level);
		if (ZSTD_isError(r))
			return {};

		return r;
	}

	export [[nodiscard]] std::optional<size_t> uncompress(std::span<u8> input, std::span<u8> output) noexcept
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

		size_t r = r = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
		if (ZSTD_isError(r))
		{
			dbg::println("ZSTD_decompress failed to decompress");
			return {};
		}
		return r;
	}


}; // namespace deckard::zstd
