module;
#include <zstd.h>

export module deckard.zstd;
import deckard.debug;

namespace deckard::compress
{


	export void test() { dbg::println("{}", ZSTD_versionString()); }
}; // namespace deckard::compress
