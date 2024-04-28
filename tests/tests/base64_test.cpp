
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

	TEST_CASE("base64('sphinx....')")
	{
		//
		CHECK_EQ(base64::encode_str("Sphinx of black quartz, judge my vow"), "U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93"s);
	}
}
