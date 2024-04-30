
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

	TEST_CASE("sha256('ba7816b...')")
	{
		//
		CHECK_EQ(sha256::quickhash("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
				 "dfe7a23fefeea519e9bbfdd1a6be94c4b2e4529dd6b7cbea83f9959c2621b13c"s);
	}
}

TEST_SUITE("SHA512" * doctest::description("SHA512 - Cryptographic hash function"))
{
	TEST_CASE("sha512('')")
	{
		//
		CHECK_EQ(
			sha512::quickhash(""),
			"cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"s);
	}

	TEST_CASE("sha512('abc')")
	{
		//
		CHECK_EQ(
			sha512::quickhash("abc"),
			"ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"s);
	}

	TEST_CASE("sha512('ddaf35a...')")
	{
		//
		CHECK_EQ(
			sha512::quickhash("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643c"
							  "e80e2a9ac94fa54ca49f"),
			"ee02b3dd5b2c06e4e61888d141998abac194d57692f77ae7a28d748fdf9b9f28f756d980687f7290f1306857edf3fe01f8ebf4626880d49a33e029399cb2d700"s);
	}
}
