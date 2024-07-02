module;
#include <zstd.h>

export module deckard.zstd;

import deckard.types;
import std;

namespace deckard::zstd
{

	export size_t bound(std::span<u8> input) { return ZSTD_compressBound(input.size()); }

	export bool compress(std::span<u8> input, std::span<u8> output) { return true; }


}; // namespace deckard::zstd
