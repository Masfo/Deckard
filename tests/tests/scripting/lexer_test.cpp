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

void CHECK_TOKEN(const utf8::view str, const Token& token, TokenType expected_type, u32 expected_line, u32 expected_column, u32 expected_length, utf8::view correct_str)
{
	CHECK(token.type == expected_type);
	CHECK(token.line == expected_line);
	CHECK(token.column == expected_column);
	CHECK(token.length == expected_length);
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

		CHECK_TOKEN(str, tokens[0], TokenType::Identifier, 1, 1, 3, utf8::view("abc"));
		CHECK_TOKEN(str, tokens[1], TokenType::Identifier, 2, 1, 6, utf8::view("qwerty"));
		CHECK_TOKEN(str, tokens[2], TokenType::Identifier, 3, 1, 4, utf8::view("hjkl"));
	}

	SECTION("integer")
	{
		utf8::string       str = "123"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Integer, 1, 1, 3, utf8::view("123"));
	}

	SECTION("floating point")
	{
		utf8::string       str = "3.1415"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::FloatingPoint, 1, 1, 6, utf8::view("3.1415"));
	}

	SECTION("symbols")
	{
		utf8::string       str = "+"sv;
		std::vector<Token> tokens;
		for (const auto& token : tokenize(str))
			tokens.emplace_back(token);

		CHECK(tokens.size() == 1);
		CHECK_TOKEN(str, tokens[0], TokenType::Plus, 1, 1, 1, utf8::view("+"));
	}
}
