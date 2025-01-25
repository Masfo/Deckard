
#include <catch2/catch_test_macros.hpp>

import deckard;
import deckard.utils.hash;
import std;

using namespace deckard;
using namespace deckard::utils;

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("SHA digests", "[sha][hash]")
{
	sha256::digest correct_abc_digest{
	  0xba78'16bf, 0x8f01'cfea, 0x4141'40de, 0x5dae'2223, 0xb003'61a3, 0x9617'7a9c, 0xb410'ff61, 0xf200'15ad};

	sha256::digest copy = correct_abc_digest;

	sha256::hasher h;
	h.update("abc"sv);
	auto abc_digest = h.finalize();

	CHECK(abc_digest == correct_abc_digest);
	CHECK(copy == abc_digest);


	sha256::digest hash256_str("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"sv);
	CHECK(correct_abc_digest == hash256_str);


	sha512::digest correct_512_digest{
	  0xcf83'e135'7eef'b8bd,
	  0xf154'2850'd66d'8007,
	  0xd620'e405'0b57'15dc,
	  0x83f4'a921'd36c'e9ce,
	  0x47d0'd13c'5d85'f2b0,
	  0xff83'18d2'877e'ec2f,
	  0x63b9'31bd'4741'7a81,
	  0xa538'327a'f927'da3e};

	sha512::digest hash512_str(
	  "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"sv);
	CHECK(correct_512_digest == hash512_str);
}

TEST_CASE("SHA256 Cryptographic Hash Function", "[sha256][sha][hash]")
{
	SECTION("empty")
	{ //
		CHECK(sha256::quickhash("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"s);
	}

	SECTION("abc")
	{
		//
		CHECK(sha256::quickhash("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"s);
	}

	SECTION("sha256(sha256(abc))")
	{
		//
		CHECK(sha256::quickhash("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") ==
				"dfe7a23fefeea519e9bbfdd1a6be94c4b2e4529dd6b7cbea83f9959c2621b13c"s);
	}

#if 0
	SECTION("'a' repeated million times")
	{
		sha256::hasher hasher;

		std::string to_hash(100, 'a');

		repeat<10000> = [&] { hasher.update(to_hash); };
		auto digest    = hasher.finalize();

		CHECK(digest.to_string() == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
	}
#endif
}

TEST_CASE("SHA512 Cryptographic Hash Function", "[sha512][sha][hash]")
{
	SECTION("empty")
	{
		//
		CHECK(
		  sha512::quickhash("") ==
		  "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"s);
	}
	SECTION("abc")
	{
		//
		CHECK(
		  sha512::quickhash("abc") ==
		  "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"s);
	}

	SECTION("sha512(sha512(abc))")
	{
		//
		CHECK(
		  sha512::quickhash("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643c"
							"e80e2a9ac94fa54ca49f") ==
		  "ee02b3dd5b2c06e4e61888d141998abac194d57692f77ae7a28d748fdf9b9f28f756d980687f7290f1306857edf3fe01f8ebf4626880d49a33e029399cb2d700"s);
	}

#if 0
	SECTION("'a' repeated million timesP", "")
	{
		sha512::hasher hasher;

		std::string to_hash(100, 'a');

		repeat<10000> = [&] { hasher.update(to_hash); };

		auto digest = hasher.finalize();

		CHECK(digest.to_string() ==
				"e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217a"
				"d8cc09b");
	}
#endif
}

TEST_CASE("siphash digests", "[siphash][hash]")
{
#ifdef _DEBUG
	// debug has static key
	CHECK("hello world"_siphash == 0xcdff'88cb'8097'b979);
#endif
}

TEST_CASE("fnv digests", "[fnv][hash]")
{

	CHECK("hello world"_hash32 == 0xd58b'3fa7);
	CHECK("hello world"_hash64 == 0x779a'65e7'023c'd2e7);
}

TEST_CASE("rapidhash digests", "[rapidhash][hash]")
{

	CHECK("hello world"_rapidhash32 == 0x6f5b'77b6);
	CHECK("hello world"_rapidhash64 == 0x5dc0'fe3e'6f5b'77b6);
}


TEST_CASE("chibihash digest", "[chibihash][hash]")
{ 
	CHECK("hello world"_chibihash64 == 0x4d6a1a71f530b808);
}
