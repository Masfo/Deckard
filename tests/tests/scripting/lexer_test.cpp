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
	std::unordered_map<TokenType, u32> token_counts;
}

void CHECK_TOKEN(const utf8::view str, const Token& token, TokenType expected_type, u32 expected_line, u32 expected_column,
				 utf8::view correct_str)
{
	detail::token_counts[token.type]++;

	CHECK(token.type == expected_type);
	CHECK(token.line == expected_line);
	CHECK(token.column == expected_column);
	CHECK(token.length == correct_str.length());
	CHECK(extract_token(str, token) == correct_str);
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
		utf8::string       str = "123"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, utf8::view("123"));
	}

	SECTION("floating point")
	{
		utf8::string       str = "3.1415"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, utf8::view("3.1415"));
	}

	SECTION("character") 
	{
		utf8::string       str = R"('c' '🌍')"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 2);
		CHECK_TOKEN(str, tokens[0], TokenType::Character, 1, 1, utf8::view(R"('c')"));
		CHECK_TOKEN(str, tokens[1], TokenType::Character, 1, 5, utf8::view(R"('🌍')"));

	}

	SECTION("string")
	{
		utf8::string       str = R"("hello")"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::String, 1, 1, utf8::view(R"("hello")"));
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
	SECTION("unique tokens tested")
	{
		// -3 for Newline, EOF, Unknown
		// -2 for aliased tokens RightAngleBracket, LeftAngleBracket
		// -1 for Dollar
		constexpr u32 expected_token_count = static_cast<u32>(TokenType::TokenCount) - 3 - 2 - 1;

		CHECK(detail::token_counts.size() == expected_token_count);
	}
}
