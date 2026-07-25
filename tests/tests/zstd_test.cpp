#include <catch2/catch_test_macros.hpp>

import deckard.types;
import deckard.zstd;
import std;

using namespace deckard;

TEST_CASE("zstd", "[zstd]")
{
	SECTION("api")
	{
		std::vector<u8> input(256);
		std::ranges::fill(input, 'A');

		std::vector<u8> output;
		output.resize(zstd::bound(input));

		// Compress
		auto result = zstd::compress(input, output);
		CHECK(result.has_value());
		CHECK(*result <= input.size());
		output.resize(*result);


		// Uncompressed size
		auto uncompressed_size = zstd::decompressed_size(output);
		CHECK(uncompressed_size.has_value());
		CHECK(*uncompressed_size == input.size());

		// Uncompress
		std::vector<u8> uncompressed{};
		if (auto s = zstd::decompressed_size(output); s)
		{
			uncompressed.resize(*s);
			CHECK(uncompressed.size() == input.size());
		}
		else
			CHECK(s.has_value());


		auto decompressed_size = zstd::decompress(output, uncompressed);

		CHECK(decompressed_size.has_value());
		CHECK(*decompressed_size == input.size());
		CHECK(uncompressed == input);
	}
}
