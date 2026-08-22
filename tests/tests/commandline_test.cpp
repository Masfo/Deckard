#include <catch2/catch_all.hpp>


import std;
import deckard.commandline;

TEST_CASE("commandline", "[commandline][cli]")
{
	using namespace Catch;
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

	SECTION("integer values")
	{
		commandline cli{"tester", "1.2.3"};
		i8          vi8{0};
		u8          vu8{0};
		i16         vi16{0};
		u16         vu16{0};
		i32         vi32{0};
		u32         vu32{0};
		i64         vi64{0};
		u64         vu64{0};

		cli.option({.short_name = "-i8", .long_name = "--int8", .description = "An int8 option"}, &vi8)
		  .option({.short_name = "-u8", .long_name = "--uint8", .description = "A uint8 option"}, &vu8)
		  .option({.short_name = "-i16", .long_name = "--int16", .description = "An int16 option"}, &vi16)
		  .option({.short_name = "-u16", .long_name = "--uint16", .description = "A uint16 option"}, &vu16)
		  .option({.short_name = "-i32", .long_name = "--int32", .description = "An int32 option"}, &vi32)
		  .option({.short_name = "-u32", .long_name = "--uint32", .description = "A uint32 option"}, &vu32)
		  .option({.short_name = "-i64", .long_name = "--int64", .description = "An int64 option"}, &vi64)
		  .option({.short_name = "-u64", .long_name = "--uint64", .description = "A uint64 option"}, &vu64);

		CHECK(cli.parse(
		  "-i8 -128 -u8 255 -i16 -32768 -u16 65535 -i32 -2147483648 -u32 4294967295 -i64 -9223372036854775808 -u64 18446744073709551615"sv));
		CHECK(vi8 == -128);
		CHECK(vu8 == 255);
		CHECK(vi16 == -32768);
		CHECK(vu16 == 65535);
		CHECK(vi32 == -2'147'483'648);
		CHECK(vu32 == 4'294'967'295);
		CHECK(vi64 == -9'223'372'036'854'775'808);
		CHECK(vu64 == 18'446'744'073'709'551'615);
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
		cli.level({.short_name = "-O", .long_name = "--optimize", .description = "Optimization level"}, &level, 0, 3, 2);
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

	SECTION("u64 level option")
	{
		commandline cli{"tester", "1.2.3"};
		u64         level{0};
		cli.level({.short_name = "-L", .long_name = "--level", .description = "A u64 level"}, &level);

		CHECK(cli.parse("-L5"));
		CHECK(level == 5);

		level = 0;
		CHECK(cli.parse("-L18446744073709551615")); // u64 max
		CHECK(level == std::numeric_limits<u64>::max());

		level = 1;
		CHECK(cli.parse("-L")); // default (0)
		CHECK(level == 0);
	}

	SECTION("u64 level option with range")
	{
		commandline cli{"tester", "1.2.3"};
		u64         level{0};
		cli.level({.short_name = "-L", .long_name = "--level", .description = "A u64 level"}, &level, 0ull, 100ull, 10ull);

		CHECK(cli.parse("-L42"));
		CHECK(level == 42);

		level = 0;
		CHECK(cli.parse("-L1000")); // out of range
		CHECK(level == 100);        // clamped to max

		level = 0;
		CHECK(cli.parse("-L"));
		CHECK(level == 10); // default
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


		verbose = false;
		count   = 0;
		name    = "";
		CHECK(cli.parse("--verbose --number 20 --string example"sv));
		CHECK(verbose);
		CHECK(count == 20);
		CHECK(name == "example"sv);

		verbose = false;
		count   = 0;
		name    = "";
		CHECK(cli.parse("-n 5 -v -s sample"sv));
		CHECK(verbose);
		CHECK(count == 5);
		CHECK(name == "sample"sv);
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
		CHECK(cli.parse("  -v  -n  42  "sv));
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

	SECTION("unterminated single-quoted string")
	{
		commandline cli{"tester", "1.2.3"};
		std::string value;
		cli.option({.short_name = "-s", .long_name = "--string", .description = "A string option"}, &value);
		CHECK_FALSE(cli.parse("-s 'quoted value"sv));
		CHECK(value.empty());
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
		CHECK(cli.parse("-s héllo"sv));
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

	SECTION("level option without target")
	{
		commandline cli{"tester", "1.2.3"};
		cli.level<int>({.short_name = "-L", .long_name = "--level", .description = "Level, no target"}, nullptr, 0, 3, 2);

		CHECK(cli.parse("-L2"));
		CHECK(cli.parse("-L"));                              // default, discarded
		CHECK(cli.parse("-L9"));                             // clamped, discarded
		CHECK_FALSE(cli.parse("-L99999999999999999999999")); // out of range still errors
	}

	//

	SECTION("two-value option")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0}, y{0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "A 2D point"}, &x, &y);

		CHECK(cli.parse("-p 3 4"));
		CHECK(x == 3);
		CHECK(y == 4);

		x = 0;
		y = 0;
		CHECK(cli.parse("--point -10 20"));
		CHECK(x == -10);
		CHECK(y == 20);
	}

	SECTION("two-value option missing value")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0}, y{0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "A 2D point"}, &x, &y);

		CHECK_FALSE(cli.parse("-p 3")); // second value missing
		CHECK_FALSE(cli.parse("-p"));   // both missing
	}

	SECTION("two-value option invalid value")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0}, y{0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "A 2D point"}, &x, &y);

		CHECK_FALSE(cli.parse("-p 3 abc"));
	}

	SECTION("two-value option consumes next token greedily")
	{
		commandline cli{"tester", "1.2.3"};
		bool        verbose{false};
		int         x{0}, y{0};
		cli.flag({.short_name = "-v", .long_name = "--verbose", .description = "Verbose"}, &verbose);
		cli.option({.short_name = "-p", .long_name = "--point", .description = "A 2D point"}, &x, &y);

		CHECK_FALSE(cli.parse("-p 3 -v")); // "-v" is taken as the 2nd value, int conversion fails
		CHECK_FALSE(verbose);
	}

	//
	SECTION("multiple two-value options")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0}, y{0};
		int         a{0}, b{0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "A 2D point"}, &x, &y);
		cli.option({.short_name = "-q", .long_name = "--quad", .description = "Another 2D point"}, &a, &b);
		CHECK(cli.parse("-p 1 2 -q 3 4"));
		CHECK(x == 1);
		CHECK(y == 2);
		CHECK(a == 3);
		CHECK(b == 4);
		x = y = a = b = 0;
		bool verbose{false};
		cli.flag({.short_name = "-v", .long_name = "--verbose", .description = "Verbose"}, &verbose);
		CHECK(cli.parse("--point -5 -6 -v --quad 7 8"));
		CHECK(x == -5);
		CHECK(y == -6);
		CHECK(a == 7);
		CHECK(b == 8);
		CHECK(verbose);
	}

	SECTION("multiple bools")
	{
		commandline cli{"tester", "1.2.3"};
		bool        a{false}, b{false}, c{false};
		cli.flag({.short_name = "-a", .long_name = "--alpha", .description = "Alpha"}, &a);
		cli.flag({.short_name = "-b", .long_name = "--beta", .description = "Beta"}, &b);
		cli.flag({.short_name = "-c", .long_name = "--charlie", .description = "Charlie"}, &c);
		CHECK(cli.parse("-a -b"));
		CHECK(a);
		CHECK(b);
		CHECK_FALSE(c);
		a = b = c = false;
		CHECK(cli.parse("--charlie --alpha"));
		CHECK(a);
		CHECK_FALSE(b);
		CHECK(c);
	}

	SECTION("float value")
	{
		commandline cli{"tester", "1.2.3"};
		f32         value{0.0f};
		cli.option({.short_name = "-f", .long_name = "--factor", .description = "A float option"}, &value);

		CHECK(cli.parse("-f 1.5"));
		CHECK(value == Approx(1.5f));

		value = 0.0f;
		CHECK(cli.parse("--factor -0.25"));
		CHECK(value == Approx(-0.25f));

		value = 0.0f;
		CHECK(cli.parse("-f 1e3"));
		CHECK(value == Approx(1000.0f));

		CHECK_FALSE(cli.parse("-f abc"));
	}

	SECTION("double value")
	{
		commandline cli{"tester", "1.2.3"};
		f64         value{0.0};
		cli.option({.short_name = "-d", .long_name = "--delta", .description = "A double option"}, &value);

		CHECK(cli.parse("-d 3.141592653589793"));
		CHECK(value == Approx(3.141592653589793));

		value = 0.0;
		CHECK(cli.parse("--delta 1e-9"));
		CHECK(value == Approx(1e-9));

		CHECK_FALSE(cli.parse("-d 1.2.3"));
	}

	SECTION("two-value float option")
	{
		commandline cli{"tester", "1.2.3"};
		f64         x{0.0}, y{0.0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "A 2D point"}, &x, &y);

		CHECK(cli.parse("-p 1.5 -2.5"));
		CHECK(x == Approx(1.5));
		CHECK(y == Approx(-2.5));

		CHECK_FALSE(cli.parse("-p 1.5"));       // missing second value
		CHECK_FALSE(cli.parse("-p 1.5 nan-x")); // invalid second value
	}

	//
	SECTION("two-value option duplicate targets rejected")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "Duplicate targets"}, &x, &x);

		CHECK_FALSE(cli.parse("-p 3 4"));
		CHECK(x == 0);
	}

	SECTION("two-value option distinct targets accepted")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0}, y{0};
		cli.option({.short_name = "-p", .long_name = "--point", .description = "Distinct"}, &x, &y);
		CHECK(cli.parse("-p 3 4"));
		CHECK(x == 3);
		CHECK(y == 4);
	}

	SECTION("three values option")
	{
		commandline cli{"tester", "1.2.3"};
		int         x{0}, y{0}, z{0};
		cli.option({.short_name = "-p", .long_name = "--point3d", .description = "A 3D point"}, &x, &y, &z);
		CHECK(cli.parse("-p 1 2 3"));
		CHECK(x == 1);
		CHECK(y == 2);
		CHECK(z == 3);
		CHECK_FALSE(cli.parse("-p 1 2")); // missing third value
	}

	SECTION("three strings option")
	{
		commandline cli{"tester", "1.2.3"};
		std::string a, b, c;
		cli.option({.short_name = "-t", .long_name = "--triple", .description = "Three strings"}, &a, &b, &c);

		CHECK(cli.parse("-t one two three"));
		CHECK(a == "one");
		CHECK(b == "two");
		CHECK(c == "three");

		CHECK(cli.parse("--triple alpha beta gamma"));
		CHECK(a == "alpha");
		CHECK(b == "beta");
		CHECK(c == "gamma");

		CHECK_FALSE(cli.parse("-t one two")); // missing third value
	}
}
