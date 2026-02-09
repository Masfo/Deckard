
#include <catch2/catch_test_macros.hpp>

import deckard;
import deckard.as;
import deckard.types;
import deckard.utils.hash;
import deckard.sha;
import deckard.hmac;
import std;

using namespace deckard;
using namespace deckard::utils;

using namespace std::string_literals;
using namespace std::string_view_literals;

TEST_CASE("stringhash", "[hash]")
{

	SECTION("stringhash with multiple views")
	{
		std::string_view part1 = "The quick brown fox ";
		std::string_view part2 = "jumps over the lazy dog";

		CHECK(stringhash(part1, part2) == stringhash("The quick brown fox jumps over the lazy dog"));


		CHECK(stringhash("The quick brown fox jumps over the lazy dog"sv,
						 "The quick brown fox jumps over the lazy dog"sv,
						 "The quick brown fox jumps over the lazy dog"sv,
						 "The quick brown fox jumps over the lazy dog"sv,
						 "The quick brown fox jumps over the lazy dog"sv,
						 "The quick brown fox jumps over the lazy dog"sv) ==
			  stringhash("The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps "
						 "over the lazy dogThe quick brown fox jumps over the lazy dogThe quick brown fox jumps "
						 "over the lazy dogThe quick brown fox jumps over the lazy dog"));
	}
}

TEST_CASE("xxhasher", "[hash][xxhash]")
{
	SECTION("xxhasher")
	{
		std::string_view part1 = "The quick brown fox ";
		std::string_view part2 = "jumps over the lazy dog";

		CHECK(xxhash64(part1, part2) == xxhash64("The quick brown fox jumps over the lazy dog"));
	}
}

TEST_CASE("HMAC-SHA1 digests", "[hmac][sha1][hash]")
{
	SECTION("Jefe key")
	{
		auto digest = hmac::sha1::quickhash("Jefe"sv, "what do ya want for nothing?"sv);
		CHECK(digest == "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79"s);
	}

	SECTION("0xaa repeated 80 times - Data 54")
	{
		std::array<u8, 80> key{};
		key.fill(0xaa);

		std::string data("Test Using Larger Than Block-Size Key - Hash Key First"sv);

		auto digest = hmac::sha1::hash(key, data);

		sha1::digest correct_digest("aa4ae5e15272d00e95705637ce8a3b55ed402112");

		CHECK(digest == correct_digest);
	}

	SECTION("0xaa repeated 80 times - Data 73")
	{
		std::array<u8, 80> key{};
		key.fill(0xaa);

		std::string data("Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data"sv);

		auto digest = hmac::sha1::hash(key, data);

		sha1::digest correct_digest("e8e99d0f45237d786d6bbaa7965c7808bbff1a91");

		CHECK(digest == correct_digest);
	}
}

TEST_CASE("HMAC-SHA256 digests", "[hmac][sha256][hash]")
{
	SECTION("Jefe key")
	{
		auto digest = hmac::sha256::quickhash("Jefe"sv, "what do ya want for nothing?"sv);
		CHECK(digest == "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"s);
	}

	SECTION("0xaa repeated 131 times - Data 54")
	{
		std::array<u8, 131> key{};
		key.fill(0xaa);

		std::string data("Test Using Larger Than Block-Size Key - Hash Key First"sv);

		auto digest = hmac::sha256::hash(key, data);

		sha256::digest correct_digest("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");

		CHECK(digest == correct_digest);
	}

	SECTION("0xaa repeated 131 times - Data 73")
	{
		std::array<u8, 131> key{};
		key.fill(0xAA);

		std::string data(
		  "This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm."sv);

		auto digest = hmac::sha256::hash(key, data);

		sha256::digest correct_digest("9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");

		CHECK(digest == correct_digest);
	}
}

TEST_CASE("HMAC-SHA512 digests", "[hmac][sha512][hash]")
{
	SECTION("Jefe key")
	{
		auto digest = hmac::sha512::quickhash("Jefe"sv, "what do ya want for nothing?"sv);
		CHECK(
		  digest ==
		  "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737"s);
	}

	SECTION("0xaa repeated 131 times - Data 54")
	{
		std::array<u8, 131> key{};
		key.fill(0xaa);

		std::string data("Test Using Larger Than Block-Size Key - Hash Key First"sv);

		auto digest = hmac::sha512::hash(key, data);

		sha512::digest correct_digest("80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f3526b56d037e05f2598bd0fd2215d6a1e5295e6"
									  "4f73f63f0aec8b915a985d786598");

		CHECK(digest == correct_digest);
	}

	SECTION("0xaa repeated 131 times - Data 73")
	{
		std::array<u8, 131> key{};
		key.fill(0xAA);

		std::string data(
		  "This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm."sv);

		auto digest = hmac::sha512::hash(key, data);

		sha512::digest correct_digest("e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944b6022cac3c4982b10d5eeb55c3e4de151346"
									  "76fb6de0446065c97440fa8c6a58");

		CHECK(digest == correct_digest);
	}
}

TEST_CASE("SHA1 digests", "[sha1][hash]")
{
	SECTION("empty")
	{
		const sha1::digest correct_abc_digest{
		  0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09};

		sha1::hasher h;
		h.update(""sv);
		auto abc_digest = h.finalize();

		CHECK(abc_digest == correct_abc_digest);
	}

	SECTION("empty from string")
	{
		const sha1::digest correct_abc_digest{"da39a3ee5e6b4b0d3255bfef95601890afd80709"};

		sha1::hasher h;
		h.update(""sv);
		auto abc_digest = h.finalize();

		CHECK(abc_digest == correct_abc_digest);
	}

	SECTION("abc")
	{
		const sha1::digest correct_abc_digest{
		  0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d};

		sha1::hasher h;
		h.update("abc"sv);
		auto abc_digest = h.finalize();

		CHECK(abc_digest == correct_abc_digest);
	}
}

TEST_CASE("SHA256 digests", "[sha][hash]")
{
	sha256::digest correct_abc_digest{"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"};

	sha256::digest copy = correct_abc_digest;

	sha256::hasher h;
	h.update("abc"sv);
	auto abc_digest = h.finalize();

	CHECK(abc_digest == correct_abc_digest);
	CHECK(copy == abc_digest);


	sha256::digest hash256_str("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"sv);
	CHECK(correct_abc_digest == hash256_str);


	sha512::digest correct_512_digest{
	  "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"};

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

TEST_CASE("SHA1/SHA256/SHA512 digest formatting", "[sha1][sha256][sha512][hash]")
{
	SECTION("sha1 digest")
	{
		const sha1::digest correct_abc_digest{"a9993e364706816aba3e25717850c26c9cd0d89d"};

		sha1::hasher h;
		h.update("abc"sv);
		auto abc_digest = h.finalize();

		CHECK(abc_digest == correct_abc_digest);
		CHECK(std::format("{}", abc_digest) == std::format("{}", correct_abc_digest));
	}

	SECTION("sha256 digest")
	{
		sha256::digest correct_abc_digest{"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"};

		sha256::hasher h;
		h.update("abc"sv);
		auto abc_digest = h.finalize();

		CHECK(abc_digest == correct_abc_digest);
		CHECK(std::format("{}", abc_digest) == std::format("{}", correct_abc_digest));
	}

	SECTION("sha512 digest")
	{
		sha512::digest correct_abc_digest{
		  "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd"
		  "454d4423643ce80e2a9ac94fa54ca49f"};

		sha512::hasher h;
		h.update("abc"sv);
		auto abc_digest = h.finalize();

		CHECK(abc_digest == correct_abc_digest);
		CHECK(std::format("{}", abc_digest) == std::format("{}", correct_abc_digest));
	}
}
