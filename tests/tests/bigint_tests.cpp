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

	SECTION("to_integer")
	{
		bigint a(0);
		CHECK(a.to_integer<i32>().value() == 0);
		CHECK(a.to_integer<u32>().value() == 0);


		a = "1234567890123456789012345678901234567890";
		CHECK(a.to_integer<i32>().has_value() == false);
		CHECK(a.to_integer<u32>().has_value() == false);

		auto check = [](const auto& a, bool correct)
		{
			bigint b(a);

			auto res = b.to_integer<std::remove_cvref_t<decltype(a)>>();
			CHECK(res.has_value() == correct);

			if (res)
			{
				CHECK(res.value() == a);
			}
		};
		check(limits::min<i64>, true);
		check(limits::min<i32>, true);
		check(limits::min<i16>, true);
		check(limits::min<i8>, true);

		check(0, true);
		check(limits::max<u64>, true);
		check(limits::max<i32>, true);
		check(limits::max<i16>, true);
		check(limits::max<i8>, true);
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

	SECTION("shift left")
	{
		bigint a(128);

		CHECK(a.to_string() == "128");
		CHECK(a.to_integer() == 128);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 3);

		a <<= 1;

		CHECK(a.to_string() == "256");
		CHECK(a.to_integer() == 256);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 3);

		a <<= 4;
		CHECK(a.to_string() == "4096");
		CHECK(a.to_integer() == 4096);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 4);

		a <<= 12;
		CHECK(a.to_string() == "16777216");
		CHECK(a.to_integer() == 16'777'216);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 8);

		a <<= 1;
		CHECK(a.to_string() == "33554432");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 8);

		a <<= 1;
		CHECK(a.to_string() == "67108864");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 8);

		a <<= 7;
		CHECK(a.to_string() == "8589934592");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 10);


		a <<= 7;
		CHECK(a.to_string() == "1099511627776");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 13);

		a <<= 7;
		CHECK(a.to_string() == "140737488355328");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 15);

		a <<= 14;
		CHECK(a.to_string() == "2305843009213693952");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 19);
#if 0
		a <<= 28;
		CHECK(a.to_string() == "618970019642690137449562112");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 27);

		a <<= 32;
		CHECK(a.to_string() == "2658455991569831745807614120560689152");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 37);

		a <<= 33;
		CHECK(a.to_string() == "22835963083295358096932575511191922182123945984");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 47);

		a <<= 50;
		CHECK(a.to_string() == "25711008708143844408671393477458601640355247900524685364822016");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 62);
#endif
	}

	SECTION("shift right")
	{
		bigint a(128);

		CHECK(a.to_string() == "128");
		CHECK(a.to_integer() == 128);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 3);

		a >>= 1;

		CHECK(a.to_string() == "64");
		CHECK(a.to_integer() == 64);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 2);

		a >>= 1;

		CHECK(a.to_string() == "32");
		CHECK(a.to_integer() == 32);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 2);

		a = "2305843009213693952";
		a >>= 16;
		CHECK(a.to_string() == "35184372088832");
		CHECK(a.to_integer<u64>() == 35'184'372'088'832ull);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 14);
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
		CHECK(i == -1);


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
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 40);

		a = a - b;
		CHECK(a.to_string() == "-1");
		CHECK(a.signum() == Sign::negative);
		CHECK(a.count() == 1);

		a = a - b;
		CHECK(a.to_string() == "-1234567890123456789012345678901234567891");
		CHECK(a.signum() == Sign::negative);
		CHECK(a.count() == 40);
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


	SECTION("multiply")
	{
		//
		bigint a(10), b(33);
		bigint c = a * b;
		CHECK(c.signum() == Sign::positive);
		CHECK(c.to_integer() == 330);


		bigint zero = a * 0;
		CHECK(zero.signum() == Sign::zero);
		CHECK(zero.to_integer() == 0);

		bigint a_neg = a * -1;
		CHECK(a_neg.signum() == Sign::negative);
		CHECK(a_neg.to_integer() == -10);

		bigint p(
		  "64135289477071580278790190170577389084825014742943447208116859632024532344630238623598752668347708737661925585694639798853367");

		bigint q(
		  "33372027594978156556226010605355114227940760344767554666784520987023841729210037080257448673296881877565718986258036932062711");

		bigint correct(
		  "21403246502407449612644230728393335630086147151447550177977549208814180234471401366433455190958046796109928518724709145876873962"
		  "61921557363047454770520805119056493106687691590019759405693457452230589325976697471681738069364894699871578494975937497937");


		bigint rsa250 = p * q;
		CHECK(rsa250 == correct);


		bigint rsa_a_neg = p * -1;
		CHECK(rsa_a_neg.signum() == Sign::negative);
		CHECK(
		  rsa_a_neg.to_string() ==
		  "-64135289477071580278790190170577389084825014742943447208116859632024532344630238623598752668347708737661925585694639798853367");

		rsa_a_neg *= -1;
		CHECK(rsa_a_neg.signum() == Sign::positive);
		CHECK(
		  rsa_a_neg.to_string() ==
		  "64135289477071580278790190170577389084825014742943447208116859632024532344630238623598752668347708737661925585694639798853367");
	}

	SECTION("divide")
	{
		bigint a(42), b(7);

		bigint c = a / b;
		CHECK(c.to_string() == "6");
		CHECK(c.to_integer() == 6);
		CHECK(c.signum() == Sign::positive);


		bigint aa("5499082447685365340");
		bigint bb("-4059411");
		bigint cc(aa / bb);

		CHECK(cc.to_string() == "-1354650329243");
		CHECK(cc.signum() == Sign::negative);
		CHECK(cc.count() == 13);

		cc /= bigint{"-456879"};
		CHECK(cc.to_string() == "2965008");
		CHECK(cc.to_integer() == 2'965'008);
		CHECK(cc.signum() == Sign::positive);
		CHECK(cc.count() == 7);
	}

	SECTION("modulus")
	{
		const bigint a(44), b(42);

		bigint c = a % b;
		CHECK(c.to_string() == "2");
		CHECK(c.to_integer() == 2);
		CHECK(c.signum() == Sign::positive);

		CHECK(-44 % 42 == -2);
		CHECK((-44 % 42 + 42) % 42 == 40);
		c = -a % b;
		CHECK(c.to_string() == "-2");
		CHECK(c.to_integer() == -44 % 42);
		CHECK(c.signum() == Sign::negative);

		bigint d = (-a % b + b) % b;
		CHECK(d.to_string() == "40");
		CHECK(d.to_integer() == (-44 % 42 + 42) % 42);
		CHECK(d.signum() == Sign::positive);
	}

	SECTION("pow")
	{
		const bigint a(10), b(2);

		bigint c = a ^ b;
		CHECK(c.to_string() == "100");
		CHECK(c.to_integer() == 100);
		CHECK(c.signum() == Sign::positive);

		 c = pow(a,b);
		CHECK(c.to_string() == "100");
		CHECK(c.to_integer() == 100);
		CHECK(c.signum() == Sign::positive);

		c = a ^ 52;
		CHECK(c.to_string() == "10000000000000000000000000000000000000000000000000000");
		CHECK(c.signum() == Sign::positive);

		c = bigint{78'945'165'163} ^ 34;
		CHECK(
		  c.to_string() ==
		  "32290017532639513388688936255873485353146117632851832154852216685914632839589735559565983569901467745559944875449797671314352532"
		  "12517775804153942538904727692315246151345450218113699311007713069004619330588346132723321347383672687243628246035531829091587859"
		  "6939195157955905767130134752581191460596795355261275651729819196521697845745019711260824560639905752076567808095289");
		CHECK(c.signum() == Sign::positive);
	}


	SECTION("sqrt")
	{
		bigint a("49");
		bigint result = a.sqrt();
		CHECK(result.to_string() == "7");

		bigint b("0");
		result = b.sqrt();
		CHECK(result.to_string() == "0");

		bigint c("1");
		result = c.sqrt();
		CHECK(result.to_string() == "1");

		bigint d("4");
		result = d.sqrt();
		CHECK(result.to_string() == "2");

		bigint e("9");
		result = e.sqrt();
		CHECK(result.to_string() == "3");

		bigint f("16");
		result = f.sqrt();
		CHECK(result.to_string() == "4");

		const bigint g("68553216584651356156");
		result = g * g;
		CHECK(result.to_string() == "4699543504102117748327603647049959096336");
		CHECK(g == sqrt(result));


		result = sqrt(g^2);
		CHECK(g == result);
		CHECK(result.to_string() == "68553216584651356156");

	}
	SECTION("umap")
	{
		std::unordered_map<int, bigint> map;

		map[0] = "123456789";

		CHECK(map[0].to_integer() == 123'456'789);
	}
}
