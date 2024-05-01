
#include <catch2/catch_test_macros.hpp>

import deckard;
import std;

using namespace deckard;
using namespace std::string_literals;

TEST_CASE("SHA256 Cryptographic Hash Function", "[sha256]")
{
	SECTION("empty")
	{
		//
		REQUIRE(sha256::quickhash("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"s);
	}

	SECTION("abc")
	{
		//
		REQUIRE(sha256::quickhash("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"s);
	}

	SECTION("sha256(sha256(abc))")
	{
		//
		REQUIRE(sha256::quickhash("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") ==
				"dfe7a23fefeea519e9bbfdd1a6be94c4b2e4529dd6b7cbea83f9959c2621b13c"s);
	}

	SECTION("'a' repeated million times")
	{

		sha256::hasher hasher;


		int counter = 10'000;
		while (counter--)
			hasher.update("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");


		auto digest = hasher.finalize();

		REQUIRE(digest.to_string() == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
	}
}

TEST_CASE("SHA512 Cryptographic Hash Function", "[sha512]")
{
	SECTION("empty")
	{
		//
		REQUIRE(
			sha512::quickhash("") ==
			"cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"s);
	}

	SECTION("abc")
	{
		//
		REQUIRE(
			sha512::quickhash("abc") ==
			"ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"s);
	}

	SECTION("sha512(sha512(abc))")
	{
		//
		REQUIRE(
			sha512::quickhash("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643c"
							  "e80e2a9ac94fa54ca49f") ==
			"ee02b3dd5b2c06e4e61888d141998abac194d57692f77ae7a28d748fdf9b9f28f756d980687f7290f1306857edf3fe01f8ebf4626880d49a33e029399cb2d700"s);
	}


	SECTION("'a' repeated million timesP", "")
	{
		sha512::hasher hasher;
		int            counter = 10'000;
		while (counter--)
			hasher.update("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

		auto digest = hasher.finalize();

		REQUIRE(digest.to_string() ==
				"e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217a"
				"d8cc09b");
	}
}
