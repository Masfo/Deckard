#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.lexer;
import deckard.utf8;
import std;
#undef EOF

using namespace std::string_view_literals;
using namespace deckard::lexer;
using namespace deckard::utf8;

bool check_token(token t, enum Token type, const std::string& str)
{
	//
	return t.str_literal == str and str.empty() ? true : t.type == type;
}

TEST_CASE("tokens", "[lexer]")
{

	SECTION("simple01")
	{
		tokenizer l("fn λ = π * π ?"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 8);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, "fn"));
		REQUIRE(check_token(tokens[1], Token::IDENTIFIER, "λ"));
		REQUIRE(check_token(tokens[2], Token::EQUAL, "="));
		REQUIRE(check_token(tokens[3], Token::IDENTIFIER, "π"));
		REQUIRE(check_token(tokens[4], Token::STAR, "*"));
		REQUIRE(check_token(tokens[5], Token::IDENTIFIER, "π"));
		REQUIRE(check_token(tokens[6], Token::QUESTION, "?"));
		REQUIRE(check_token(tokens[7], Token::EOF, ""));
	}

	SECTION("string")
	{
		tokenizer l(R"(π = "three")"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 4);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, "π"));
		REQUIRE(check_token(tokens[1], Token::EQUAL, "="));
		REQUIRE(check_token(tokens[2], Token::STRING, "three"));
		REQUIRE(check_token(tokens[3], Token::EOF, ""));
	}
}
