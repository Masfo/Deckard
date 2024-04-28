
#include <doctest/doctest.h>

import deckard.base64;
import std;

using namespace deckard::utils;
using namespace std::string_literals;

TEST_SUITE("base64" * doctest::description("base64"))
{
	TEST_CASE("base64('foob') - w/ padding")
	{
		//
		CHECK_EQ(base64::encode_str("foob"), "Zm9vYg=="s);
	}

	TEST_CASE("base64('foob') - w/o padding")
	{
		//
		CHECK_EQ(base64::encode_str("foob", base64::padding::no), "Zm9vYg"s);
	}
}
