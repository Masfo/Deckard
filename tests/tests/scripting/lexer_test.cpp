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

	lexer l("fn π() -> u32 {}"sv);

	const auto tokens = l.tokenize();

	REQUIRE(tokens.size() == 10);
	REQUIRE(check_token(tokens[0], Token::IDENTIFIER, "fn"));
	REQUIRE(check_token(tokens[1], Token::IDENTIFIER, "π"));
	REQUIRE(check_token(tokens[2], Token::LEFT_PAREN, "("));
	REQUIRE(check_token(tokens[3], Token::RIGHT_PAREN, ")"));
	REQUIRE(check_token(tokens[4], Token::MINUS, "-"));
	REQUIRE(check_token(tokens[5], Token::GREATER, ">"));
	REQUIRE(check_token(tokens[6], Token::IDENTIFIER, "u32"));
	REQUIRE(check_token(tokens[7], Token::LEFT_BRACE, "{"));
	REQUIRE(check_token(tokens[8], Token::RIGHT_BRACE, "}"));
	REQUIRE(check_token(tokens[9], Token::EOF, ""));
}
