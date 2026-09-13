#include <catch2/catch_test_macros.hpp>


import deckard.base_encoding;
import std;

using namespace deckard::utils;
using namespace std::string_view_literals;

TEST_CASE("base64", "[base64]")
{
	SECTION("encode")
	{
		CHECK(base64::encode_str("") == ""sv);

		CHECK(base64::encode_str("hello world") == "aGVsbG8gd29ybGQ="sv);


		// w/ padding
		CHECK(base64::encode_str("foob") == "Zm9vYg=="sv);

		// w/o padding
		CHECK(base64::encode_str("foob", base64::padding::no) == "Zm9vYg"sv);

		// sphinx
		CHECK(base64::encode_str("Sphinx of black quartz, judge my vow")
			  == "U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93"sv);
	}

	SECTION("decode")
	{
		// w/ padding
		CHECK(base64::decode_str("Zm9vYg==") == "foob"sv);

		// w/o padding
		CHECK(base64::decode_str("Zm9vYg") == "foob"sv);

		// sphinx
		CHECK(base64::decode_str("U3BoaW54IG9mIGJsYWNrIHF1YXJ0eiwganVkZ2UgbXkgdm93")
			  == "Sphinx of black quartz, judge my vow"sv);
	}
}

TEST_CASE("base32", "[base32]")
{
	SECTION("encode")
	{
		CHECK(base32::encode_str("") == ""sv);

		CHECK(base32::encode_str("hello world") == "NBSWY3DPEB3W64TMMQ======"sv);

		// w/ padding
		CHECK(base32::encode_str("foob") == "MZXW6YQ="sv);

		// w/o padding
		CHECK(base32::encode_str("foob", base32::padding::no) == "MZXW6YQ"sv);

		// sphinx
		CHECK(base32::encode_str("Sphinx of black quartz, judge my vow")
			  == "KNYGQ2LOPAQG6ZRAMJWGCY3LEBYXKYLSOR5CYIDKOVSGOZJANV4SA5TPO4======"sv);
	}

	SECTION("decode")
	{
		CHECK(base32::encode_str("") == ""sv);

		// w/ padding
		CHECK(base32::decode_str("MZXW6YQ=") == "foob"sv);

		// w/o padding
		CHECK(base32::decode_str("MZXW6YQ") == "foob"sv);

		// sphinx
		CHECK(base32::decode_str("KNYGQ2LOPAQG6ZRAMJWGCY3LEBYXKYLSOR5CYIDKOVSGOZJANV4SA5TPO4")
			  == "Sphinx of black quartz, judge my vow"sv);
	}
}

TEST_CASE("base85", "[base85]")
{
	SECTION("encode")
	{
		auto r = base85::encode("");
		CHECK(r.empty());

		CHECK(base85::encode("") == ""sv);

		CHECK(base85::encode("a") == "fO"sv);
		CHECK(base85::encode("ab") == "fZh"sv);
		CHECK(base85::encode("abc") == "fZk9"sv);
		CHECK(base85::encode("abcd") == "fZk!2"sv);
		CHECK(base85::encode("abcde") == "fZk!2gn"sv);
		CHECK(base85::encode("abcdef") == "fZk!2gyH"sv);
		CHECK(base85::encode("abcdefg") == "fZk!2gyLw"sv);
		CHECK(base85::encode("abcdefgh") == "fZk!2gyLx+"sv);

		CHECK(base85::encode("Sphinx of black quartz, judge my vow") == "a)nj$jq[Zhg)~wAfZuG`knv8QllCp7iScGBgq4-XKk,6o"sv);
	}

	SECTION("decode")
	{
		CHECK(base85::decode_as_string("fO") == "a"sv);
		CHECK(base85::decode_as_string("fZh") == "ab"sv);
		CHECK(base85::decode_as_string("fZk9") == "abc"sv);
		CHECK(base85::decode_as_string("fZk!2") == "abcd"sv);
		CHECK(base85::decode_as_string("fZk!2gn") == "abcde"sv);
		CHECK(base85::decode_as_string("fZk!2gyH") == "abcdef"sv);
		CHECK(base85::decode_as_string("fZk!2gyLw") == "abcdefg"sv);
		CHECK(base85::decode_as_string("fZk!2gyLx+") == "abcdefgh"sv);

		CHECK(base85::decode_as_string("a)nj$jq[Zhg)~wAfZuG`knv8QllCp7iScGBgq4-XKk,6o")
			  == "Sphinx of black quartz, judge my vow"sv);
	}

	SECTION("decode invalid")
	{
		auto r = base85::decode_as_string("AB CD");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Invalid character ' ' at position 2"sv);

		r = base85::decode_as_string("ABC?D");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Invalid character '?' at position 3"sv);

		r = base85::decode_as_string("ABCD\n");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Invalid character '\n' at position 4"sv);

		r = base85::decode_as_string("A");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Invalid length at position 0"sv);

		r = base85::decode_as_string("AAAAAA");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Invalid length at position 5"sv);

		r = base85::decode_as_string("~~~~~");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Value overflow at position 0: 0x105520e97 (exceeds 0xFFFFFFFF)"sv);

		r = base85::decode_as_string("~~~~");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Value overflow at position 0: 0x105520e98 (exceeds 0xFFFFFFFF)"sv);

		r = base85::decode_as_string("AAAAA~~~");
		CHECK_FALSE(r.has_value());
		CHECK(r.error() == "Value overflow at position 5: 0x105520eed (exceeds 0xFFFFFFFF)"sv);
	}
}
