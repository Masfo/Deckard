#include <catch2/catch_test_macros.hpp>

import deckard.types;
import deckard.zstd;
import std;

using namespace deckard;

TEST_CASE("zstd", "[zstd]")
{
	SECTION("api")
	{
		std::vector<u8> buffer(256);
		std::vector<u8> comp;
		comp.resize(zstd::bound(buffer));

		// Compress
		auto result = zstd::compress(buffer, comp);
		REQUIRE(result != std::nullopt);
		comp.resize(*result);

		// Uncompressed size
		auto uncompressed_size = zstd::uncompressed_size(comp);
		REQUIRE(uncompressed_size != std::nullopt);
		REQUIRE(*uncompressed_size == buffer.size());

		// Uncompress
		std::vector<u8> uncompressed;
		if (auto s = zstd::uncompressed_size(comp); s)
			uncompressed.resize(*s);
		else
			REQUIRE(s != std::nullopt);


		result = zstd::uncompress(comp, uncompressed);

		REQUIRE(result != std::nullopt);
	}
}
