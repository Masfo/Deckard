
#include <doctest/doctest.h>

import deckard.tokenizer;
import std;

using namespace deckard;

TEST_CASE("tokenizer" * doctest::description("description") * doctest::skip(false))
{

	//
	// CHECK_EQ(1, 1);
	SUBCASE("no tokens")
	{
		//
		CHECK_EQ(1, 1);
	}
}

/*
TEST_SUITE("tokenizer")
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
*/