#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.types;
import deckard.lexer;
import deckard.math;
import deckard.utf8;
import deckard.debug;
import std;
#undef EOF

using namespace Catch::Matchers;
namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace deckard::math;
using namespace deckard::utf8;
using namespace deckard;

using namespace deckard::lexer;

namespace details
{
	std::unordered_map<TokenKind, u32> token_counts;
}

void CHECK_TOKEN(const utf8::view str, const Token& token, TokenKind expected_type, u32 expected_line, u32 expected_column,
				 utf8::view correct_str, TokenError expected_error = TokenError::None)
{
	details::token_counts[token.type]++;

	if (token.group == TokenGroup::Literal or token.group == TokenGroup::Identifier or token.group == TokenGroup::Keyword)
	{
		CHECK(correct_str == detail::pool.get(token.id));
	}
	else
	{
		// CHECK(token.id == limits::max<u32>);
	}

	if (token.line != expected_line || token.column != expected_column)
	{
		// Show visual layout on failure
		dbg::eprintln("Token layout mismatch at line {}, column {}:", token.line, token.column);
		//	// Display the line with column markers
		//	auto lines = str.split('\n');
		//	if (token.line <= lines.size())
		//	{
		//		dbg::eprintln("{}", lines[token.line - 1]);
		//		dbg::eprintln("{}^", std::string(token.column - 1, ' '));
		//	}
	}

	CHECK(token.type == expected_type);
	CHECK(token.line == expected_line);
	CHECK(token.column == expected_column);
	CHECK(token.length == correct_str.length());

	CHECK(token.error == expected_error);

	auto extracted_str = extract_token(str, token);
	if (not extracted_str)
		dbg::println("Failed to extract token of type {} at line {}, column {}",
					 std::to_underlying(token.type),
					 token.line,
					 token.column);

	REQUIRE(extracted_str);
	CHECK(*extracted_str == correct_str);
}

TEST_CASE("tokens", "[lexer]")
{

	SECTION("single ident")
	{
		utf8::string       str = "abc"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);

		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("abc"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 4, utf8::view(""));
	}

	SECTION("stringview initialize")
	{
		// line 1: abc\n
		// line 2: qwerty\r\n
		// line 3: hjkl
		utf8::string       str = "abc\nqwerty\r\nhjkl"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);

		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("abc"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Identifier, 2, 1, utf8::view("qwerty"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Identifier, 3, 1, utf8::view("hjkl"));
		CHECK_TOKEN(str, tokens[3], TokenKind::EOF, 3, 5, utf8::view(""));
	}

	SECTION("integer")
	{
		utf8::string       str = "123 456_789"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("123"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Integer, 1, 5, utf8::view("456_789"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 12, utf8::view(""));
	}


	SECTION("negative integer")
	{
		utf8::string       str = "-42"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);
		CHECK(tokens.size() == 3);

		CHECK_TOKEN(str, tokens[0], TokenKind::Minus, 1, 1, utf8::view("-"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Integer, 1, 2, utf8::view("42"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 4, utf8::view(""));
	}

	SECTION("invalid integer")
	{
		utf8::string       str = "666a"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("666a"), TokenError::InvalidInteger);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("hex integer")
	{
		utf8::string       str = "0x29A 0xbad"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("0x29A"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Integer, 1, 7, utf8::view("0xbad"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 12, utf8::view(""));
	}

	SECTION("invalid hex integer")
	{
		utf8::string       str = "0xG"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("0xG"), TokenError::InvalidHex);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 4, utf8::view(""));
	}


	SECTION("binary integer")
	{
		utf8::string       str = "0b101010"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("0b101010"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	SECTION("binary integer trailing underscore")
	{
		utf8::string       str = "0b10_"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("0b10_"), TokenError::InvalidBinary);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 6, utf8::view(""));
	}

	SECTION("invalid binary integer")
	{
		utf8::string       str = "0b2"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("0b2"), TokenError::InvalidBinary);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 4, utf8::view(""));
	}


	SECTION("floating point")
	{
		utf8::string       str = "3.1415 1."sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::FloatingPoint, 1, 1, utf8::view("3.1415"));
		CHECK_TOKEN(str, tokens[1], TokenKind::FloatingPoint, 1, 8, utf8::view("1."));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 10, utf8::view(""));
	}

	SECTION("floating point trailing underscore")
	{
		utf8::string       str = "3.14_15_"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::FloatingPoint, 1, 1, utf8::view("3.14_15_"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	
	SECTION("hex integer trailing underscore")
	{
		utf8::string       str = "0x10_"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("0x10_"), TokenError::InvalidHex);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 6, utf8::view(""));
	}


	SECTION("negative floating point")
	{
		utf8::string       str = "-3.14"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);
		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::Minus, 1, 1, utf8::view("-"));
		CHECK_TOKEN(str, tokens[1], TokenKind::FloatingPoint, 1, 2, utf8::view("3.14"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 6, utf8::view(""));
	}

	SECTION("invalid floating point")
	{
		utf8::string       str = "1.2.3 1.a"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::FloatingPoint, 1, 1, utf8::view("1.2.3"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[1], TokenKind::FloatingPoint, 1, 7, utf8::view("1.a"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 10, utf8::view(""));
	}

	SECTION("floating point with exponent")
	{
		utf8::string       str = "1e5 3.14e10 1.e-3 2.5e+7 1_000.5e-2"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 6);
		CHECK_TOKEN(str, tokens[0], TokenKind::FloatingPoint, 1, 1, utf8::view("1e5"));
		CHECK_TOKEN(str, tokens[1], TokenKind::FloatingPoint, 1, 5, utf8::view("3.14e10"));
		CHECK_TOKEN(str, tokens[2], TokenKind::FloatingPoint, 1, 13, utf8::view("1.e-3"));
		CHECK_TOKEN(str, tokens[3], TokenKind::FloatingPoint, 1, 19, utf8::view("2.5e+7"));
		CHECK_TOKEN(str, tokens[4], TokenKind::FloatingPoint, 1, 26, utf8::view("1_000.5e-2"));
		CHECK_TOKEN(str, tokens[5], TokenKind::EOF, 1, 36, utf8::view(""));
	}

	SECTION("invalid floating point with exponent")
	{
		utf8::string       str = "1e 1e+ 1e- 1ea"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::FloatingPoint, 1, 1, utf8::view("1e"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[1], TokenKind::FloatingPoint, 1, 4, utf8::view("1e+"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[2], TokenKind::FloatingPoint, 1, 8, utf8::view("1e-"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[3], TokenKind::FloatingPoint, 1, 12, utf8::view("1ea"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 15, utf8::view(""));
	}

	SECTION("floating point comprehensive")
	{
		utf8::string       str = "1.5e-10 5. 5.5e1 1_2_3.4_5e-6_7"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::FloatingPoint, 1, 1, utf8::view("1.5e-10"sv));
		CHECK_TOKEN(str, tokens[1], TokenKind::FloatingPoint, 1, 9, utf8::view("5."sv));
		CHECK_TOKEN(str, tokens[2], TokenKind::FloatingPoint, 1, 12, utf8::view("5.5e1"sv));
		CHECK_TOKEN(str, tokens[3], TokenKind::FloatingPoint, 1, 18, utf8::view("1_2_3.4_5e-6_7"sv));
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 32, utf8::view(""));
	}

	SECTION("identifier intern")
	{
		utf8::string       str = "привет"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);
		CHECK(tokens.size() == 2);

		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("привет"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 7, utf8::view(""));
	}

	SECTION("underscore start")
	{
		utf8::string       str = "_foo"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("_foo"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("digit cannot start identifier")
	{
		utf8::string       str = "1abc"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Integer, 1, 1, utf8::view("1abc"), TokenError::InvalidInteger);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("dollar start")
	{
		utf8::string       str = "$bar"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("$bar"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("identifier")
	{
		utf8::string       str = "hello _world _123abc $dollar привет 東京 café π"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);
		CHECK(tokens.size() == 9);

		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("hello"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Identifier, 1, 7, utf8::view("_world"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Identifier, 1, 14, utf8::view("_123abc"));
		CHECK_TOKEN(str, tokens[3], TokenKind::Identifier, 1, 22, utf8::view("$dollar"));
		CHECK_TOKEN(str, tokens[4], TokenKind::Identifier, 1, 30, utf8::view("привет"));
		CHECK_TOKEN(str, tokens[5], TokenKind::Identifier, 1, 37, utf8::view("東京"));
		CHECK_TOKEN(str, tokens[6], TokenKind::Identifier, 1, 40, utf8::view("café"));
		CHECK_TOKEN(str, tokens[7], TokenKind::Identifier, 1, 45, utf8::view("π"));

		CHECK_TOKEN(str, tokens[8], TokenKind::EOF, 1, 46, utf8::view(""));
	}

	SECTION("single character")
	{
		utf8::string       str = R"('c')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 4, utf8::view(""));
	}

	SECTION("single character unicode")
	{
		utf8::string       str = R"('🌍')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('🌍')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 4, utf8::view(""));
	}

	SECTION("single hex character ")
	{
		utf8::string       str = R"('\x42')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\x42')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 7, utf8::view(""));
	}

	SECTION("character")
	{
		utf8::string       str = R"('c' '🌍' '\'' '\xff' '\n')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 6);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Character, 1, 5, utf8::view(R"('🌍')"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Character, 1, 9, utf8::view(R"('\'')"));
		CHECK_TOKEN(str, tokens[3], TokenKind::Character, 1, 14, utf8::view(R"('\xff')"));
		CHECK_TOKEN(str, tokens[4], TokenKind::Character, 1, 21, utf8::view(R"('\n')"));
		CHECK_TOKEN(str, tokens[5], TokenKind::EOF, 1, 25, utf8::view(""));
	}

	SECTION("invalid character (unterminated)")
	{
		utf8::string       str = "'x"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view("'x"), TokenError::InvalidCharacter);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 3, utf8::view(""));
	}

	SECTION("invalid character")
	{
		utf8::string       str = R"('\xfff')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\xfff')"), TokenError::InvalidCharacter);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 8, utf8::view(""));
	}


	SECTION("newline in character literal")
	{
		utf8::string       str = R"('a
', ident)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		//  REQUIRE(tokens.size() == 5);
		// CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view("'a"), TokenError::InvalidCharacterNewline);
		// CHECK_TOKEN(str, tokens[1], TokenKind::Character, 2, 1, utf8::view("'"));
		// CHECK_TOKEN(str, tokens[2], TokenKind::Comma, 2, 2, utf8::view(","));
		// CHECK_TOKEN(str, tokens[3], TokenKind::Identifier, 2, 4, utf8::view("ident"));
		// CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 2, 9, utf8::view(""));
	}


	SECTION("string")
	{
		utf8::string       str = R"("hello" "hel\nlo")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("hello")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::String, 1, 9, utf8::view(R"("hel\nlo")"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 18, utf8::view(""));
	}

	SECTION("invalid string (unterminated)")
	{
		utf8::string       str = "\"invalid"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"invalid"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	SECTION("escaped string - basic")
	{
		utf8::string       str = R"("\n" "\r" "\t")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\n")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::String, 1, 6, utf8::view(R"("\r")"));
		CHECK_TOKEN(str, tokens[2], TokenKind::String, 1, 11, utf8::view(R"("\t")"));
		CHECK_TOKEN(str, tokens[3], TokenKind::EOF, 1, 15, utf8::view(""));
	}

	SECTION("escaped string - hex")
	{
		utf8::string       str = R"("\xFF" "\xB")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\xFF")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::String, 1, 8, utf8::view(R"("\xB")"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 13, utf8::view(""));
	}

	SECTION("escaped string - invalid hex (invalid char)")
	{
		utf8::string       str = R"("\xG")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\xG")"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 6, utf8::view(""));
	}

	SECTION("escaped string - invalid hex (too long)")
	{
		utf8::string       str = R"("\xAABB")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\xAABB")"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	SECTION("escaped string - invalid hex (missing digits)")
	{
		utf8::string       str = R"("\x")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\x")"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("string with newline")
	{
		utf8::string       str = "\"first line\nsecond line\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"first line"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::Identifier, 2, 1, utf8::view("second"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Identifier, 2, 8, utf8::view("line"));
		CHECK_TOKEN(str, tokens[3], TokenKind::String, 2, 12, utf8::view("\""), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 2, 13, utf8::view(""));
	}

	SECTION("string with trailing backslash")
	{
		utf8::string       str = R"("trailing \)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("trailing \)"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 12, utf8::view(""));
	}

	SECTION("string with escaped newline")
	{
		utf8::string       str = "\"escaped \\\nnewline\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"escaped \\\nnewline\""));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 2, 9, utf8::view(""));
	}

	SECTION("string with escaped carriage return")
	{
		utf8::string       str = "\"escaped \\\rnewline\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"escaped \\\rnewline\""));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 2, 9, utf8::view(""));
	}

	SECTION("string with escaped mixed line ending")
	{
		utf8::string       str = "\"escaped \\\r\nnewline\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"escaped \\\r\nnewline\""));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 2, 9, utf8::view(""));
	}

	SECTION("unicode escape string")
	{
		utf8::string       str = R"("\u41" "\U0041" "\uFFFDzxc" "\U000012345")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\u41")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::String, 1, 8, utf8::view(R"("\U0041")"));
		CHECK_TOKEN(str, tokens[2], TokenKind::String, 1, 17, utf8::view(R"("\uFFFDzxc")"));
		CHECK_TOKEN(str, tokens[3], TokenKind::String, 1, 29, utf8::view(R"("\U000012345")"));
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 42, utf8::view(""));
	}

	SECTION("hex escape case sensitivity")
	{
		utf8::string       str = R"("\xAb" "\xaB" "\xab" "\xAB")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\xAb")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::String, 1, 8, utf8::view(R"("\xaB")"));
		CHECK_TOKEN(str, tokens[2], TokenKind::String, 1, 15, utf8::view(R"("\xab")"));
		CHECK_TOKEN(str, tokens[3], TokenKind::String, 1, 22, utf8::view(R"("\xAB")"));
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 28, utf8::view(""));
	}

	SECTION("empty character literal")
	{
		utf8::string       str = R"('')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('')"), TokenError::InvalidCharacterEmpty);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 3, utf8::view(""));
	}

	SECTION("character with multiple codepoints")
	{
		utf8::string       str = R"('ab')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('ab')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("character with multiple hex escapes")
	{
		utf8::string       str = R"('\x41\x42')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\x41\x42')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 11, utf8::view(""));
	}

	SECTION("character hex escape boundary - incomplete at end")
	{
		utf8::string       str = R"('\x)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\x)"), TokenError::InvalidCharacter);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 4, utf8::view(""));
	}

	SECTION("character with escaped quote")
	{
		utf8::string       str = R"('\'')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\'')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("character with escaped backslash")
	{
		utf8::string       str = R"('\\')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\\')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("string with escaped quote")
	{
		utf8::string       str = R"("hello\"world")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("hello\"world")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 15, utf8::view(""));
	}

	SECTION("string with escaped backslash")
	{
		utf8::string       str = R"("path\\to\\file")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("path\\to\\file")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 17, utf8::view(""));
	}

	SECTION("character and string adjacent without space")
	{
		utf8::string       str = R"('c'"string")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[1], TokenKind::String, 1, 4, utf8::view(R"("string")"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 12, utf8::view(""));
	}

	SECTION("string and character adjacent without space")
	{
		utf8::string       str = R"("string"'c')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("string")"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Character, 1, 9, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 12, utf8::view(""));
	}

	SECTION("hex escape with non-hex adjacent")
	{
		utf8::string       str = R"("\x0g")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view(R"("\x0g")"), TokenError::None);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 7, utf8::view(""));
	}

	SECTION("hex escape in character with non-hex adjacent")
	{
		utf8::string       str = R"('\x0Z')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Character, 1, 1, utf8::view(R"('\x0Z')"), TokenError::None);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 7, utf8::view(""));
	}

	SECTION("carriage return in string")
	{
		utf8::string       str = "\"line1\rline2\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"line1"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::Identifier, 2, 1, utf8::view("line2"));
		CHECK_TOKEN(str, tokens[2], TokenKind::String, 2, 6, utf8::view("\""), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[3], TokenKind::EOF, 2, 7, utf8::view(""));
	}

	SECTION("mixed line endings in string")
	{
		utf8::string       str = "\"line1\r\nline2\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenKind::String, 1, 1, utf8::view("\"line1"), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[1], TokenKind::Identifier, 2, 1, utf8::view("line2"));
		CHECK_TOKEN(str, tokens[2], TokenKind::String, 2, 6, utf8::view("\""), TokenError::InvalidString);
		CHECK_TOKEN(str, tokens[3], TokenKind::EOF, 2, 7, utf8::view(""));
	}

	//

	SECTION("basic symbols")
	{
		utf8::string       str = "+ - * /"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::Plus, 1, 1, utf8::view("+"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Minus, 1, 3, utf8::view("-"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Star, 1, 5, utf8::view("*"));
		CHECK_TOKEN(str, tokens[3], TokenKind::Slash, 1, 7, utf8::view("/"));
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 8, utf8::view(""));
	}

	SECTION("basic symbols++")
	{
		utf8::string       str = "++ --"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenKind::PlusPlus, 1, 1, utf8::view("++"));
		CHECK_TOKEN(str, tokens[1], TokenKind::MinusMinus, 1, 4, utf8::view("--"));
		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 6, utf8::view(""));
	}

	SECTION("slash")
	{
		utf8::string str = R"(/ // /* */ \ \\ /=)"sv;

		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 8);
		CHECK_TOKEN(str, tokens[0], TokenKind::Slash, 1, 1, utf8::view("/"));
		CHECK_TOKEN(str, tokens[1], TokenKind::SlashSlash, 1, 3, utf8::view("//"));
		CHECK_TOKEN(str, tokens[2], TokenKind::SlashStar, 1, 6, utf8::view("/*"));
		CHECK_TOKEN(str, tokens[3], TokenKind::StarSlash, 1, 9, utf8::view("*/"));
		CHECK_TOKEN(str, tokens[4], TokenKind::BackSlash, 1, 12, utf8::view("\\"));
		CHECK_TOKEN(str, tokens[5], TokenKind::BackSlashBackSlash, 1, 14, utf8::view("\\\\"));
		CHECK_TOKEN(str, tokens[6], TokenKind::SlashEqual, 1, 17, utf8::view("/="));
		CHECK_TOKEN(str, tokens[7], TokenKind::EOF, 1, 19, utf8::view(""));
	}

	SECTION("shifts")
	{
		utf8::string       str = "<< >> <<= >>="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::ShiftLeft, 1, 1, utf8::view("<<"));
		CHECK_TOKEN(str, tokens[1], TokenKind::ShiftRight, 1, 4, utf8::view(">>"));
		CHECK_TOKEN(str, tokens[2], TokenKind::ShiftLeftEqual, 1, 7, utf8::view("<<="));
		CHECK_TOKEN(str, tokens[3], TokenKind::ShiftRightEqual, 1, 11, utf8::view(">>="));
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 14, utf8::view(""));
	}

	SECTION("compares")
	{
		utf8::string       str = "< > <= >= =="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 6);
		CHECK_TOKEN(str, tokens[0], TokenKind::LessThan, 1, 1, utf8::view("<"));
		CHECK_TOKEN(str, tokens[1], TokenKind::GreaterThan, 1, 3, utf8::view(">"));
		CHECK_TOKEN(str, tokens[2], TokenKind::LessThanEqual, 1, 5, utf8::view("<="));
		CHECK_TOKEN(str, tokens[3], TokenKind::GreaterThanEqual, 1, 8, utf8::view(">="));
		CHECK_TOKEN(str, tokens[4], TokenKind::EqualEqual, 1, 11, utf8::view("=="));
		CHECK_TOKEN(str, tokens[5], TokenKind::EOF, 1, 13, utf8::view(""));
	}


	SECTION("extended symbols")
	{
		utf8::string       str = "%?!@"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenKind::Percent, 1, 1, utf8::view("%"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Question, 1, 2, utf8::view("?"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Bang, 1, 3, utf8::view("!"));
		CHECK_TOKEN(str, tokens[3], TokenKind::At, 1, 4, utf8::view("@"));
		CHECK_TOKEN(str, tokens[4], TokenKind::EOF, 1, 5, utf8::view(""));
	}

	SECTION("brackets and braces")
	{
		utf8::string       str = "{}()[]<>"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 9);
		CHECK_TOKEN(str, tokens[0], TokenKind::LeftBrace, 1, 1, utf8::view("{"));
		CHECK_TOKEN(str, tokens[1], TokenKind::RightBrace, 1, 2, utf8::view("}"));
		CHECK_TOKEN(str, tokens[2], TokenKind::LeftParen, 1, 3, utf8::view("("));
		CHECK_TOKEN(str, tokens[3], TokenKind::RightParen, 1, 4, utf8::view(")"));
		CHECK_TOKEN(str, tokens[4], TokenKind::LeftBracket, 1, 5, utf8::view("["));
		CHECK_TOKEN(str, tokens[5], TokenKind::RightBracket, 1, 6, utf8::view("]"));

		CHECK_TOKEN(str, tokens[6], TokenKind::LessThan, 1, 7, utf8::view("<"));
		CHECK_TOKEN(str, tokens[7], TokenKind::GreaterThan, 1, 8, utf8::view(">"));
		CHECK_TOKEN(str, tokens[8], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	SECTION("symbols")
	{
		utf8::string       str = "="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Equal, 1, 1, utf8::view("="));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 2, utf8::view(""));
	}

	SECTION("blank equals")
	{
		utf8::string       str = "!= += -= *= /= %= =="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 8);
		CHECK_TOKEN(str, tokens[0], TokenKind::BangEqual, 1, 1, utf8::view("!="));
		CHECK_TOKEN(str, tokens[1], TokenKind::PlusEqual, 1, 4, utf8::view("+="));
		CHECK_TOKEN(str, tokens[2], TokenKind::MinusEqual, 1, 7, utf8::view("-="));
		CHECK_TOKEN(str, tokens[3], TokenKind::StarEqual, 1, 10, utf8::view("*="));
		CHECK_TOKEN(str, tokens[4], TokenKind::SlashEqual, 1, 13, utf8::view("/="));
		CHECK_TOKEN(str, tokens[5], TokenKind::PercentEqual, 1, 16, utf8::view("%="));
		CHECK_TOKEN(str, tokens[6], TokenKind::EqualEqual, 1, 19, utf8::view("=="));
		CHECK_TOKEN(str, tokens[7], TokenKind::EOF, 1, 21, utf8::view(""));
	}

	SECTION("dots")
	{
		utf8::string       str = ". .. ..."sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenKind::Dot, 1, 1, utf8::view("."));
		CHECK_TOKEN(str, tokens[1], TokenKind::DotDot, 1, 3, utf8::view(".."));
		CHECK_TOKEN(str, tokens[2], TokenKind::Ellipsis, 1, 6, utf8::view("..."));
		CHECK_TOKEN(str, tokens[3], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	SECTION("extra symbols")
	{
		utf8::string       str = "# -> ; : ,"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 6);
		CHECK_TOKEN(str, tokens[0], TokenKind::Hash, 1, 1, utf8::view("#"));
		CHECK_TOKEN(str, tokens[1], TokenKind::Arrow, 1, 3, utf8::view("->"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Semicolon, 1, 6, utf8::view(";"));
		CHECK_TOKEN(str, tokens[3], TokenKind::Colon, 1, 8, utf8::view(":"));
		CHECK_TOKEN(str, tokens[4], TokenKind::Comma, 1, 10, utf8::view(","));
		CHECK_TOKEN(str, tokens[5], TokenKind::EOF, 1, 11, utf8::view(""));
	}

	SECTION("bit operators")
	{
		utf8::string       str = "| & ^ ~ || &&"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 7);
		CHECK_TOKEN(str, tokens[0], TokenKind::Or, 1, 1, utf8::view("|"));
		CHECK_TOKEN(str, tokens[1], TokenKind::And, 1, 3, utf8::view("&"));
		CHECK_TOKEN(str, tokens[2], TokenKind::Xor, 1, 5, utf8::view("^"));
		CHECK_TOKEN(str, tokens[3], TokenKind::Tilde, 1, 7, utf8::view("~"));
		CHECK_TOKEN(str, tokens[4], TokenKind::OrOr, 1, 9, utf8::view("||"));
		CHECK_TOKEN(str, tokens[5], TokenKind::AndAnd, 1, 12, utf8::view("&&"));
		CHECK_TOKEN(str, tokens[6], TokenKind::EOF, 1, 14, utf8::view(""));
	}

	SECTION("bit equal operators")
	{
		utf8::string       str = "|= &= ^="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenKind::OrEqual, 1, 1, utf8::view("|="));
		CHECK_TOKEN(str, tokens[1], TokenKind::AndEqual, 1, 4, utf8::view("&="));
		CHECK_TOKEN(str, tokens[2], TokenKind::XorEqual, 1, 7, utf8::view("^="));
		CHECK_TOKEN(str, tokens[3], TokenKind::EOF, 1, 9, utf8::view(""));
	}

	SECTION("pseudo code test")
	{
		utf8::string   script = R"(
fn calc(a: i32, b: i32) -> i32 { # this is a comment
    return a*2; # this is a comment
}
)"sv;
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		};
		std::vector<Token> tokens;
		for (const auto& token : tokenize(script, cfg))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() == 21);

		u32 idx = 0;
		// line 2
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 1, utf8::view("fn"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 4, utf8::view("calc"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::LeftParen, 2, 8, utf8::view("("));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 9, utf8::view("a"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Colon, 2, 10, utf8::view(":"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 12, utf8::view("i32"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Comma, 2, 15, utf8::view(","));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 17, utf8::view("b"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Colon, 2, 18, utf8::view(":"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 20, utf8::view("i32"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::RightParen, 2, 23, utf8::view(")"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Arrow, 2, 25, utf8::view("->"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 2, 28, utf8::view("i32"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::LeftBrace, 2, 32, utf8::view("{"));

		// line 3
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 3, 5, utf8::view("return"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Identifier, 3, 12, utf8::view("a"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Star, 3, 13, utf8::view("*"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Integer, 3, 14, utf8::view("2"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::Semicolon, 3, 15, utf8::view(";"));

		// line 4
		CHECK_TOKEN(script, tokens[idx++], TokenKind::RightBrace, 4, 1, utf8::view("}"));
		CHECK_TOKEN(script, tokens[idx++], TokenKind::EOF, 5, 1, utf8::view(""));
	}


	SECTION("comment //")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "//"sv,
		};

		utf8::string       str = "a // comment"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("a"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 13, utf8::view(""));
	}

	SECTION("comment #")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		};

		utf8::string       str = "a # comment"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("a"));
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 12, utf8::view(""));
	}

	SECTION("invalid line comment")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		  .block_comment_start       = "#"sv,
		  .block_comment_end         = "#"sv,
		};


		std::vector<Token> tokens;
		for (const auto& token : tokenize("a"sv, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK(tokens[0].error == TokenError::InvalidLineComment);
	}

	SECTION("invalid comment blocks")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "//"sv,
		  .block_comment_start       = "#"sv,
		  .block_comment_end         = "#"sv,
		};


		std::vector<Token> tokens;
		for (const auto& token : tokenize("a"sv, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK(tokens[0].error == TokenError::InvalidCommentBlock);
	}

	SECTION("single comment block")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		  .block_comment_start       = "/*"sv,
		  .block_comment_end         = "*/"sv,
		};

		utf8::string       script = R"(
a /*
    block comment
   with multiple lines
*/ b # comment
c
)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(script, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(script, tokens[0], TokenKind::Identifier, 2, 1, utf8::view("a"));
		CHECK_TOKEN(script, tokens[1], TokenKind::Identifier, 5, 4, utf8::view("b"));
		CHECK_TOKEN(script, tokens[2], TokenKind::Identifier, 6, 1, utf8::view("c"));
		CHECK_TOKEN(script, tokens[3], TokenKind::EOF, 7, 1, utf8::view(""));
	}


	SECTION("unclosed comment block")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		  .block_comment_start       = "/*"sv,
		  .block_comment_end         = "*/"sv,
		};

		utf8::string       script = R"(
a /*
c
)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(script, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(script, tokens[0], TokenKind::Identifier, 2, 1, utf8::view("a"));
		CHECK_TOKEN(script, tokens[1], TokenKind::Comment, 2, 3, utf8::view("/*"), TokenError::UnclosedCommentBlock);
		CHECK_TOKEN(script, tokens[2], TokenKind::EOF, 4, 1, utf8::view(""));
	}

	SECTION("nested comment block")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		  .block_comment_start       = "/*"sv,
		  .block_comment_end         = "*/"sv,
		};

		utf8::string       script = R"(
a /*
  block comment
  with multiple lines
/* nested comment */
*/ b # comment
/*

*/
  c
*/
)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(script, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(script, tokens[0], TokenKind::Identifier, 2, 1, utf8::view("a"));
		CHECK_TOKEN(script, tokens[1], TokenKind::Identifier, 6, 4, utf8::view("b"));
		CHECK_TOKEN(script, tokens[2], TokenKind::Identifier, 10, 3, utf8::view("c"));
		CHECK_TOKEN(script, tokens[3], TokenKind::Comment, 11, 1, utf8::view("*/"), TokenError::UnopenedCommentBlock);
		CHECK_TOKEN(script, tokens[4], TokenKind::EOF, 12, 1, utf8::view(""));
	}

	SECTION("closing comment block without opening")
	{
		TokenizeConfig cfg{
		  .single_line_comment_start = "#"sv,
		  .block_comment_start       = "/*"sv,
		  .block_comment_end         = "*/"sv,
		};

		utf8::string       script = R"(a */ b)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(script, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(script, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("a"));
		CHECK_TOKEN(script, tokens[1], TokenKind::Comment, 1, 3, utf8::view("*/"), TokenError::UnopenedCommentBlock);
		CHECK_TOKEN(script, tokens[2], TokenKind::Identifier, 1, 6, utf8::view("b"));
		CHECK_TOKEN(script, tokens[3], TokenKind::EOF, 1, 7, utf8::view(""));
	}

	SECTION("ZWJ identifier - admin reject")
	{
		utf8::string admin = "admin a\u200Ddmin"sv; // "admin a<ZWJ>dmin"

		std::vector<Token> tokens;
		for (const auto& token : tokenize(admin))
			tokens.emplace_back(token);
		CHECK(tokens.size() == 3);

		CHECK_TOKEN(admin, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("admin"));
		CHECK_TOKEN(
		  admin,
		  tokens[1],
		  TokenKind::Identifier,
		  1,
		  7,
		  utf8::view("a\u200Ddmin"),
		  TokenError::InvalidIdentifierZeroWidthJoiner);
		CHECK_TOKEN(admin, tokens[2], TokenKind::EOF, 1, 13, utf8::view(""));
	}

	SECTION("identifier with zero-width joiner")
	{
		utf8::string       str = "id\u200Dent id\u200Cent"sv; // id<ZWJ>ent and id<ZWNJ>ent
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(
		  str,
		  tokens[0],
		  TokenKind::Identifier,
		  1,
		  1,
		  utf8::view("id\u200Dent"),
		  TokenError::InvalidIdentifierZeroWidthJoiner);

		CHECK_TOKEN(
		  str,
		  tokens[1],
		  TokenKind::Identifier,
		  1,
		  8,
		  utf8::view("id\u200Cent"),
		  TokenError::InvalidIdentifierZeroWidthJoiner);

		CHECK_TOKEN(str, tokens[2], TokenKind::EOF, 1, 14, utf8::view(""));
	}

	SECTION("identifier with non-ASCII forbidden")
	{
		TokenizeConfig cfg{
		  .forbid_non_ascii_in_identifiers = true,
		};

		utf8::string       str = "π"sv; // Greek letter pi (non-ASCII)
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str, cfg))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("π"), TokenError::InvalidIdentifierNonASCII);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 2, utf8::view(""));
	}

	SECTION("identifier with non-ASCII allowed (default)")
	{
		utf8::string       str = "π"sv;         // Greek letter pi (non-ASCII)
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str)) // forbid_non_ascii_in_identifiers = false by default
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenKind::Identifier, 1, 1, utf8::view("π"), TokenError::None);
		CHECK_TOKEN(str, tokens[1], TokenKind::EOF, 1, 2, utf8::view(""));
	}

	// ###################################################################################################################


	SECTION("unique tokens tested")
	{
		// skip unknown and keyword
		details::token_counts[TokenKind::Unknown]++;
		details::token_counts[TokenKind::Keyword]++;

		// detail::pool.dump();

		CHECK(details::token_counts.size() == static_cast<u32>(TokenKind::TokenCount));
	}
}
