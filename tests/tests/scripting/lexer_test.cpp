#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.lexer;
import deckard.math;
import deckard.utf8;
import std;
#undef EOF

using namespace Catch::Matchers;

using namespace std::string_view_literals;
using namespace deckard::lexer;
using namespace deckard::math;
using namespace deckard::utf8;

bool check_token(token t, enum Token type, const std::wstring& str)
{
	//
	bool str_cmp = str.empty() ? true : t.str_literal == str;

	return (str_cmp == true) and (t.type == type);
}

bool check_token_float(token t, double value)
{
	if (not t.as_double().has_value())
		return false;

	if (not(t.type == Token::FLOATING_POINT))
		return false;

	return is_close_enough(t.as_double().value(), value);
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

	SECTION("newlines")
	{
		tokenizer  l("1\r\n2\n-3\r-4"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 5);
		REQUIRE(check_token(tokens[0], Token::UNSIGNED_INTEGER, L"1"));
		REQUIRE(check_token(tokens[1], Token::UNSIGNED_INTEGER, L"2"));
		REQUIRE(check_token(tokens[2], Token::INTEGER, L"-3"));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"-4"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}

	SECTION("whitespaces")
	{
		tokenizer  l("1\t\n2 3 \t 4"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 5);
		REQUIRE(check_token(tokens[0], Token::UNSIGNED_INTEGER, L"1"));
		REQUIRE(check_token(tokens[1], Token::UNSIGNED_INTEGER, L"2"));
		REQUIRE(check_token(tokens[2], Token::UNSIGNED_INTEGER, L"3"));
		REQUIRE(check_token(tokens[3], Token::UNSIGNED_INTEGER, L"4"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}

	SECTION("symbols")
	{
		tokenizer  l("# @ ! = + - * / _  % < > ? | & ^ ~ \\ : ; . ,"sv);
		const auto tokens = l.tokenize();
		int        count  = 0;
		REQUIRE(check_token(tokens[count++], Token::HASH, L"#"));
		REQUIRE(check_token(tokens[count++], Token::AT, L"@"));
		REQUIRE(check_token(tokens[count++], Token::BANG, L"!"));
		REQUIRE(check_token(tokens[count++], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[count++], Token::PLUS, L"+"));
		REQUIRE(check_token(tokens[count++], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[count++], Token::STAR, L"*"));
		REQUIRE(check_token(tokens[count++], Token::SLASH, L"/"));
		REQUIRE(check_token(tokens[count++], Token::UNDERSCORE, L"_"));
		REQUIRE(check_token(tokens[count++], Token::PERCENT, L"%"));
		REQUIRE(check_token(tokens[count++], Token::LESSER, L"<"));
		REQUIRE(check_token(tokens[count++], Token::GREATER, L">"));
		REQUIRE(check_token(tokens[count++], Token::QUESTION, L"?"));
		REQUIRE(check_token(tokens[count++], Token::OR, L"|"));
		REQUIRE(check_token(tokens[count++], Token::AND, L"&"));
		REQUIRE(check_token(tokens[count++], Token::XOR, L"^"));
		REQUIRE(check_token(tokens[count++], Token::TILDE, L"~"));
		REQUIRE(check_token(tokens[count++], Token::BACK_SLASH, L"\\"));
		REQUIRE(check_token(tokens[count++], Token::COLON, L":"));
		REQUIRE(check_token(tokens[count++], Token::SEMI_COLON, L";"));
		REQUIRE(check_token(tokens[count++], Token::DOT, L"."));
		REQUIRE(check_token(tokens[count++], Token::COMMA, L","));

		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}

	SECTION("multisymbols")
	{
		tokenizer  l("== != += -= *= /= %= ^= &= |= && || -> .. /* */ //"sv);
		const auto tokens = l.tokenize();
		int        count  = 0;
		REQUIRE(check_token(tokens[count++], Token::EQUAL_EQUAL, L"=="));
		REQUIRE(check_token(tokens[count++], Token::BANG_EQUAL, L"!="));
		REQUIRE(check_token(tokens[count++], Token::PLUS_EQUAL, L"+="));
		REQUIRE(check_token(tokens[count++], Token::MINUS_EQUAL, L"-="));
		REQUIRE(check_token(tokens[count++], Token::STAR_EQUAL, L"*="));
		REQUIRE(check_token(tokens[count++], Token::SLASH_EQUAL, L"/="));
		REQUIRE(check_token(tokens[count++], Token::PERCENT_EQUAL, L"%="));
		REQUIRE(check_token(tokens[count++], Token::XOR_EQUAL, L"^="));
		REQUIRE(check_token(tokens[count++], Token::AND_EQUAL, L"&="));
		REQUIRE(check_token(tokens[count++], Token::OR_EQUAL, L"|="));
		REQUIRE(check_token(tokens[count++], Token::AND_AND, L"&&"));
		REQUIRE(check_token(tokens[count++], Token::OR_OR, L"||"));
		REQUIRE(check_token(tokens[count++], Token::ARROW, L"->"));
		REQUIRE(check_token(tokens[count++], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[count++], Token::SLASH_STAR, L"/*"));
		REQUIRE(check_token(tokens[count++], Token::STAR_SLASH, L"*/"));
		REQUIRE(check_token(tokens[count++], Token::SLASH_SLASH, L"//"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}


	SECTION("multisymbols 2")
	{
		tokenizer  l("X++ --Y"sv);
		const auto tokens = l.tokenize();
		int        count  = 0;
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"X"));
		REQUIRE(check_token(tokens[count++], Token::PLUS_PLUS, L"++"));
		REQUIRE(check_token(tokens[count++], Token::MINUS_MINUS, L"--"));
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"Y"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}


	SECTION("paren_bracket_brace")
	{
		tokenizer  l("()[]{}"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 7);
		REQUIRE(check_token(tokens[0], Token::LEFT_PAREN, L"("));
		REQUIRE(check_token(tokens[1], Token::RIGHT_PAREN, L")"));
		REQUIRE(check_token(tokens[2], Token::LEFT_BRACKET, L"["));
		REQUIRE(check_token(tokens[3], Token::RIGHT_BRACKET, L"]"));
		REQUIRE(check_token(tokens[4], Token::LEFT_BRACE, L"{"));
		REQUIRE(check_token(tokens[5], Token::RIGHT_BRACE, L"}"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}


	SECTION("integers")
	{
		tokenizer l("123 0x400 -26 -0x100 0x 0x3.14"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 7);

		REQUIRE(check_token(tokens[0], Token::UNSIGNED_INTEGER, L"123"));
		REQUIRE(check_token(tokens[1], Token::UNSIGNED_INTEGER, L"0x400"));
		REQUIRE(check_token(tokens[2], Token::INTEGER, L"-26"));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"-0x100"));
		REQUIRE(check_token(tokens[4], Token::INVALID_HEX, L"0x"));
		REQUIRE(check_token(tokens[5], Token::INVALID_HEX, L"0x3.14"));


		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}


	SECTION("floating point")
	{
#if 1
		tokenizer  l("3.14 .123 3. -.678"sv);
		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 5);

		REQUIRE(check_token(tokens[0], Token::FLOATING_POINT, L"3.14"));
		REQUIRE(check_token(tokens[1], Token::FLOATING_POINT, L".123"));
		REQUIRE(check_token(tokens[2], Token::FLOATING_POINT, L"3."));
		REQUIRE(check_token(tokens[3], Token::FLOATING_POINT, L"-.678"));

		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
#endif
	}

	SECTION("character")
	{
		tokenizer l(R"('a' 'c)"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 3);
		REQUIRE(check_token(tokens[0], Token::CHARACTER, L"a"));
		REQUIRE(check_token(tokens[1], Token::INVALID_CHAR, L"c"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}


	SECTION("string")
	{
		tokenizer l(R"(π = "three")"sv);

		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 4);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, L"π"));
		REQUIRE(check_token(tokens[1], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[2], Token::STRING, L"three"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}

	SECTION("string escapes")
	{
		tokenizer  l(R"("\"hello\t\"" key "\tworld")"sv);
		const auto tokens = l.tokenize();

		REQUIRE(tokens.size() == 4);
		REQUIRE(check_token(tokens[0], Token::STRING, L"\"hello\t\""));
		REQUIRE(check_token(tokens[1], Token::IDENTIFIER, L"key"));
		REQUIRE(check_token(tokens[2], Token::STRING, L"\tworld"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}


	SECTION("multiline string")
	{
		tokenizer l("\"hello \\\r\n\tanother \\\r\n\tline\""sv);

		// const auto tokens = l.tokenize();
		//
		// REQUIRE(tokens.size() == 2);
		// REQUIRE(check_token(tokens[0], Token::STRING, L"hello \r\n\tanother \r\n\tline"));
		// REQUIRE(check_token(tokens.back(), Token::EOF, L""));
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
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}
}
