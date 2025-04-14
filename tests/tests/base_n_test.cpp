#include <catch2/catch_test_macros.hpp>


import deckard.base64;
import deckard.base32;
import std;

using namespace deckard::utils;
using namespace std::string_view_literals;

TEST_CASE("base64", "[base64]") 
{
	SECTION("encode") 
	{ 
		CHECK(base64::encode_str("") == ""sv);

		// w/ padding
		CHECK(base64::encode_str("foob") == "Zm9vYg=="sv);

		// w/o padding
		CHECK(base64::encode_str("foob", base64::padding::no) == "Zm9vYg"sv);

		// sphinx
		CHECK(base64::encode_str("Sphinx of black quartz, judge my vow") == "U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93"sv);
	}

	SECTION("decode") 
	{
		// w/ padding
		CHECK(base64::decode_str("Zm9vYg==") == "foob"sv);
		
		// w/o padding
		CHECK(base64::decode_str("Zm9vYg") == "foob"sv);

		// sphinx
		CHECK(base64::decode_str("U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93") == "Sphinx of black quartz, judge my vow"sv);
		  
	}
}


TEST_CASE("base32", "[base32]")
{
	SECTION("encode")
	{
		CHECK(base32::encode_str("") == ""sv);

		// w/ padding
		CHECK(base32::encode_str("foob") == "MZXW6YQ="sv);

		// w/o padding
		CHECK(base32::encode_str("foob", base32::padding::no) == "MZXW6YQ"sv);

		// sphinx
		CHECK(base32::encode_str("Sphinx of black quartz, judge my vow") == "KNYGQ2LOPAQG6ZRAMJWGCY3LEBYXKYLSOR5CYIDKOVSGOZJANV4SA5TPO4======"sv);
	}

	SECTION("decode")
	{
		CHECK(base32::encode_str("") == ""sv);

		// w/ padding
		CHECK(base32::decode_str("MZXW6YQ=") == "foob"sv);

		// w/o padding
		CHECK(base32::decode_str("MZXW6YQ") == "foob"sv);

		// sphinx
		CHECK(base32::decode_str("KNYGQ2LOPAQG6ZRAMJWGCY3LEBYXKYLSOR5CYIDKOVSGOZJANV4SA5TPO4") == "Sphinx of black quartz, judge my vow"sv);
	}
}
