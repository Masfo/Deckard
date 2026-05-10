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

namespace detail
{
	std::unordered_set<TokenType> token_counts;
}

void CHECK_TOKEN(const utf8::view str, const Token& token, TokenType expected_type, u32 expected_line, u32 expected_column,
				 utf8::view correct_str, TokenError expected_error = TokenError::None)
{
	detail::token_counts.insert(token.type);

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

	SECTION("stringview initialize")
	{
		// line 1: abc\n
		// line 2: qwerty\r\n
		// line 3: hjkl
		utf8::string       str = "abc\nqwerty\r\nhjkl"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);

		CHECK_TOKEN(str, tokens[0], TokenType::Identifier, 1, 1, utf8::view("abc"));
		CHECK_TOKEN(str, tokens[1], TokenType::Identifier, 2, 1, utf8::view("qwerty"));
		CHECK_TOKEN(str, tokens[2], TokenType::Identifier, 3, 1, utf8::view("hjkl"));
	}

	SECTION("integer")
	{
		utf8::string       str = "123 456_789"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("123"));
		CHECK_TOKEN(str, tokens[1], TokenType::Integer, 1, 5, utf8::view("456_789"));
	}

	SECTION("invalid integer")
	{
		utf8::string       str = "666a"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str) | std::views::take(1))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("666a"), TokenError::InvalidInteger);
	}

	SECTION("hex integer")
	{
		utf8::string       str = "0x29A 0xbad"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("0x29A"));
		CHECK_TOKEN(str, tokens[1], TokenType::Integer, 1, 7, utf8::view("0xbad"));
	}

	SECTION("invalid hex integer")
	{
		utf8::string       str = "0xG"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str) | std::views::take(1))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("0xG"), TokenError::InvalidHex);
	}


	SECTION("binary integer")
	{
		utf8::string       str = "0b101010"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("0b101010"));
	}

	SECTION("invalid binary integer")
	{
		utf8::string       str = "0b2"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str) | std::views::take(1))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("0b2"), TokenError::InvalidBinary);
	}


	SECTION("floating point")
	{
		utf8::string       str = "3.1415 1."sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, utf8::view("3.1415"));
		CHECK_TOKEN(str, tokens[1], TokenType::FloatingPoint, 1, 8, utf8::view("1."));
	}

	SECTION("invalid floating point")
	{
		utf8::string       str = "1.2.3 1.a"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, utf8::view("1.2.3"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[1], TokenType::FloatingPoint, 1, 7, utf8::view("1.a"), TokenError::InvalidFloatingPoint);
	}

	SECTION("floating point with exponent")
	{
		utf8::string       str = "1e5 3.14e10 1.e-3 2.5e+7 1_000.5e-2"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, utf8::view("1e5"));
		CHECK_TOKEN(str, tokens[1], TokenType::FloatingPoint, 1, 5, utf8::view("3.14e10"));
		CHECK_TOKEN(str, tokens[2], TokenType::FloatingPoint, 1, 13, utf8::view("1.e-3"));
		CHECK_TOKEN(str, tokens[3], TokenType::FloatingPoint, 1, 19, utf8::view("2.5e+7"));
		CHECK_TOKEN(str, tokens[4], TokenType::FloatingPoint, 1, 26, utf8::view("1_000.5e-2"));
	}

	SECTION("invalid floating point with exponent")
	{
		utf8::string       str = "1e 1e+ 1e- 1ea"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, utf8::view("1e"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[1], TokenType::FloatingPoint, 1, 4, utf8::view("1e+"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[2], TokenType::FloatingPoint, 1, 8, utf8::view("1e-"), TokenError::InvalidFloatingPoint);
		CHECK_TOKEN(str, tokens[3], TokenType::FloatingPoint, 1, 12, utf8::view("1ea"), TokenError::InvalidFloatingPoint);
	}

	SECTION("floating point comprehensive")
	{
		utf8::string       str = "1.5e-10 5. 5.5e1 1_2_3.4_5e-6_7"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, utf8::view("1.5e-10"));
		CHECK_TOKEN(str, tokens[1], TokenType::FloatingPoint, 1, 9, utf8::view("5."));
		CHECK_TOKEN(str, tokens[2], TokenType::FloatingPoint, 1, 12, utf8::view("5.5e1"));
		CHECK_TOKEN(str, tokens[3], TokenType::FloatingPoint, 1, 18, utf8::view("1_2_3.4_5e-6_7"));
	}

	SECTION("character")
	{
		utf8::string       str = R"('c' '🌍' '\'' '\xff' '\n')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[1], TokenType::Character, 1, 5, utf8::view(R"('🌍')"));
		CHECK_TOKEN(str, tokens[2], TokenType::Character, 1, 9, utf8::view(R"('\'')"));
		CHECK_TOKEN(str, tokens[3], TokenType::Character, 1, 14, utf8::view(R"('\xff')"));
		CHECK_TOKEN(str, tokens[4], TokenType::Character, 1, 21, utf8::view(R"('\n')"));
	}

	SECTION("invalid character (unterminated)")
	{
		utf8::string       str = "'x"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view("'x"), TokenError::InvalidCharacter);
	}

	SECTION("invalid character")
	{
		utf8::string       str = R"('\xfff')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('\xfff')"), TokenError::InvalidCharacter);
	}


	SECTION("string")
	{
		utf8::string       str = R"("hello" "hel\nlo")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("hello")"));
		CHECK_TOKEN(str, tokens[1], TokenType::String, 1, 9, utf8::view(R"("hel\nlo")"));
	}

	SECTION("invalid string (unterminated)")
	{
		utf8::string       str = "\"invalid"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view("\"invalid"), TokenError::InvalidString);
	}

	SECTION("escaped string - basic")
	{
		utf8::string       str = R"("\n" "\r" "\t")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\n")"));
		CHECK_TOKEN(str, tokens[1], TokenType::String, 1, 6, utf8::view(R"("\r")"));
		CHECK_TOKEN(str, tokens[2], TokenType::String, 1, 11, utf8::view(R"("\t")"));
	}

	SECTION("escaped string - hex")
	{
		utf8::string       str = R"("\xFF" "\xB")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\xFF")"));
		CHECK_TOKEN(str, tokens[1], TokenType::String, 1, 8, utf8::view(R"("\xB")"));
	}

	SECTION("escaped string - invalid hex (invalid char)")
	{
		utf8::string       str = R"("\xG")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\xG")"), TokenError::InvalidString);
	}

	SECTION("escaped string - invalid hex (too long)")
	{
		utf8::string       str = R"("\xAABB")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\xAABB")"), TokenError::InvalidString);
	}

	SECTION("escaped string - invalid hex (missing digits)")
	{
		utf8::string       str = R"("\x")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\x")"), TokenError::InvalidString);
	}

	SECTION("string with newline")
	{
		utf8::string       str = "\"first line\nsecond line\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() >= 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view("\"first line"), TokenError::InvalidString);
	}

	SECTION("string with trailing backslash")
	{
		utf8::string       str = R"("trailing \)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("trailing \)"), TokenError::InvalidString);
	}

	SECTION("string with escaped newline")
	{
		utf8::string       str = "\"escaped \\\nnewline\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view("\"escaped \\\nnewline\""));
	}

	SECTION("unicode escape string")
	{
		utf8::string       str = R"("\u41" "\U0041" "\uFFFDzxc" "\U000012345")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\u41")"));
		CHECK_TOKEN(str, tokens[1], TokenType::String, 1, 8, utf8::view(R"("\U0041")"));
		CHECK_TOKEN(str, tokens[2], TokenType::String, 1, 17, utf8::view(R"("\uFFFDzxc")"));
		CHECK_TOKEN(str, tokens[3], TokenType::String, 1, 29, utf8::view(R"("\U000012345")"));
	}

	SECTION("hex escape case sensitivity")
	{
		utf8::string       str = R"("\xAb" "\xaB" "\xab" "\xAB")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\xAb")"));
		CHECK_TOKEN(str, tokens[1], TokenType::String, 1, 8, utf8::view(R"("\xaB")"));
		CHECK_TOKEN(str, tokens[2], TokenType::String, 1, 15, utf8::view(R"("\xab")"));
		CHECK_TOKEN(str, tokens[3], TokenType::String, 1, 22, utf8::view(R"("\xAB")"));
	}

	SECTION("empty character literal")
	{
		utf8::string       str = R"('')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('')"), TokenError::None);
	}

	SECTION("character with multiple codepoints")
	{
		utf8::string       str = R"('ab')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('ab')"));
	}

	SECTION("character with multiple hex escapes")
	{
		utf8::string       str = R"('\x41\x42')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('\x41\x42')"));
	}

	SECTION("character hex escape boundary - incomplete at end")
	{
		utf8::string       str = R"('\x)"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('\x)"), TokenError::InvalidCharacter);
	}

	SECTION("character with escaped quote")
	{
		utf8::string       str = R"('\'')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('\'')"));
	}

	SECTION("character with escaped backslash")
	{
		utf8::string       str = R"('\\')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('\\')"));
	}

	SECTION("string with escaped quote")
	{
		utf8::string       str = R"("hello\"world")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("hello\"world")"));
	}

	SECTION("string with escaped backslash")
	{
		utf8::string       str = R"("path\\to\\file")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("path\\to\\file")"));
	}

	SECTION("character and string adjacent without space")
	{
		utf8::string       str = R"('c'"string")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[1], TokenType::String, 1, 4, utf8::view(R"("string")"));
	}

	SECTION("string and character adjacent without space")
	{
		utf8::string       str = R"("string"'c')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("string")"));
		CHECK_TOKEN(str, tokens[1], TokenType::Character, 1, 9, utf8::view(R"('c')"));
	}

	SECTION("hex escape with non-hex adjacent")
	{
		utf8::string       str = R"("\x0g")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("\x0g")"), TokenError::None);
	}

	SECTION("hex escape in character with non-hex adjacent")
	{
		utf8::string       str = R"('\x0Z')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('\x0Z')"), TokenError::None);
	}

	SECTION("carriage return in string")
	{
		utf8::string       str = "\"line1\rline2\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() >= 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view("\"line1"), TokenError::InvalidString);
	}

	SECTION("mixed line endings in string")
	{
		utf8::string       str = "\"line1\r\nline2\""sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() >= 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view("\"line1"), TokenError::InvalidString);
	}

	//

	SECTION("basic symbols")
	{
		utf8::string       str = "+ - * /"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::Plus, 1, 1, utf8::view("+"));
		CHECK_TOKEN(str, tokens[1], TokenType::Minus, 1, 3, utf8::view("-"));
		CHECK_TOKEN(str, tokens[2], TokenType::Star, 1, 5, utf8::view("*"));
		CHECK_TOKEN(str, tokens[3], TokenType::Slash, 1, 7, utf8::view("/"));
	}

	SECTION("basic symbols++")
	{
		utf8::string       str = "++ --"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::PlusPlus, 1, 1, utf8::view("++"));
		CHECK_TOKEN(str, tokens[1], TokenType::MinusMinus, 1, 4, utf8::view("--"));
	}

	SECTION("slash")
	{
		utf8::string str = R"(/ // /* */ \ \\ /=)"sv;

		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 7);
		CHECK_TOKEN(str, tokens[0], TokenType::Slash, 1, 1, utf8::view("/"));
		CHECK_TOKEN(str, tokens[1], TokenType::SlashSlash, 1, 3, utf8::view("//"));
		CHECK_TOKEN(str, tokens[2], TokenType::SlashStar, 1, 6, utf8::view("/*"));
		CHECK_TOKEN(str, tokens[3], TokenType::StarSlash, 1, 9, utf8::view("*/"));
		CHECK_TOKEN(str, tokens[4], TokenType::BackSlash, 1, 12, utf8::view("\\"));
		CHECK_TOKEN(str, tokens[5], TokenType::BackSlashBackSlash, 1, 14, utf8::view("\\\\"));
		CHECK_TOKEN(str, tokens[6], TokenType::SlashEqual, 1, 17, utf8::view("/="));
	}

	SECTION("shifts")
	{
		utf8::string       str = "<< >> <<= >>="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::ShiftLeft, 1, 1, utf8::view("<<"));
		CHECK_TOKEN(str, tokens[1], TokenType::ShiftRight, 1, 4, utf8::view(">>"));
		CHECK_TOKEN(str, tokens[2], TokenType::ShiftLeftEqual, 1, 7, utf8::view("<<="));
		CHECK_TOKEN(str, tokens[3], TokenType::ShiftRightEqual, 1, 11, utf8::view(">>="));
	}

	SECTION("compares")
	{
		utf8::string       str = "< > <= >= =="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenType::LessThan, 1, 1, utf8::view("<"));
		CHECK_TOKEN(str, tokens[1], TokenType::GreaterThan, 1, 3, utf8::view(">"));
		CHECK_TOKEN(str, tokens[2], TokenType::LessThanEqual, 1, 5, utf8::view("<="));
		CHECK_TOKEN(str, tokens[3], TokenType::GreaterThanEqual, 1, 8, utf8::view(">="));
		CHECK_TOKEN(str, tokens[4], TokenType::EqualEqual, 1, 11, utf8::view("=="));
	}


	SECTION("extended symbols")
	{
		utf8::string       str = "%?!@"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 4);
		CHECK_TOKEN(str, tokens[0], TokenType::Percent, 1, 1, utf8::view("%"));
		CHECK_TOKEN(str, tokens[1], TokenType::Question, 1, 2, utf8::view("?"));
		CHECK_TOKEN(str, tokens[2], TokenType::Bang, 1, 3, utf8::view("!"));
		CHECK_TOKEN(str, tokens[3], TokenType::At, 1, 4, utf8::view("@"));
	}

	SECTION("brackets and braces")
	{
		utf8::string       str = "{}()[]<>"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 8);
		CHECK_TOKEN(str, tokens[0], TokenType::LeftBrace, 1, 1, utf8::view("{"));
		CHECK_TOKEN(str, tokens[1], TokenType::RightBrace, 1, 2, utf8::view("}"));
		CHECK_TOKEN(str, tokens[2], TokenType::LeftParen, 1, 3, utf8::view("("));
		CHECK_TOKEN(str, tokens[3], TokenType::RightParen, 1, 4, utf8::view(")"));
		CHECK_TOKEN(str, tokens[4], TokenType::LeftBracket, 1, 5, utf8::view("["));
		CHECK_TOKEN(str, tokens[5], TokenType::RightBracket, 1, 6, utf8::view("]"));

		CHECK_TOKEN(str, tokens[6], TokenType::LeftAngleBracket, 1, 7, utf8::view("<"));
		CHECK_TOKEN(str, tokens[7], TokenType::RightAngleBracket, 1, 8, utf8::view(">"));

		CHECK_TOKEN(str, tokens[6], TokenType::LessThan, 1, 7, utf8::view("<"));
		CHECK_TOKEN(str, tokens[7], TokenType::GreaterThan, 1, 8, utf8::view(">"));
	}

	SECTION("symbols")
	{
		utf8::string       str = "="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Equal, 1, 1, utf8::view("="));
	}

	SECTION("blank equals")
	{
		utf8::string       str = "!= += -= *= /= %= =="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 7);
		CHECK_TOKEN(str, tokens[0], TokenType::BangEqual, 1, 1, utf8::view("!="));
		CHECK_TOKEN(str, tokens[1], TokenType::PlusEqual, 1, 4, utf8::view("+="));
		CHECK_TOKEN(str, tokens[2], TokenType::MinusEqual, 1, 7, utf8::view("-="));
		CHECK_TOKEN(str, tokens[3], TokenType::StarEqual, 1, 10, utf8::view("*="));
		CHECK_TOKEN(str, tokens[4], TokenType::SlashEqual, 1, 13, utf8::view("/="));
		CHECK_TOKEN(str, tokens[5], TokenType::PercentEqual, 1, 16, utf8::view("%="));
		CHECK_TOKEN(str, tokens[6], TokenType::EqualEqual, 1, 19, utf8::view("=="));
	}

	SECTION("dots")
	{
		utf8::string       str = ". .. ..."sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenType::Dot, 1, 1, utf8::view("."));
		CHECK_TOKEN(str, tokens[1], TokenType::DotDot, 1, 3, utf8::view(".."));
		CHECK_TOKEN(str, tokens[2], TokenType::Ellipsis, 1, 6, utf8::view("..."));
	}

	SECTION("extra symbols")
	{
		utf8::string       str = "# -> ; : ,"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 5);
		CHECK_TOKEN(str, tokens[0], TokenType::Hash, 1, 1, utf8::view("#"));
		CHECK_TOKEN(str, tokens[1], TokenType::Arrow, 1, 3, utf8::view("->"));
		CHECK_TOKEN(str, tokens[2], TokenType::Semicolon, 1, 6, utf8::view(";"));
		CHECK_TOKEN(str, tokens[3], TokenType::Colon, 1, 8, utf8::view(":"));
		CHECK_TOKEN(str, tokens[4], TokenType::Comma, 1, 10, utf8::view(","));
	}

	SECTION("bit operators")
	{
		utf8::string       str = "| & ^ ~ || &&"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 6);
		CHECK_TOKEN(str, tokens[0], TokenType::Or, 1, 1, utf8::view("|"));
		CHECK_TOKEN(str, tokens[1], TokenType::And, 1, 3, utf8::view("&"));
		CHECK_TOKEN(str, tokens[2], TokenType::Xor, 1, 5, utf8::view("^"));
		CHECK_TOKEN(str, tokens[3], TokenType::Tilde, 1, 7, utf8::view("~"));
		CHECK_TOKEN(str, tokens[4], TokenType::OrOr, 1, 9, utf8::view("||"));
		CHECK_TOKEN(str, tokens[5], TokenType::AndAnd, 1, 12, utf8::view("&&"));
	}

	SECTION("bit equal operators")
	{
		utf8::string       str = "|= &= ^="sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 3);
		CHECK_TOKEN(str, tokens[0], TokenType::OrEqual, 1, 1, utf8::view("|="));
		CHECK_TOKEN(str, tokens[1], TokenType::AndEqual, 1, 4, utf8::view("&="));
		CHECK_TOKEN(str, tokens[2], TokenType::XorEqual, 1, 7, utf8::view("^="));
		// CHECK_TOKEN(str, tokens[3], TokenType::QuestionEqual, 1, 9, utf8::view("?="));
	}

	SECTION("pseudo code test")
	{
		utf8::string script = R"(
fn calc(a: i32, b: i32) -> i32 {
    return a*2;
}
)"sv;

		std::vector<Token> tokens;
		for (const auto& token : tokenize(script))
			tokens.emplace_back(token);

		REQUIRE(tokens.size() == 20);

		u32 idx = 0;
		// line 2
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 1, utf8::view("fn"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 4, utf8::view("calc"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::LeftParen, 2, 8, utf8::view("("));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 9, utf8::view("a"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Colon, 2, 10, utf8::view(":"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 12, utf8::view("i32"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Comma, 2, 15, utf8::view(","));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 17, utf8::view("b"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Colon, 2, 18, utf8::view(":"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 20, utf8::view("i32"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::RightParen, 2, 23, utf8::view(")"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Arrow, 2, 25, utf8::view("->"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 2, 28, utf8::view("i32"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::LeftBrace, 2, 32, utf8::view("{"));

		// line 3
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 3, 5, utf8::view("return"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Identifier, 3, 12, utf8::view("a"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Star, 3, 13, utf8::view("*"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Integer, 3, 14, utf8::view("2"));
		CHECK_TOKEN(script, tokens[idx++], TokenType::Semicolon, 3, 15, utf8::view(";"));

		// line 4
		CHECK_TOKEN(script, tokens[idx++], TokenType::RightBrace, 4, 1, utf8::view("}"));
	}

	// ###################################################################################################################

	SECTION("unique tokens tested")
	{
		// -3 for Newline, EOF, Unknown
		// -2 for aliased tokens RightAngleBracket, LeftAngleBracket
		// -1 for keywords (not tested here)
		constexpr u32 expected_token_count = static_cast<u32>(TokenType::TokenCount) - 3 - 2 - 1;

		CHECK(detail::token_counts.size() == expected_token_count);
	}
}
