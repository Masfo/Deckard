#include <catch2/catch_test_macros.hpp>


import std;
import deckard.bigint;
import deckard.debug;

TEST_CASE("bigint", "[bigint]")
{
	using namespace deckard;

	SECTION("c-tor (integer)")
	{
		bigint def;
		CHECK(def.to_string() == "0");
		CHECK(def.to_integer() == 0);
		CHECK(def.signum() == Sign::zero);
		CHECK(def.count() == 1);

		bigint def_str = "1234567890";
		CHECK(def_str.to_string() == "1234567890");
		CHECK(def_str.to_integer() == 1'234'567'890);


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

	SECTION("c-tor hex string")
	{
		bigint a("1311768467294899695");
		CHECK(a.to_string() == "1311768467294899695");
		CHECK(a.to_string(16) == "1234567890abcdef");

		bigint b("0x1234567890ABCDEF");

		CHECK(b.to_string() == "1311768467294899695");
		CHECK(b.to_string(16) == "1234567890abcdef");

		-b;
		CHECK(b.signum() == Sign::negative);
		CHECK(b.to_string() == "-1311768467294899695");
		CHECK(b.to_string(16) == "-1234567890abcdef");

		bigint c("1234567890123456789012345678901234567890");
		CHECK(c.to_string() == "1234567890123456789012345678901234567890");

		c = "0x3a0c92075c0dbf3b8acbc5f96ce3f0ad2";
		CHECK(c.to_string() == "1234567890123456789012345678901234567890");

		c = "016406222016560155763561262742771331617605322";
		CHECK(c.to_string() == "1234567890123456789012345678901234567890");
	}

	SECTION("to_integer")
	{
		bigint a(0);
		CHECK(a.to_integer<i32>().value() == 0);
		CHECK(a.to_integer<u32>().value() == 0);


		a = "1234567890123456789012345678901234567890";
		CHECK(a.to_integer<i32>().has_value() == false);
		CHECK(a.to_integer<u32>().has_value() == false);

		auto check = [](const auto& a)
		{
			bigint b(a);

			auto res = b.to_integer<std::remove_cvref_t<decltype(a)>>();
			CHECK(res.has_value() == true);

			if (res)
			{
				CHECK(res.value() == a);
			}
		};
		check(limits::min<i64>);
		check(limits::min<i32>);
		check(limits::min<i16>);
		check(limits::min<i8>);

		check(0);
		check(limits::max<u64>);
		check(limits::max<i32>);
		check(limits::max<i16>);
		check(limits::max<i8>);
	}

	SECTION("compares")
	{
		bigint a(1'234'567'890);
		bigint b(1234);

		CHECK((a < b) == false);
		CHECK((a <= b) == false);

		CHECK((a > b) == true);
		CHECK((a >= b) == true);

		CHECK((a >= a) == true);
		CHECK((a <= a) == true);

		CHECK((a == a) == true);
		CHECK((a != b) == true);

		a = "548136543546186135844635161";
		b = "78123165165132168546";

		CHECK((a < b) == false);
		CHECK((a <= b) == false);

		CHECK((a > b) == true);
		CHECK((a >= b) == true);

		CHECK((a >= a) == true);
		CHECK((a <= a) == true);

		CHECK((a == a) == true);
		CHECK((a != b) == true);


		a = "-548136543546186135844635161";
		b = "78123165165132168546";
		CHECK((a < b) == true);
		CHECK((a <= b) == true);

		CHECK((a > b) == false);
		CHECK((a >= b) == false);

		CHECK((a >= a) == true);
		CHECK((a <= a) == true);

		CHECK((a == a) == true);
		CHECK((a != b) == true);

		a = "-548136543546186135844635161";
		b = "-78123165165132168546";
		CHECK((a < b) == true);
		CHECK((a <= b) == true);

		CHECK((a > b) == false);
		CHECK((a >= b) == false);

		CHECK((a >= a) == true);
		CHECK((a <= a) == true);

		CHECK((a == a) == true);
		CHECK((a != b) == true);
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
		CHECK(a == bigint(128) * (1 << 1));

		a <<= 4;
		CHECK(a.to_string() == "4096");
		CHECK(a.to_integer() == 4096);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 4);
		CHECK(a == bigint(128) * (1 << 5));

		a <<= 12;
		CHECK(a.to_string() == "16777216");
		CHECK(a.to_integer() == 16'777'216);
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 8);
		CHECK(a == bigint(128) * (1 << 17));

		a <<= 1;
		CHECK(a.to_string() == "33554432");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 8);
		CHECK(a == bigint(128) * (1 << 18));


		a <<= 1;
		CHECK(a.to_string() == "67108864");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 8);
		CHECK(a == bigint(128) * (1 << 19));

		a <<= 7;
		CHECK(a.to_string() == "8589934592");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 10);
		CHECK(a == bigint(128) * (1 << 26));


		a <<= 7;
		CHECK(a.to_string() == "1099511627776");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 13);
		CHECK(a == bigint(128) * (1ull << 33ull));


		a <<= 7;
		CHECK(a.to_string() == "140737488355328");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 15);
		CHECK(a == bigint(128) * (1ull << 40ull));


		a <<= 14;
		CHECK(a.to_string() == "2305843009213693952");
		CHECK(a.signum() == Sign::positive);
		CHECK(a.count() == 19);
		CHECK(a == bigint(128) * (1ull << 54ull));


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

		e = 10 - e;
		CHECK(e.to_string() == "11");
		CHECK(e.signum() == Sign::positive);
		CHECK(e.count() == 2);


		e = 1000;
		e = e - (-10);
		CHECK(e.to_string() == "1010");
		CHECK(e.signum() == Sign::positive);
		CHECK(e.count() == 4);

		constexpr int aa = 88000;
		constexpr int bb = -3000;
		constexpr int cc = aa - bb;
		bigint        f  = bigint(aa);
		f                = f - bigint(bb);

		CHECK(f.to_string() == "91000");
		CHECK(f.to_integer() == cc);
		CHECK(f.signum() == Sign::positive);
		CHECK(f.count() == 5);
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

		c = 10 + c;
		CHECK(c.to_string() == "11");
		CHECK(c.signum() == Sign::positive);
		CHECK(c.count() == 2);
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


		bigint rsa250_double = p * 2;
		CHECK(
		  rsa250_double.to_string() ==
		  "128270578954143160557580380341154778169650029485886894416233719264049064689260477247197505336695417475323851171389279597706734");


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

		// Mersenne Prime factor, 131
		bigint p131("10350794431055162386718619237468234569");
		bigint factor = p131 * 263;
		CHECK(factor.to_string() == "2722258935367507707706996859454145691647");


		auto pb = 10 * p131;
		CHECK(pb.to_string() == "103507944310551623867186192374682345690");


		//
		bigint big1 = 1'234'567'890;
		bigint big2("9876543210123456789098765432101234567890");
		bigint big3 = big1 * big2 * 123'456;
		CHECK(big3.to_string() == "1505331490682966620443288524512589666204282352096057600");
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

		bigint p(
		  "64135289477071580278790190170577389084825014742943447208116859632024532344630238623598752668347708737661925585694639798853367");
		p /= 2;

		CHECK(
		  p.to_string() ==
		  "32067644738535790139395095085288694542412507371471723604058429816012266172315119311799376334173854368830962792847319899426683");
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


		bigint aa = "1233456789123456789";
		bigint bb = "16";
		CHECK((aa % bb).to_string() == "5");
	}

	SECTION("pow")
	{
		const bigint a(10), b(2);

		bigint c = pow(a, b);
		CHECK(c.to_string() == "100");
		CHECK(c.to_integer() == 100);
		CHECK(c.signum() == Sign::positive);

		c = pow(a, b);
		CHECK(c.to_string() == "100");
		CHECK(c.to_integer() == 100);
		CHECK(c.signum() == Sign::positive);

		c = pow(a, 52);
		CHECK(c.to_string() == "10000000000000000000000000000000000000000000000000000");
		CHECK(c.signum() == Sign::positive);

		c = pow(bigint{78'945'165'163}, 34);
		CHECK(
		  c.to_string() ==
		  "32290017532639513388688936255873485353146117632851832154852216685914632839589735559565983569901467745559944875449797671314352532"
		  "12517775804153942538904727692315246151345450218113699311007713069004619330588346132723321347383672687243628246035531829091587859"
		  "6939195157955905767130134752581191460596795355261275651729819196521697845745019711260824560639905752076567808095289");
		CHECK(c.signum() == Sign::positive);
		CHECK(c.count() == 371);


		auto aaa = pow(bigint(2), 1024);
		CHECK(aaa.to_string() ==
			  "17976931348623159077293051907890247336179769789423065727343008115773267580550096313270847732240753602112011"
			  "38798713933576587897688144166224928474306394741243777678934248654852763022196012460941194530829520850057688"
			  "38150682342462881473913110540827237163350510684586298239947245938479716304835356329624224137216");
		CHECK(aaa.count() == 309);


		aaa = pow(bigint(-15), 2);
		CHECK(aaa.to_string() == "-225");
		CHECK(aaa.to_integer() == -225);
		CHECK(aaa.signum() == Sign::negative);

		aaa = pow(bigint(-455), 9);
		CHECK(aaa.to_string() == "-835800390878492990234375");
		CHECK(aaa.signum() == Sign::negative);

		aaa = pow(bigint(-455), 89);
		CHECK(aaa.to_string() ==
			  "-365606831829184082087636830292783886697273542574811330940045764208963408401899618528182596556243961192043721542111882547153"
			  "579244270460323674958722095560806595915245100899962245232488846537273106084009466343331951065920293331146240234375");
		CHECK(aaa.signum() == Sign::negative);
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


		result = sqrt(pow(g, 2));
		CHECK(g == result);
		CHECK(result.to_string() == "68553216584651356156");
	}

	SECTION("abs")
	{
		bigint a("1234567890123456789012345678901234567890");
		bigint b("-9876543210987654321098765432109876543210");

		CHECK(abs(a).to_string() == "1234567890123456789012345678901234567890");
		a = a.abs();
		CHECK(a.signum() == Sign::positive);

		b = abs(b);
		CHECK(b.to_string() == "9876543210987654321098765432109876543210");
	}
	SECTION("gcd")
	{
		bigint a("1234567890123456789012345678901234567890");
		bigint b("9876543210987654321098765432109876543210");

		bigint result = gcd(a, b);
		CHECK(result.to_string() == "90000000009000000000900000000090");

		a = pow(bigint(2), 1000);
		b = pow(bigint(2), 500);

		CHECK(a.to_string() ==
			  "10715086071862673209484250490600018105614048117055336074437503883703510511249361224931983788156958581275946"
			  "72917553146825187145285692314043598457757469857480393456777482423098542107460506237114187795418215304647498"
			  "3581941267398767559165543946077062914571196477686542167660429831652624386837205668069376");

		CHECK(b.to_string() == "32733906078961418700131896968275991522166420460430647894832913680961337964046745548832700923259041571508866"
							   "84127560071009217256545885393053328527589376");

		result = gcd(a, b);
		CHECK(result.to_string() == "327339060789614187001318969682759915221664204604306478948329136809613379640467455488327009232590415715"
									"0886684127560071009217256545885393053328527589376");
	}

	SECTION("lcm")
	{
		bigint a(12);
		bigint b(18);
		bigint result = lcm(a, b);

		CHECK(result.to_integer() == 36);


		a      = 123'456;
		b      = 789'012;
		result = lcm(a, b);
		CHECK(result.to_string() == "8117355456");


		a      = "123456123456789";
		b      = "7890123456789012";
		result = lcm(a, b);
		CHECK(result.to_string() == "324694685190217018074557334156");

		a      = "123456123456789324694685190217018074557334156";
		b      = "324694685190217018074557334156";
		result = lcm(a, b);
		CHECK(result.to_string() == "81173671297554468008735326376926582645357408844016206091156");
	}

	SECTION("random")
	{
		constexpr u32 TESTS = 50;

		std::random_device                 rd;
		std::mt19937                       gen(rd());
		std::uniform_int_distribution<i64> r(limits::min<i32> /2, limits::max<i32> / 2);

		u32 N = 100 * TESTS;

		while (--N)
		{
			i64    a = r(gen);
			i64    b = r(gen);
			bigint aa(a);
			bigint bb(b);

			CHECK(bigint(aa + bb).to_integer() == a + b);
			CHECK(bigint(aa - bb).to_integer() == a - b);
			CHECK(bigint(aa * bb).to_integer() == a * b);
			CHECK(bigint(aa / bb).to_integer() == a / b);
		}

		std::uniform_int_distribution<i32> negatives(limits::min<i32>/2, limits::max<i32>/2);
		std::uniform_int_distribution<i32> positives(limits::min<i32> /2, limits::max<i32> / 2);

		N = TESTS;
		while (--N)
		{
			bigint start = negatives(gen);
			bigint end   = positives(gen);

			if (end < start)
				std::swap(start, end);

			auto rnd1 = random_range(start, end);

			CHECK(rnd1 >= start);
			CHECK(rnd1 < end);
		}


		N = TESTS;
		while (--N)
		{
			bigint start = random_range("-1000000000000", "1000000000000");
			bigint end   = random_range("-1000000000000", "1000000000000");

			if (end < start)
				std::swap(start, end);

			auto rnd2 = random_range(start, end);

			CHECK(rnd2 >= start);
			CHECK(rnd2 < end);
		}
	}

	SECTION("bitwise and")
	{
		int b1 = 0b1001;
		int b2 = 0b1100;
		int b3 = b1 & b2;

		CHECK(b3 == (b1 & b2));

		bigint a(0b1001);
		bigint b(0b1100);

		bigint c = a & b;
		CHECK(c.to_integer() == 8);
		CHECK(c.to_string(2) == "1000");


		b1 = 0xFFFF'FFFF;
		b2 = 0x0'0000'000F;
		b3 = b1 & b2;

		a = "0xFFFF";
		b = "0x003F";
		c = a & b;

		auto astr = a.to_string(2);
		auto bstr = b.to_string(2);
		auto cstr = c.to_string(2);

		CHECK(c.to_string(16) == "3f");
		CHECK(1 == 1);

		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = "0x00000000000000000000000000000000";
		c = a & b;
		CHECK(c.to_string(16) == "0");


		a = "0x00000000000000000000000000000000";
		b = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		c = a & b;
		CHECK(c.to_string(16) == "0");

		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		c = a & b;
		CHECK(c.to_string(16) == "ffffffffffffffffffffffffffffffff");


		a = "84309483902093439083849034732890487320498374";
		b = "47895478932092390843789438902390832487394390";
		c = a & b;
		CHECK(c.to_string() == "45107841234594946144076549931291317083586630");

		a = "0b110011";
		b = "0b001100";
		CHECK((a & b).to_string(2) == "0");

	}

	SECTION("bitwise or")
	{
		bigint a(0b1001);
		bigint b(0b1001);
		bigint c = a ^ b;
		CHECK(c.to_integer() == 0);

		a = "0b111100001111";
		b = "0b101010101010";
		c = a | b;
		CHECK(c.to_string(2) == "111110101111");

		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = "0x00000000000000000000000000000000";
		c = a | b;
		CHECK(c.to_string(16) == "ffffffffffffffffffffffffffffffff");

		a = "0x00000000000000000000000000000000";
		b = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		c = a | b;
		CHECK(c.to_string(16) == "ffffffffffffffffffffffffffffffff");


		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = "0xAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
		c = a | b;
		CHECK(c.to_string(16) == "ffffffffffffffffffffffffffffffff");


		a = "84309483902093439083849034732890487320498374";
		b = "47895478932092390843789438902390832487394390";
		c = a | b;
		CHECK(c.to_string() == "87097121599590883783561923703990002724306134");

		a = "0b110011";
		b = "0b001100";
		CHECK((a | b).to_string(2) == "111111");
	}

	SECTION("bitwise not")
	{

		bigint a(0b1001);

		bigint b = ~a;
		CHECK(b.to_integer() == 6);
		CHECK(b.to_string(2) == "110");

		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = ~a;
		CHECK(b.to_string(16) == "0");
		CHECK(b.to_integer() == 0);
		CHECK(b.signum() == Sign::zero);

		a = "0b10001110000110001011010010010110010100100110100110111001110011100111011100101101110000001001001011011110101011";
		b = ~a;
		CHECK(b.to_string(2) ==
			  "1110001111001110100101101101001101011011001011001000110001100011000100011010010001111110110110100100001010100");
	}


	SECTION("bitwise xor")
	{
		bigint a(0b1001);
		bigint b(0b1001);
		bigint c = a ^ b;
		CHECK(c.to_integer() == 0);

		a = "0b111100001111";
		b = "0b101010101010";
		c = a ^ b;
		CHECK(c.to_integer() == 1445);
		CHECK(c.to_string(2) == "10110100101");


		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = "0x0";
		c = a ^ b;
		CHECK(c.to_string(16) == "ffffffffffffffffffffffffffffffff");

		a = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		b = "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
		c = a ^ b;
		CHECK(c.to_string(16) == "0");
		CHECK(c.to_integer() == 0);
		CHECK(c.signum() == Sign::zero);


		a = "84309483902093439083849034732890487320498374";
		b = "47895478932092390843789438902390832487394390";
		c = a ^ b;

		CHECK(a.to_string(2) == "1111000111110100110100111000011100010010011001110000001100010010101000101111010001000110001011010000110001"
								"1001100110111010111010000101000011000110");
		CHECK(b.to_string(2) == "1000100101110100000010100110001011100110111100010011011010001000010110000001100100111011001010010101111111"
								"0010011101111110001100101110110001010110");
		CHECK(c.to_string(2) == "1111000100000001101100111100101111101001001011000110101100110101111101011101101011111010000010001010011101"
								"011111011000100110110101011110010010000");

		CHECK(c.to_string() == "41989280364995937639485373772698685640719504");
	}

	SECTION("popcount")
	{ 
		u64 a = 0xFFFF'FFFF'FFFF'FFFF;
		CHECK(std::popcount(a) == 64);

		a = 0xFFFF'0000'0000'FFFF;
		CHECK(std::popcount(a) == 32);

		a = 0xF00F'0000'0000'F00F;
		CHECK(std::popcount(a) == 16);

		bigint b(0xFFFF'FFFF'FFFF'FFFF);
		CHECK(b.popcount() == 64);

		b = "739847398473429847349083840394830948309483094748940398730978429347320498732049872";
		CHECK(b == "739847398473429847349083840394830948309483094748940398730978429347320498732049872");
		CHECK(b.popcount() == 139);

		auto c = ~b;
		CHECK(b == "739847398473429847349083840394830948309483094748940398730978429347320498732049872");
		CHECK(c == "208721396558664425560809668796340392824504619631987101880258098845503859278305839");
		CHECK(c.popcount() == 130);


	}

	SECTION("umap")
	{
		std::unordered_map<int, bigint> map;

		map[0] = "123456789";

		CHECK(map[0].to_integer() == 123'456'789);
	}

	SECTION("formatter")
	{

		bigint a(1'234'567'890);
		CHECK(a.to_string(10) == "1234567890");
		CHECK(a.to_string(16) == "499602d2");
		CHECK(std::format("{}", a) == "1234567890");
		CHECK(std::format("{:x}", a) == "499602d2");
		a = "81173671297554468008735326376926582645357408844016206091156";
		CHECK(a.to_string(10) == "81173671297554468008735326376926582645357408844016206091156");
		CHECK(a.to_string(16) == "cee84ac0860d1d3f5a8ab7f9ad365fbf1a4c7afcf00161394");


		CHECK(std::format("{:b2}", a) ==
			  "1100111011101000010010101100000010000110000011010001110100111111010110101000101010110111111110011010110100110110010111111011"
			  "111100011010010011000111101011111100111100000000000101100001001110010100");
		CHECK(std::format("{:b36}", a) == "l701f2az4zj11momniss8jl4co03sao1cpvr10");
		CHECK(a.to_string(16, true) == "CEE84AC0860D1D3F5A8AB7F9AD365FBF1A4C7AFCF00161394");
		CHECK(std::format("{:x}", a) == "cee84ac0860d1d3f5a8ab7f9ad365fbf1a4c7afcf00161394");
	}
}
