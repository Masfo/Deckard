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

		cli.flag({.short_name = "-f", .long_name = "--flag", .description = "A boolean flag"}, &flag);

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
		cli.option({.short_name = "-n", .long_name = "--number", .description = "An integer option"}, &value);
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
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
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

	SECTION("level option")
	{
		commandline cli{"tester", "1.2.3"};
		int         level{0};
		cli.level(
		  {.short_name = "-O", .long_name = "--optimize", .description = "Optimization level"}, &level, 0, 3, 2);
		CHECK(cli.parse("-O3"));
		CHECK(level == 3);
		level = 0;
		CHECK(cli.parse("--optimize2"));
		CHECK(level == 2);
		level = 0;
		CHECK(cli.parse("-O"));
		CHECK(level == 2);                      // default value
		level = 0;
		CHECK(cli.parse("--optimize"));
		CHECK(level == 2);                      // default value
		level = 0;
		CHECK(cli.parse("-O5"));                // out of range
		CHECK(level == 3);                      // clamped to max
		level = 0;
		CHECK_FALSE(cli.parse("--optimize-1")); // invalid suffix
		CHECK(level == 0);
	}

	SECTION("typical usage")
	{
		commandline cli{"tester", "1.2.3"};
		bool        verbose{false};
		int         count{0};
		std::string name;

		cli.flag({.short_name = "-v", .long_name = "--verbose", .description = "Enable verbose output"}, &verbose);
		cli.option({.short_name = "-n", .long_name = "--number", .description = "A number option"}, &count);
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &name);

		CHECK(cli.parse("-v -n 10 -s test"sv));

		CHECK(verbose);
		CHECK(count == 10);
		CHECK(name == "test"sv);
	}

	SECTION("utf8 string value")
	{
		commandline cli{"tester", "1.2.3"};
		std::string value;
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
		CHECK(cli.parse("-s \"hello 🌍 world\""sv));
		CHECK(value == "hello 🌍 world"sv);
	}

	SECTION("multiple spaces between tokens")
	{
		commandline cli{"tester", "1.2.3"};
		bool        verbose{false};
		int         count{0};
		cli.flag({.short_name = "-v", .long_name = "--verbose", .description = "Enable verbose output"}, &verbose);
		cli.option({.short_name = "-n", .long_name = "--number", .description = "A number option"}, &count);
		cli.parse("  -v  -n  42  "sv);
		CHECK(verbose);
		CHECK(count == 42);
	}

	SECTION("single quoted string")
	{
		commandline cli{"tester", "1.2.3"};
		std::string value;
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
		CHECK(cli.parse("-s 'quoted value'"sv));
		CHECK(value == "quoted value"sv);
	}

	SECTION("unterminated quoted string")
	{
		commandline cli{"tester", "1.2.3"};
		std::string value;
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
		CHECK_FALSE(cli.parse("-s \"quoted value"sv));
		CHECK(value.empty());
	}

	SECTION("utf8 unquoted value")
	{
		commandline cli{"tester", "1.2.3"};
		std::string value;
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
		cli.parse("-s héllo"sv);
		CHECK(value == "héllo"sv);
	}

	SECTION("required option missing")
	{
		commandline cli{"tester", "1.2.3"};
		bool        flag{false};
		cli.flag({.short_name = "-f", .long_name = "--flag", .description = "A flag", .required = true}, &flag);
		CHECK_FALSE(cli.parse("-x"));

		CHECK_FALSE(cli.parse(""));

	}

	SECTION("duplicate option")
	{
		commandline cli{"tester", "1.2.3"};
		bool        flag{false};
		cli.flag({.short_name = "-f", .long_name = "--flag", .description = "A flag"}, &flag)
		  .flag({.short_name = "-f", .long_name = "--flag", .description = "Duplicate"}, nullptr);

		CHECK(cli.parse("-f"));
		CHECK(flag);
	}

	SECTION("help and version flags")
	{
		commandline cli{"tester", "1.2.3"};
		CHECK_FALSE(cli.parse("-h"));
		CHECK_FALSE(cli.parse("--help"));
		CHECK_FALSE(cli.parse("-V"));
		CHECK_FALSE(cli.parse("--version"));
	}
}
