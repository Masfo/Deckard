#include <catch2/catch_test_macros.hpp>


import std;
import deckard.as;
import deckard.types;
import deckard.config;
import deckard.debug;
using namespace deckard;
using namespace std::string_view_literals;

TEST_CASE("config parsing", "[config]")
{

	SECTION("newlines")
	{
		//
		std::array<u8, 4> c{'\n', '\r', '\n', '\r'}; // LF (\n) posix, CRLF (\r\n) windows, CR (\r) old mac

		config cfg(c);

		CHECK(cfg.size() == 3);
		CHECK(cfg[0].type == TokenType::NEWLINE_POSIX);
		CHECK(cfg[1].type == TokenType::NEWLINE_WINDOWS);
		CHECK(cfg[2].type == TokenType::NEWLINE_POSIX);
	}

	SECTION("empty comment")
	{
		config c(utf8::string("#\n"));

		CHECK(c.size() == 2);

		CHECK(c[0].type == TokenType::COMMENT);
		CHECK(c[0].start == 1);
		CHECK(c[0].length == 0);

		CHECK(c[1].type == TokenType::NEWLINE_POSIX);
		CHECK(c[1].start == 1);
		CHECK(c[1].length == 1);
	}

	SECTION("utf comment without newline")
	{
		//
		std::array<u8, 10> c = {'#', 'c', 'o', 'm', 'm', ' ', 0xF0, 0x9F, 0x8C, 0x8D}; // "comm 🌍"

		config cfg(c);
		CHECK(cfg.size() == 1);
		CHECK(cfg[0].type == TokenType::COMMENT);
		CHECK(cfg[0].start == 1);
		CHECK(cfg[0].length == 6);
	}

	SECTION("utf comment with whitespacew")
	{
		//
		std::array<u8, 14> c = {'#', ' ', ' ', 'c', 'o', 'm', 'm', ' ', 0xF0, 0x9F, 0x8C, 0x8D, ' ', ' '}; // "comm 🌍  "

		config cfg(c);
		CHECK(cfg.size() == 1);
		CHECK(cfg[0].type == TokenType::COMMENT);
		CHECK(cfg[0].start == 3);
		CHECK(cfg[0].length == 6);
	}

	SECTION("longer comment with newlines")
	{
		//                         1         2          3         4
		//             012345678 9 012345678901234 56789012345678901 2345
		config cfg(utf8::string("\n#comment\r\n#another comment\r#third comment\n#❌"));
		CHECK(cfg.size() == 8);

		CHECK(cfg[0].type == TokenType::NEWLINE_POSIX);
		CHECK(cfg[0].start == 0);
		CHECK(cfg[0].length == 1);


		CHECK(cfg[1].type == TokenType::COMMENT); // comment
		CHECK(cfg[1].start == 2);
		CHECK(cfg[1].length == 7);

		CHECK(cfg[2].type == TokenType::NEWLINE_WINDOWS);
		CHECK(cfg[2].start == 9);
		CHECK(cfg[2].length == 2);

		CHECK(cfg[3].type == TokenType::COMMENT); // another comment
		CHECK(cfg[3].start == 12);
		CHECK(cfg[3].length == 15);

		CHECK(cfg[4].type == TokenType::NEWLINE_POSIX);
		CHECK(cfg[4].start == 27);
		CHECK(cfg[4].length == 1);

		CHECK(cfg[5].type == TokenType::COMMENT); // third comment
		CHECK(cfg[5].start == 29);
		CHECK(cfg[5].length == 13);

		CHECK(cfg[6].type == TokenType::NEWLINE_POSIX);
		CHECK(cfg[6].start == 42);
		CHECK(cfg[6].length == 1);

		CHECK(cfg[7].type == TokenType::COMMENT); // ❌
		CHECK(cfg[7].start == 44);
		CHECK(cfg[7].length == 1);
	}

	SECTION("section parsing")
	{
		config cfg = config("[section]"sv);
		CHECK(cfg.size() == 1);
		CHECK(cfg[0].type == TokenType::SECTION);
		CHECK(cfg[0].start == 1);
		CHECK(cfg[0].length == 7);
	}

	SECTION("section parsing with comment")
	{
		config cfg = config("[section] # comment"sv);

		CHECK(cfg.size() == 2);

		CHECK(cfg[0].type == TokenType::SECTION);
		CHECK(cfg[0].start == 1);
		CHECK(cfg[0].length == 7);

		CHECK(cfg[1].type == TokenType::COMMENT);
		CHECK(cfg[1].start == 12);
		CHECK(cfg[1].length == 7);
	}

	SECTION("section parsing with comment and newline")
	{
		config cfg = config("[section] # comment\n"sv);
		CHECK(cfg.size() == 3);

		CHECK(cfg[0].type == TokenType::SECTION);
		CHECK(cfg[0].start == 1);
		CHECK(cfg[0].length == 7);

		CHECK(cfg[1].type == TokenType::COMMENT);
		CHECK(cfg[1].start == 12);
		CHECK(cfg[1].length == 7);

		CHECK(cfg[2].type == TokenType::NEWLINE_POSIX);
		CHECK(cfg[2].start == 19);
		CHECK(cfg[2].length == 1);
	}

	SECTION("utf8 section parsing with comment and newline")
	{
		config cfg = config(utf8::string("[🌍] # comment\n"));
		CHECK(cfg.size() == 3);

		CHECK(cfg[0].type == TokenType::SECTION);
		CHECK(cfg[0].start == 1);
		CHECK(cfg[0].length == 1);

		CHECK(cfg[1].type == TokenType::COMMENT);
		CHECK(cfg[1].start == 6);
		CHECK(cfg[1].length == 7);

		CHECK(cfg[2].type == TokenType::NEWLINE_POSIX);
		CHECK(cfg[2].start == 13);
		CHECK(cfg[2].length == 1);
	}

	SECTION("invalid section: empty []")
	{
		config cfg = config("[]"sv);
		CHECK(cfg.has_errors() == true);
		CHECK(cfg.size() == 0);
	}

	SECTION("invalid section: whitespace only [  ]")
	{
		config cfg = config("[  ]"sv);
		CHECK(cfg.has_errors() == true);
		CHECK(cfg.size() == 0);
	}

	SECTION("invalid section: missing closing bracket")
	{
		config cfg = config("[section"sv);
		CHECK(cfg.has_errors() == true);
		CHECK(cfg.size() == 0);
	}

	SECTION("invalid section: newline before closing bracket")
	{
		// newline terminates section early; \n still produces a NEWLINE token
		config cfg = config("[\n"sv);
		CHECK(cfg.has_errors() == true);
		CHECK(cfg.size() == 1);
		CHECK(cfg[0].type == TokenType::NEWLINE_POSIX);
	}

	SECTION("boolean parsing")
	{
		//
		config cfg = config("truekey = true"sv);

		cfg.dump();
		_ = 0;
	}

	SECTION("value type: bool")
	{
		config cfg = config("a = true\nb = false"sv);

		CHECK(cfg[1].type == TokenType::VALUE_BOOL);
		CHECK(cfg[4].type == TokenType::VALUE_BOOL);
		CHECK(cfg["a"].as<bool>() == true);
		CHECK(cfg["b"].as<bool>() == false);
	}

	SECTION("value type: i32eger")
	{
		config cfg = config("count = 42\nneg = -7\nzero = 0"sv);

		CHECK(cfg[1].type == TokenType::VALUE_INTEGER);
		CHECK(cfg[4].type == TokenType::VALUE_INTEGER);
		CHECK(cfg[7].type == TokenType::VALUE_INTEGER);
		CHECK(cfg["count"].as<i32>() == 42);
		CHECK(cfg["neg"].as<i32>() == -7);
		CHECK(cfg["zero"].as<i32>() == 0);
	}

	SECTION("value type: f32")
	{
		config cfg = config("pi = 3.14\neps = 1.0e-5"sv);

		CHECK(cfg[1].type == TokenType::VALUE_FLOAT);
		CHECK(cfg[4].type == TokenType::VALUE_FLOAT);
		CHECK(cfg["pi"].as<f32>() == 3.14f);
	}

	SECTION("value type: string stays string")
	{
		config cfg = config("name = hello"sv);

		CHECK(cfg[1].type == TokenType::VALUE_STRING);
		CHECK(cfg["name"].as<std::string>() == "hello");
	}
}

TEST_CASE("config getters/setters", "[config]")
{
	SECTION("global boolean get")
	{
		config cfg = config(R"(global = true)"sv);

		CHECK(cfg["global"] == true);
	}

	SECTION("section boolean get")
	{
		config cfg = config(R"(
global = true
[init]
local = true
blob = false;
)"sv);

		CHECK(cfg["global"].as<bool>() == true);

		CHECK(cfg["init.local"].as<bool>() == true);
		CHECK(cfg["init.blob"].as<bool>() == false);

		cfg["init.blob"] = true;
		CHECK(cfg["init.blob"].as<bool>() == true);

		cfg["init.blob"] = false;
		CHECK(cfg["init.blob"].as<bool>() == false);
	}

	SECTION("quoted string parsing")
	{
		config cfg = config(R"(
name    = "Alice"
title   = "Hello World"
empty   = ""
count   = 42
active  = true
)"sv);
		CHECK(cfg["name"].as<std::string>() == "Alice");
		CHECK(cfg["title"].as<std::string>() == "Hello World");
		CHECK(cfg["empty"].as<std::string>() == "");
		CHECK(cfg["count"].as<i32>() == 42);
		CHECK(cfg["active"] == true);
	}

	SECTION("quoted string set and update")
	{
		config cfg;
		cfg["greeting"] = "Hello";
		CHECK(cfg["greeting"].as<std::string>() == "Hello");

		cfg["greeting"] = "World";
		CHECK(cfg["greeting"].as<std::string>() == "World");

		cfg["greeting"] = "Hi";
		CHECK(cfg["greeting"].as<std::string>() == "Hi");
	}

	SECTION("create new section, keys, and values")
	{
		config cfg;
		cfg["newkey"] = "value";
		CHECK(cfg["newkey"].as<std::string>() == "value");
		cfg["section1.newkey"]     = "value1";
		cfg["section1.anotherkey"] = "value2";
		CHECK(cfg["section1.newkey"].as<std::string>() == "value1");
		CHECK(cfg["section1.anotherkey"].as<std::string>() == "value2");
	}

	SECTION("update modifies original key when duplicates exist")
	{
		config cfg = config("dup = 1\ndup = 2\n"sv);

		cfg["dup"] = 99;

		CHECK(cfg["dup"].as<i32>() == 99);
		CHECK(cfg.to_string() == "dup = 99\ndup = 2\n");
	}

	SECTION("modify key value, not comment")
	{
		config cfg = config("width = 1 # only modify value"sv);

		CHECK(cfg["width"].as<i32>() == 1);
		CHECK(cfg.to_string() == "width = 1 # only modify value");

		cfg["width"] = 99;
		CHECK(cfg["width"].as<i32>() == 99);

		CHECK(cfg.to_string() == "width = 99 # only modify value");
	}

	SECTION("modify key value, with section, not comment")
	{
		config cfg = config("[window]\nwidth = 1 # only modify value"sv);

		CHECK(cfg["window.width"].as<i32>() == 1);
		CHECK(cfg.to_string() == "[window]\nwidth = 1 # only modify value");

		cfg["window.width"] = 99;
		CHECK(cfg["window.width"].as<i32>() == 99);

		CHECK(cfg.to_string() == "[window]\nwidth = 99 # only modify value");
	}
}

TEST_CASE("serialize", "[config]")
{

	SECTION("serialize empty config")
	{
		config cfg;
		CHECK(cfg.to_string() == "");
	}

	SECTION("serialize simple key-value pair")
	{
		config cfg;
		cfg["key"] = "value";
		CHECK(cfg.to_string() == "\nkey = \"value\"\n");
	}

	SECTION("serialize section with key-value pairs")
	{
		config cfg;
		cfg["section.key1"] = "value1";
		cfg["section.key2"] = 42;
		cfg["section.key3"] = true;

		CHECK(cfg[1].type == TokenType::SECTION);


		CHECK(cfg.to_string() == "\n[section]\nkey1 = \"value1\"\nkey2 = 42\nkey3 = true\n");

		config cfg2(cfg.to_string());
	}
}

TEST_CASE("config all features under one section", "[config]")
{
	constexpr std::string_view input = R"(
[app]
name    = "MyApp"
version = 3
scale   = 1.5
debug   = true
visible = false
)";

	SECTION("read all types")
	{
		config cfg(input);

		CHECK(cfg["app.name"].as<std::string>() == "MyApp");
		CHECK(cfg["app.version"].as<i32>() == 3);
		CHECK(cfg["app.scale"].as<f32>() == 1.5f);
		CHECK(cfg["app.debug"].as<bool>() == true);
		CHECK(cfg["app.visible"].as<bool>() == false);
	}

	SECTION("update all types")
	{
		config cfg(input);

		cfg["app.name"]    = "NewApp";
		cfg["app.version"] = -7;
		cfg["app.scale"]   = -2.5f;
		cfg["app.debug"]   = false;
		cfg["app.visible"] = true;

		CHECK(cfg["app.name"].as<std::string>() == "NewApp");
		CHECK(cfg["app.version"].as<i32>() == -7);
		CHECK(cfg["app.scale"].as<f32>() == -2.5f);
		CHECK(cfg["app.debug"].as<bool>() == false);
		CHECK(cfg["app.visible"].as<bool>() == true);
	}

	SECTION("round-trip: parse -> serialize -> re-parse")
	{
		config cfg(input);
		auto   serialized = cfg.to_string();

		config cfg2(serialized);

		CHECK(cfg2["app.name"].as<std::string>() == "MyApp");
		CHECK(cfg2["app.version"].as<i32>() == 3);
		CHECK(cfg2["app.scale"].as<f32>() == 1.5f);
		CHECK(cfg2["app.debug"].as<bool>() == true);
		CHECK(cfg2["app.visible"].as<bool>() == false);
	}

	SECTION("missing key returns default")
	{
		config cfg(input);

		CHECK(cfg["app.nonexistent"].as<std::string>() == "");
		CHECK(cfg["app.nonexistent"].as<i32>() == 0);
		CHECK(cfg["app.nonexistent"].as<bool>() == false);
	}

	SECTION("add new keys to existing section")
	{
		config cfg(input);

		cfg["app.extra"] = "bonus";
		CHECK(cfg["app.extra"].as<std::string>() == "bonus");

		cfg["app.count"] = 99;
		CHECK(cfg["app.count"].as<i32>() == 99);
	}
}

TEST_CASE("config global key placement", "[config]")
{
	SECTION("global key on empty config is at top")
	{
		config cfg;
		cfg["version"] = 1;

		CHECK(cfg["version"].as<i32>() == 1);
		CHECK(cfg.to_string() == "\nversion = 1\n");
	}

	SECTION("global key added to config with section appears before section")
	{
		config cfg("[window]\nwidth = 1280"sv);
		cfg["version"] = 2;

		CHECK(cfg["version"].as<i32>() == 2);
		CHECK(cfg.to_string() == "\nversion = 2\n\n[window]\nwidth = 1280");
	}

	SECTION("global string key added to config with section appears before section")
	{
		config cfg("[app]\nname = test"sv);
		cfg["title"] = "MyApp";

		auto s = cfg.to_string().to_string();
		CHECK(cfg["title"].as<std::string>() == "MyApp");
		CHECK(s.find("title") < s.find("[app]"));
	}

	SECTION("global key readable after adding to config with existing section keys")
	{
		config cfg("[db]\nhost = localhost\nport = 5432"sv);
		cfg["debug"]    = false;
		cfg["loglevel"] = 3;

		CHECK(cfg["debug"].as<bool>() == false);
		CHECK(cfg["loglevel"].as<i32>() == 3);
		CHECK(cfg["db.host"].as<std::string>() == "localhost");
		CHECK(cfg["db.port"].as<i32>() == 5432);

		auto s = cfg.to_string().to_string();
		CHECK(s.find("loglevel") < s.find("[db]"));
		CHECK(s.find("debug") < s.find("[db]"));
	}
}

TEST_CASE("config utf8 string values", "[config]")
{
	SECTION("parse quoted utf8 string value")
	{
		config cfg = config("name = \"caf\xC3\xA9\""sv);

		CHECK(cfg["name"].as<std::string>() == "caf\xC3\xA9");
	}

	SECTION("parse quoted emoji string value")
	{
		config cfg = config("icon = \"\xF0\x9F\x8C\x8D\""sv);

		CHECK(cfg["icon"].as<std::string>() == "\xF0\x9F\x8C\x8D");
	}

	SECTION("parse quoted utf8 string with section")
	{
		config cfg = config("[app]\ngreeting = \"H\xC3\xA9llo W\xC3\xB6rld\""sv);

		CHECK(cfg["app.greeting"].as<std::string>() == "H\xC3\xA9llo W\xC3\xB6rld");
	}

	SECTION("set key with utf8 string value (new key)")
	{
		config cfg;
		cfg["label"] = "caf\xC3\xA9";

		CHECK(cfg["label"].as<std::string>() == "caf\xC3\xA9"sv);
	}

	SECTION("set key with emoji string value (new key)")
	{
		config cfg;
		cfg["icon"] = "\xF0\x9F\x8C\x8D";

		CHECK(cfg["icon"].as<std::string>() == "\xF0\x9F\x8C\x8D"sv);
	}

	SECTION("update existing key with utf8 string value")
	{
		config cfg = config("name = \"hello\""sv);

		cfg["name"] = "caf\xC3\xA9";

		CHECK(cfg["name"].as<std::string>() == "caf\xC3\xA9");
	}

	SECTION("update existing key with emoji string value")
	{
		config cfg = config("icon = \"star\""sv);

		cfg["icon"] = "\xF0\x9F\x8C\x8D";

		CHECK(cfg["icon"].as<std::string>() == "\xF0\x9F\x8C\x8D");
	}

	SECTION("round-trip utf8 string value")
	{
		config cfg;
		cfg["title"] = "H\xC3\xA9llo \xF0\x9F\x8C\x8D";

		auto   serialized = cfg.to_string();
		config cfg2(serialized);

		CHECK(cfg2["title"].as<std::string>() == "H\xC3\xA9llo \xF0\x9F\x8C\x8D");
	}

	SECTION("round-trip utf8 string with section")
	{
		config cfg;
		cfg["window.title"] = "H\xC3\xA9llo \xF0\x9F\x8C\x8D";

		auto   serialized = cfg.to_string();
		config cfg2(serialized);

		CHECK(cfg2["window.title"].as<std::string>() == "H\xC3\xA9llo \xF0\x9F\x8C\x8D");
	}
}

TEST_CASE("config set_comment", "[config]")
{
	SECTION("add comment to existing key without comment")
	{
		config cfg("width = 1280"sv);
		cfg.set_comment("width", "screen width in pixels");

		auto s = cfg.to_string().to_string();
		CHECK(s.find("width = 1280 # screen width in pixels") != std::string::npos);
	}

	SECTION("replace existing comment on key")
	{
		config cfg("width = 1280 # old comment"sv);
		cfg.set_comment("width", "new comment");

		auto s = cfg.to_string().to_string();
		CHECK(s.find("width = 1280 # new comment") != std::string::npos);
		CHECK(s.find("old comment") == std::string::npos);
	}

	SECTION("set comment on sectioned key")
	{
		config cfg("[window]\nwidth = 1280"sv);
		cfg.set_comment("window.width", "display width");

		auto s = cfg.to_string().to_string();
		CHECK(s.find("width = 1280 # display width") != std::string::npos);
	}

	SECTION("set_comment on non-existent key does nothing")
	{
		config cfg("width = 1280"sv);
		cfg.set_comment("height", "should fail silently");

		auto s = cfg.to_string().to_string();
		CHECK(s.find("height") == std::string::npos);
		CHECK(s.find("width = 1280") != std::string::npos);
	}

	SECTION("value is preserved when updating comment")
	{
		config cfg("count = 42 # old"sv);
		cfg.set_comment("count", "answer to everything");

		CHECK(cfg["count"].as<i32>() == 42);
		CHECK(cfg.to_string().to_string().find("count = 42 # answer to everything") != std::string::npos);
	}

	SECTION("has_multiple is false for unique key")
	{
		config cfg = config("key = one\n"sv);

		CHECK(cfg.has_multiple("key") == false);
	}

	SECTION("has_multiple is true for duplicate key")
	{
		config cfg = config("key = one\nkey = two\n"sv);

		CHECK(cfg.has_multiple("key") == true);
	}

	SECTION("get returns first value when duplicates exist")
	{
		config cfg = config("key = first\nkey = second\nkey = third\n"sv);

		CHECK(cfg["key"].as<std::string>() == "first");
	}

	SECTION("get_all returns all values for duplicate string key")
	{
		config cfg = config("key = one\nkey = two\nkey = three\n"sv);

		auto values = cfg["key"].as_all<std::string>();

		REQUIRE(values.size() == 3);
		CHECK(values[0] == "one");
		CHECK(values[1] == "two");
		CHECK(values[2] == "three");
	}

	SECTION("get_all returns all values for duplicate integer key")
	{
		config cfg = config("port = 80\nport = 443\nport = 8080\n"sv);

		auto values = cfg["port"].as_all<i32>();

		REQUIRE(values.size() == 3);
		CHECK(values[0] == 80);
		CHECK(values[1] == 443);
		CHECK(values[2] == 8080);
	}

	SECTION("get_all returns all values for duplicate bool key")
	{
		config cfg = config("flag = true\nflag = false\nflag = true\n"sv);

		auto values = cfg["flag"].as_all<bool>();

		REQUIRE(values.size() == 3);
		CHECK(values[0] == true);
		CHECK(values[1] == false);
		CHECK(values[2] == true);
	}

	SECTION("get_all returns single-element vector for unique key")
	{
		config cfg = config("key = only\n"sv);

		auto values = cfg["key"].as_all<std::string>();

		REQUIRE(values.size() == 1);
		CHECK(values[0] == "only");
	}

	SECTION("get_all returns empty vector for missing key")
	{
		config cfg = config("key = value\n"sv);

		auto values = cfg["missing"].as_all<std::string>();

		CHECK(values.empty());
	}

	SECTION("duplicate keys under section")
	{
		config cfg = config("[server]\nhost = alpha\nhost = beta\nhost = gamma\n"sv);

		CHECK(cfg.has_multiple("server.host") == true);
		CHECK(cfg["server.host"].as<std::string>() == "alpha");

		auto values = cfg["server.host"].as_all<std::string>();

		REQUIRE(values.size() == 3);
		CHECK(values[0] == "alpha");
		CHECK(values[1] == "beta");
		CHECK(values[2] == "gamma");
	}

	SECTION("duplicate keys mixed with unique keys under section")
	{
		config cfg = config("[net]\nport = 80\nport = 443\ntimeout = 30\n"sv);

		CHECK(cfg.has_multiple("net.port") == true);
		CHECK(cfg.has_multiple("net.timeout") == false);

		auto ports = cfg["net.port"].as_all<i32>();
		REQUIRE(ports.size() == 2);
		CHECK(ports[0] == 80);
		CHECK(ports[1] == 443);

		CHECK(cfg["net.timeout"].as<i32>() == 30);
	}

	SECTION("set on duplicate key updates only the first occurrence")
	{
		config cfg = config("dup = 1\ndup = 2\ndup = 3\n"sv);

		cfg["dup"] = 99;

		auto values = cfg["dup"].as_all<i32>();
		REQUIRE(values.size() == 3);
		CHECK(values[0] == 99);
		CHECK(values[1] == 2);
		CHECK(values[2] == 3);
	}

	SECTION("duplicate quoted string values")
	{
		config cfg = config("tag = \"alpha\"\ntag = \"beta\"\n"sv);

		auto values = cfg["tag"].as_all<std::string>();

		REQUIRE(values.size() == 2);
		CHECK(values[0] == "alpha");
		CHECK(values[1] == "beta");
	}

	SECTION("two sections with same key are independent")
	{
		config cfg = config("[a]\nkey = 1\n[b]\nkey = 2\n"sv);

		CHECK(cfg.has_multiple("a.key") == false);
		CHECK(cfg.has_multiple("b.key") == false);
		CHECK(cfg["a.key"].as<i32>() == 1);
		CHECK(cfg["b.key"].as<i32>() == 2);
	}

	SECTION("duplicate keys only in one of two sections")
	{
		config cfg = config("[a]\nkey = 1\nkey = 2\n[b]\nkey = 9\n"sv);

		CHECK(cfg.has_multiple("a.key") == true);
		CHECK(cfg.has_multiple("b.key") == false);

		auto a_vals = cfg["a.key"].as_all<i32>();
		REQUIRE(a_vals.size() == 2);
		CHECK(a_vals[0] == 1);
		CHECK(a_vals[1] == 2);

		CHECK(cfg["b.key"].as<i32>() == 9);
	}
}

