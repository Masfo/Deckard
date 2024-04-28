
#include <doctest/doctest.h>

import deckard.base64;
import std;

using namespace deckard::utils;
using namespace std::string_literals;

// Encode
TEST_SUITE("encode base64")
{
	TEST_CASE("encode base64('foob') - w/ padding")
	{
		//
		CHECK_EQ(base64::encode_str("foob"), "Zm9vYg=="s);
	}

	TEST_CASE("encode base64('foob') - w/o padding")
	{
		//
		CHECK_EQ(base64::encode_str("foob", base64::padding::no), "Zm9vYg"s);
	}

	TEST_CASE("encode base64('sphinx....')")
	{
		//
		CHECK_EQ(base64::encode_str("Sphinx of black quartz, judge my vow"), "U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93"s);
	}
}

// Decode
TEST_SUITE("decode base64")
{
	TEST_CASE("decode base64('foob') - w/ padding")
	{
		//
		CHECK_EQ(base64::decode_str("Zm9vYg=="), "foob"s);
	}

	TEST_CASE("decode base64('foob') - w/o padding")
	{
		//
		CHECK_EQ(base64::decode_str("Zm9vYg"), "foob"s);
	}

	TEST_CASE("decode base64('sphinx....')")
	{
		//
		CHECK_EQ(base64::decode_str("U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93"), "Sphinx of black quartz, judge my vow"s);
	}
}
