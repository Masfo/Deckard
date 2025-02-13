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

	SECTION("invert")
	{
		bigint a("1234567890123456789012345678901234567890");
		CHECK(a.to_string() == "1234567890123456789012345678901234567890");
		CHECK(std::format("{}", a) == "1234567890123456789012345678901234567890");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 40);

		-a;

		CHECK(a.to_string() == "-1234567890123456789012345678901234567890");
		CHECK(std::format("{}", a) == "-1234567890123456789012345678901234567890");
		CHECK(a.signum() == Sign::negative);
		CHECK(a.count() == 40);

		bigint b = -a;

		CHECK(b.to_string() == "1234567890123456789012345678901234567890");
		CHECK(std::format("{}", b) == "1234567890123456789012345678901234567890");
		CHECK(b.signum() == Sign::positive);
		CHECK(b.count() == 40);

		+b;

		CHECK(b.to_string() == "1234567890123456789012345678901234567890");
		CHECK(std::format("{}", b) == "1234567890123456789012345678901234567890");
		CHECK(b.signum() == Sign::positive);
		CHECK(b.count() == 40);
	}

	SECTION("subtract (small)")
	{
		bigint a(768);
		bigint b(256);

		auto c = a - b;
		CHECK(c.to_string() == "512");
		CHECK(c.signum() == Sign::positive);
		CHECK(c.count() == 3);

		auto d = c - 512;
		CHECK(d.to_string() == "0");
		CHECK(d.signum() == Sign::zero);
		CHECK(d.count() == 1);

		auto e = d - 512;
		CHECK(e.to_string() == "-512");
		CHECK(e.signum() == Sign::negative);
		CHECK(e.count() == 3);

		int i = 1;
		i--;
		CHECK(i == 0);
		i--;
		CHECK(i==-1);


		e = 1;
		e--;
		CHECK(e.to_string() == "0");
		CHECK(e.signum() == Sign::zero);
		CHECK(e.count() == 1);

		e--;
		CHECK(e.to_string() == "-1");
		CHECK(e.signum() == Sign::negative);
		CHECK(e.count() == 1);
	}

	SECTION("sub (large)")
	{ 
		bigint a("1234567890123456789012345678901234567890");

		CHECK(a.to_string() == "1234567890123456789012345678901234567890");

		a = a - 1;
		CHECK(a.to_string() == "1234567890123456789012345678901234567889");
		
		bigint b("1234567890123456789012345678901234567890");

		a = a - b;
		CHECK(a.to_string() == "-1");
		
	}

	SECTION("add (small)") 
	{
		bigint a(-256);
		bigint b(256);

		auto c = a + b;
		CHECK(c.to_string() == "0");
		CHECK(c.signum() == Sign::zero);
		CHECK(c.count() == 1);

		c = c + 256;
		CHECK(c.to_string() == "256");
		CHECK(c.signum() == Sign::positive);
		CHECK(c.count() == 3);

		c++;
		CHECK(c.to_string() == "257");
		CHECK(c.signum() == Sign::positive);
		CHECK(c.count() == 3);

	
		int i = -1;
		i++;
		CHECK(i == 0);

		i = -1;
		i++;
		CHECK(i == 0);

		i++;
		CHECK(i == 1);

		c = -1;
		CHECK(c.to_string() == "-1");
		CHECK(c.signum() == Sign::negative);
		CHECK(c.count() == 1);

		c++;
		CHECK(c.to_string() == "0");
		CHECK(c.signum() == Sign::zero);
		CHECK(c.count() == 1);

		c++;
		CHECK(c.to_string() == "1");
		CHECK(c.signum() == Sign::positive);
		CHECK(c.count() == 1);


	}

	SECTION("add (large)")
	{ 
		bigint a("1234567890123456789012345678901234567890");

		CHECK(a.to_string() == "1234567890123456789012345678901234567890");


		a = a + a;

		CHECK(a.to_string() == "2469135780246913578024691357802469135780");
		
	}
	// RSA-250 =
	// 2140324650240744961264423072839333563008614715144755017797754920881418023447140136643345519095804679610992851872470914587687396261921557363047454770520805119056493106687691590019759405693457452230589325976697471681738069364894699871578494975937497937
	//
	// RSA-250 =
	// 64135289477071580278790190170577389084825014742943447208116859632024532344630238623598752668347708737661925585694639798853367
	// *
	// 33372027594978156556226010605355114227940760344767554666784520987023841729210037080257448673296881877565718986258036932062711
}
