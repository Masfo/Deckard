
#include <doctest/doctest.h>

import deckard.sha256;
import std;

using namespace deckard::sha256;
using namespace std::string_literals;

TEST_SUITE("SHA256" * doctest::description("SHA256 - Cryptographic hash function"))
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

		hasher.update("abc");
		auto digest = hasher.finalize().to_string();

		CHECK_EQ(digest, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"s);
	}
}
