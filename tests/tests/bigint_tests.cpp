#include <catch2/catch_test_macros.hpp>


import std;
import deckard.bigint;

TEST_CASE("bigint", "[bigint]")
{
	using namespace deckard;

	SECTION("c-tor (integer)")
	{
		bigint a(1'234'567'890);
		CHECK(a.to_string() == "1234567890");
		CHECK(std::format("{}", a) == "1234567890");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 10);

		bigint b(-1'234'567'890);
		CHECK(b.to_string() == "-1234567890");
		CHECK(std::format("{}", b) == "-1234567890");
		CHECK(b.signum() == Sign::negative);
		CHECK(b.count() == 10);


		bigint c(-1234);
		CHECK(c.to_string() == "-1234");
		CHECK(std::format("{}", c) == "-1234");
		CHECK(c.signum() == Sign::negative);
		CHECK(c.count() == 4);

		bigint d(0);
		CHECK(d.to_string() == "0");
		CHECK(std::format("{}", d) == "0");
		CHECK(d.signum() == Sign::zero);
		CHECK(d.count() == 1);


		bigint e(-0);
		CHECK(e.to_string() == "0");
		CHECK(std::format("{}", e) == "0");
		CHECK(e.signum() == Sign::zero);
		CHECK(e.count() == 1);
	}

	SECTION("c-tor (string)")
	{
		bigint a("1234567890123456789012345678901234567890");
		CHECK(a.to_string() == "1234567890123456789012345678901234567890");
		CHECK(std::format("{}", a) == "1234567890123456789012345678901234567890");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 40);

		bigint b("-1234567890123456789012345678901234567890");
		CHECK(b.to_string() == "-1234567890123456789012345678901234567890");
		CHECK(std::format("{}", b) == "-1234567890123456789012345678901234567890");
		CHECK(b.signum() == Sign::negative);
		CHECK(b.count() == 40);

		bigint c("0");
		CHECK(c.to_string() == "0");
		CHECK(std::format("{}", c) == "0");
		CHECK(c.signum() == Sign::zero);
		CHECK(c.count() == 1);
	}

	// RSA-250 =
	// 2140324650240744961264423072839333563008614715144755017797754920881418023447140136643345519095804679610992851872470914587687396261921557363047454770520805119056493106687691590019759405693457452230589325976697471681738069364894699871578494975937497937
	//
	// RSA-250 =
	// 64135289477071580278790190170577389084825014742943447208116859632024532344630238623598752668347708737661925585694639798853367
	// *
	// 33372027594978156556226010605355114227940760344767554666784520987023841729210037080257448673296881877565718986258036932062711
}
