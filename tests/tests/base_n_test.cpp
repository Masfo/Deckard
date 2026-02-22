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

		CHECK(base32::encode_str("hello world") == "NBSWY3DPEB3W64TMMQ======"sv);

		// w/ padding
		CHECK(base32::encode_str("foob") == "MZXW6YQ="sv);

		// w/o padding
		CHECK(base32::encode_str("foob", base32::padding::no) == "MZXW6YQ"sv);

		// sphinx
		CHECK(base32::encode_str("Sphinx of black quartz, judge my vow") ==
			  "KNYGQ2LOPAQG6ZRAMJWGCY3LEBYXKYLSOR5CYIDKOVSGOZJANV4SA5TPO4======"sv);
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

TEST_CASE("base95", "[base95]")
{
	SECTION("encode")
	{
		CHECK(base85::encode("") == ""sv);

		CHECK(base85::encode("a") == "ve"sv);
		CHECK(base85::encode("ab") == "vpx"sv);
		CHECK(base85::encode("abc") == "vpAZ"sv);
		CHECK(base85::encode("abcd") == "vpA.S"sv);
		CHECK(base85::encode("abcde") == "vpA.SwD"sv);
		CHECK(base85::encode("abcdef") == "vpA.SwO7"sv);
		CHECK(base85::encode("abcdefg") == "vpA.SwObM"sv);
		CHECK(base85::encode("abcdefgh") == "vpA.SwObN*"sv);

		CHECK(base85::encode("Sphinx of black quartz, judge my vow") == "q/Dz:zG)pxw/$M0vpK6}ADLYgBB2FXyis61wGU&naA?WE"sv);
	}

	SECTION("decode")
	{
		CHECK(base85::decode_as_str("ve") == "a"sv);
		CHECK(base85::decode_as_str("vpx") == "ab"sv);
		CHECK(base85::decode_as_str("vpAZ") == "abc"sv);
		CHECK(base85::decode_as_str("vpA.S") == "abcd"sv);
		CHECK(base85::decode_as_str("vpA.SwD") == "abcde"sv);
		CHECK(base85::decode_as_str("vpA.SwO7") == "abcdef"sv);
		CHECK(base85::decode_as_str("vpA.SwObM") == "abcdefg"sv);
		CHECK(base85::decode_as_str("vpA.SwObN*") == "abcdefgh"sv);

		CHECK(base85::decode_as_str("q/Dz:zG)pxw/$M0vpK6}ADLYgBB2FXyis61wGU&naA?WE") == "Sphinx of black quartz, judge my vow"sv);
	}

	SECTION("decode invalid")
	{
		CHECK(base85::decode_as_str("").empty());
		CHECK(not base85::decode("v~"sv).has_value());
		CHECK(not base85::decode("v"sv).has_value());
		CHECK(not base85::decode("BOu!rD]j7BEbo7~"sv).has_value());
		CHECK(not base85::decode("vp~"sv).has_value());

		// invalid tail length
		CHECK(not base85::decode("0"sv).has_value());

	}
}
