#include <catch2/catch_test_macros.hpp>

import std;
import deckard.net;

using namespace deckard::net;

TEST_CASE("ipv4", "[ip]")
{
	SECTION("ipv4 single-digit octets")
	{
		ip addr("1.2.3.4");
		CHECK(addr.is_ipv4() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "1.2.3.4");
	}

	SECTION("ipv4 multi-digit octets")
	{
		ip addr("192.168.1.100");
		CHECK(addr.is_ipv4() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "192.168.1.100");
	}

	SECTION("invalid ipv4: too few octets")
	{
		ip addr("192.168.1");
		CHECK(addr.valid() == false);
	}

	SECTION("invalid ipv4: too many octets")
	{
		ip addr("192.168.1.1.1");
		CHECK(addr.valid() == false);
	}

	SECTION("invalid ipv4: non-numeric octet")
	{
		ip addr("192.168.one.1");
		CHECK(addr.valid() == false);
	}

	SECTION("invalid ipv4: negative octet")
	{
		ip addr("192.168.-1.1");
		CHECK(addr.valid() == false);
	}


	SECTION("invalid ipv4: octet out of range")
	{
		ip addr("192.168.1.256");
		CHECK(addr.valid() == false);
	}

	SECTION("invalid ipv4: empty octet")
	{
		ip addr("192.168..1");
		CHECK(addr.valid() == false);
	}

	SECTION("invalid ipv4: leading zeros")
	{
		ip addr("01.002.0003.0004");
		CHECK(addr.valid() == false);
	}

	SECTION("ipv4 loopback")
	{
		ip addr("127.0.0.1");
		CHECK(addr.is_ipv4() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "127.0.0.1");
	}

	SECTION("ipv4 broadcast")
	{
		ip addr("255.255.255.255");
		CHECK(addr.is_ipv4() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "255.255.255.255");
	}
}

TEST_CASE("ipv6", "[ip]")
{
	SECTION("ipv6 loopback (::1)")
	{
		ip addr("::1");
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "::1");
	}
	SECTION("ipv6 all zeros (::)")
	{
		ip addr("::");
      CHECK(addr.is_ipv6() == false);
		CHECK(addr.valid() == false);
		CHECK(addr.to_string() == "<invalid IP>");
	}
	SECTION("ipv6 double-colon at start (::1234)")
	{
		ip addr("::1234");
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.to_string() == "::1234");
	}
	SECTION("ipv6 double-colon in middle (2001:db8::1)")
	{
		ip addr("2001:db8::1");
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "2001:db8::1");
	}
	SECTION("ipv6 double-colon at end (2001:db8::)")
	{
		ip addr("2001:db8::");
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "2001:db8::");
	}
	SECTION("ipv6 no compression needed")
	{
		ip addr("1:2:3:4:5:6:7:8");
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "1:2:3:4:5:6:7:8");
	}
	SECTION("ipv6 single zero group is not compressed")
	{
		ip addr("1:2:3:0:5:6:7:8");
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "1:2:3:0:5:6:7:8");
	}
}

TEST_CASE("ip version", "[ip]")
{
	SECTION("ipv4 version is 4")
	{
		CHECK(ip("1.2.3.4").version() == 4);
	}

	SECTION("ipv6 version is 6")
	{
		CHECK(ip("::1").version() == 6);
	}

	SECTION("invalid ip version is -1")
	{
		CHECK(ip{}.version() == -1);
		CHECK(ip("").version() == -1);
	}
}

TEST_CASE("ip equality", "[ip]")
{
	SECTION("same ipv4 are equal")
	{
		CHECK(ip("1.2.3.4") == ip("1.2.3.4"));
	}

	SECTION("different ipv4 are not equal")
	{
		CHECK_FALSE(ip("1.2.3.4") == ip("5.6.7.8"));
	}

	SECTION("same ipv6 are equal")
	{
		CHECK(ip("::1") == ip("::1"));
	}

	SECTION("ipv4 and ipv6 are not equal")
	{
		CHECK_FALSE(ip("1.2.3.4") == ip("::1"));
	}

	SECTION("two invalid addresses are equal")
	{
		CHECK(ip{} == ip{});
	}
}

TEST_CASE("ip edge cases", "[ip]")
{
	SECTION("empty string is invalid")
	{
		ip addr("");
		CHECK(addr.valid() == false);
		CHECK(addr.to_string() == "<invalid IP>");
	}

	SECTION("0.0.0.0 is invalid")
	{
		ip addr("0.0.0.0");
		CHECK(addr.valid() == false);
	}

	SECTION("default constructed to_string returns placeholder")
	{
		CHECK(ip{}.to_string() == "<invalid IP>");
	}

	SECTION("ipv4-mapped ipv6 (::ffff:1.2.3.4) is classified as ipv4")
	{
		ip addr("::ffff:1.2.3.4");
		CHECK(addr.valid() == true);
		CHECK(addr.is_ipv4() == true);
		CHECK(addr.is_ipv6() == false);
		CHECK(addr.to_string() == "1.2.3.4");
	}

	SECTION("ipv6 full-length address is compressed")
	{
		ip addr("2001:0000:0000:0000:0000:0000:0000:0001");
		CHECK(addr.valid() == true);
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.to_string() == "2001::1");
	}

	SECTION("ipv6 longest zero run is chosen for compression")
	{
		// 1:0:0:0:0:2:0:3 — first run (len=4) wins over last run (len=1)
		ip addr("1:0:0:0:0:2:0:3");
		CHECK(addr.valid() == true);
		CHECK(addr.to_string() == "1::2:0:3");
	}

	SECTION("default constructed is not valid")
	{
		ip addr;
		CHECK(addr.valid() == false);
		CHECK(addr.is_ipv4() == false);
		CHECK(addr.is_ipv6() == false);
	}

	SECTION("ipv6 with embedded ipv4 tail")
	{
		ip addr("::192.168.1.1");
		CHECK(addr.valid() == true);
		CHECK(addr.is_ipv4() == false);
		CHECK(addr.is_ipv6() == true);
		CHECK(addr.to_string() == "::c0a8:101");
	}

	SECTION("ipv4 is not ipv6")
	{
		ip addr("10.0.0.1");
		CHECK(addr.is_ipv4() == true);
		CHECK(addr.is_ipv6() == false);
	}

	SECTION("ipv6 is not ipv4")
	{
		ip addr("::1");
		CHECK(addr.is_ipv4() == false);
		CHECK(addr.is_ipv6() == true);
	}
}