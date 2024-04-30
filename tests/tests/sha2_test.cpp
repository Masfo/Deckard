
#include <doctest/doctest.h>

import deckard.sha2;
import std;

using namespace deckard;
using namespace std::string_literals;

TEST_SUITE("SHA256" * doctest::description("SHA256 - Cryptographic hash function"))
{
	TEST_CASE("sha256('')")
	{
		//
		CHECK_EQ(sha256::quickhash(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"s);
	}

	TEST_CASE("sha256('abc')")
	{
		//
		CHECK_EQ(sha256::quickhash("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"s);
	}
}
