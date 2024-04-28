
#include <doctest/doctest.h>

import deckard.sha256;
import std;

using namespace deckard::sha256;
using namespace std::string_literals;

TEST_SUITE("SHA256" * doctest::description(""))
{
	TEST_CASE("sha256('')")
	{
		sha256 hasher;

		auto digest = hasher.finalize().to_string();

		CHECK_EQ(digest, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"s);
	}

	TEST_CASE("sha256('foobar')")
	{
		sha256 hasher;

		hasher.update("foobar");
		auto digest = hasher.finalize().to_string();

		CHECK_EQ(digest, "c3ab8ff13720e8ad9047dd39466b3c8974e592c2fa383d4a3960714caef0c4f2"s);
	}
}
