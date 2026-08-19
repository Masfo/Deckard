#include <catch2/catch_test_macros.hpp>


import std;
import deckard.commandline;

TEST_CASE("commandline", "[commandline][cli]")
{
	using namespace deckard;
	using namespace std::string_view_literals;

	SECTION("bool value")
	{
		commandline cli{"tester", "1.2.3"};

		bool flag{false};

		cli.add_flag({.short_name = "-f", .long_name = "--flag", .description = "A boolean flag"}, &flag);

		CHECK(cli.parse("-f"));

		CHECK(flag);

		flag = false;
		CHECK(cli.parse("--flag"));
		CHECK(flag);

		flag = false;
		CHECK_FALSE(cli.parse("-x"));
		CHECK_FALSE(flag);
	}

	SECTION("int value")
	{
		commandline cli{"tester", "1.2.3"};
		int         value{0};
		cli.add_option({.short_name = "-n", .long_name = "--number", .description = "An integer option"}, &value);
		CHECK(cli.parse("-n 42"));
		CHECK(value == 42);
		value = 0;
		CHECK(cli.parse("--number 100"));
		CHECK(value == 100);
		value = 0;
		CHECK_FALSE(cli.parse("-x"));
		CHECK(value == 0);
	}

	SECTION("string value")
	{
		commandline cli{"tester", "1.2.3"};
		std::string value;
		cli.add_option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
		CHECK(cli.parse("-s hello"sv));
		CHECK(value == "hello"sv);
		value.clear();
		CHECK(cli.parse("--string world"sv));
		CHECK(value == "world"sv);
		value.clear();
		CHECK_FALSE(cli.parse("-x"));
		CHECK(value.empty());

		value.clear();
		CHECK(cli.parse("-s \"quoted string\""sv));
		CHECK(value == "quoted string"sv);
	}
	{
		CHECK(1 == 1);
	}
}
