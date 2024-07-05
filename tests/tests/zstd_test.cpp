#include <catch2/catch_test_macros.hpp>

import deckard.types;
import deckard.zstd;
import std;

using namespace deckard;

TEST_CASE("zstd", "[zstd]")
{
	std::vector<u8> buffer(1);
	std::vector<u8> comp;
	comp.resize(zstd::bound(buffer));

	auto result = zstd::compress(buffer, comp);
	REQUIRE(result != std::nullopt);
	comp.resize(*result);

	std::vector<u8> uncompressed;
	if (auto s = zstd::uncompressed_size(comp); s)
		uncompressed.resize(*s);

	result = zstd::uncompress(comp, uncompressed);

	REQUIRE(result != std::nullopt);
}
