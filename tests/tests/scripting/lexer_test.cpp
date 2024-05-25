#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.lexer;
import deckard.utf8;
import std;
#undef EOF

using namespace std::string_view_literals;
using namespace deckard::lexer;
using namespace deckard::utf8;

bool check_token(token t, enum Token type, const std::wstring& str)
{
	//
	bool str_cmp = str.empty() ? true : t.str_literal == str;

	return (str_cmp == true) and (t.type == type);
}

TEST_CASE("tokens", "[lexer]")
{
	SECTION("eof")
	{
		tokenizer  l(R"()"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 1);
		REQUIRE(check_token(tokens[0], Token::EOF, L""));
	}

	SECTION("integers")
	{
		tokenizer l("1 2"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 3);
		REQUIRE(check_token(tokens[0], Token::INTEGER, L"1"));
		REQUIRE(check_token(tokens[1], Token::INTEGER, L"2"));
		REQUIRE(check_token(tokens[2], Token::EOF, L""));
	}

	SECTION("string")
	{
		tokenizer l(R"(π = "three")"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 4);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, L"π"));
		REQUIRE(check_token(tokens[1], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[2], Token::STRING, L"three"));
		REQUIRE(check_token(tokens[3], Token::EOF, L""));
	}


	SECTION("keyword")
	{
		tokenizer l(R"(fn π()  { return "three"; })"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 10);
		REQUIRE(check_token(tokens[0], Token::KEYWORD, L"fn"));
		REQUIRE(check_token(tokens[1], Token::IDENTIFIER, L"π"));
		REQUIRE(check_token(tokens[2], Token::LEFT_PAREN, L"("));
		REQUIRE(check_token(tokens[3], Token::RIGHT_PAREN, L")"));
		REQUIRE(check_token(tokens[4], Token::LEFT_BRACE, L"{"));
		REQUIRE(check_token(tokens[5], Token::KEYWORD, L"return"));
		REQUIRE(check_token(tokens[6], Token::STRING, L"three"));
		REQUIRE(check_token(tokens[7], Token::SEMI_COLON, L";"));
		REQUIRE(check_token(tokens[8], Token::RIGHT_BRACE, L"}"));
		REQUIRE(check_token(tokens[9], Token::EOF, L""));
	}
}
