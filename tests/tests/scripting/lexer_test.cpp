#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

import deckard.lexer;
import deckard.math;
import deckard.utf8;
import deckard.debug;
import std;
#undef EOF

using namespace Catch::Matchers;
namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace deckard::lexer;
using namespace deckard::math;
using namespace deckard::utf8;
using namespace deckard;

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

#if 0
TEST_CASE("tokens", "[lexer]")
{
	SECTION("file")
	{
		//
		// fs::path  input = fs::path{TEST_PATH} / "simple01.txt";
		// tokenizer tok(input);
		// auto      tokens = tok.tokenize();
		//
		// int k = 0;
	}

	SECTION("eof")
	{
		tokenizer  l(R"()"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 1);
		REQUIRE(check_token(tokens[0], Token::EOF, L""));
	}


	SECTION("identifiers")
	{
		tokenizer l(R"(hello)"sv);
		auto      tokens = l.tokenize();
		REQUIRE(tokens.size() == 2);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, L"hello"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));


		l      = "hello.with.dots"sv;
		tokens = l.tokenize({.dot_identifier = true});
		REQUIRE(tokens.size() == 2);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, L"hello.with.dots"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}

	SECTION("newlines")
	{
		tokenizer  l("1\r\n2\n-3\r-4"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 7);
		REQUIRE(check_token(tokens[0], Token::INTEGER, L"1"));
		REQUIRE(check_token(tokens[1], Token::INTEGER, L"2"));
		REQUIRE(check_token(tokens[2], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"3"));
		REQUIRE(check_token(tokens[4], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[5], Token::INTEGER, L"4"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}

	SECTION("newlines - eat síngle backslash and newline")
	{
		tokenizer  l("hello \\\n\"another line\""sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 3);
		REQUIRE(check_token(tokens[0], Token::IDENTIFIER, L"hello"));
		REQUIRE(check_token(tokens[1], Token::STRING, L"another line"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
	}

	SECTION("whitespaces")
	{
		tokenizer  l("1\t\n2 3 \t 4"sv);
		const auto tokens = l.tokenize();
		REQUIRE(tokens.size() == 5);
		REQUIRE(check_token(tokens[0], Token::INTEGER, L"1"));
		REQUIRE(check_token(tokens[1], Token::INTEGER, L"2"));
		REQUIRE(check_token(tokens[2], Token::INTEGER, L"3"));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"4"));
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
		tokenizer  l("== != += -= *= /= %= ^= &= |= && || -> .. /* */ // <<= >>="sv);
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
		REQUIRE(check_token(tokens[count++], Token::LESSER_LESSER_EQUAL, L"<<="));
		REQUIRE(check_token(tokens[count++], Token::GREATER_GREATER_EQUAL, L">>="));

		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}


	SECTION("ellipsis ranges exclusive/inclusive")
	{
		tokenizer l(R"(0..5)"sv);
		auto      tokens = l.tokenize();

		REQUIRE(check_token(tokens[0], Token::INTEGER, L"0"));
		REQUIRE(check_token(tokens[1], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[2], Token::INTEGER, L"5"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 4);

		l      = "3.14..0b101010"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::FLOATING_POINT, L"3.14"));
		REQUIRE(check_token(tokens[1], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[2], Token::INTEGER, L"0b101010"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 4);

		l      = "3.14..=-1.234"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::FLOATING_POINT, L"3.14"));
		REQUIRE(check_token(tokens[1], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[2], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[3], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[4], Token::FLOATING_POINT, L"1.234"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 6);


		l      = "-0b10..-0b100"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[1], Token::INTEGER, L"0b10"));
		REQUIRE(check_token(tokens[2], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[3], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[4], Token::INTEGER, L"0b100"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 6);

		l      = "-0x10..0x200"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[1], Token::INTEGER, L"0x10"));
		REQUIRE(check_token(tokens[2], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"0x200"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 5);

		l      = "-0b..0x"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[1], Token::INVALID_BINARY, L"0b"));
		REQUIRE(check_token(tokens[2], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[3], Token::INVALID_HEX, L"0x"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 5);


		l      = "0..=5"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::INTEGER, L"0"));
		REQUIRE(check_token(tokens[1], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[2], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"5"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 5);

		l      = "0b10..=-0x5"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::INTEGER, L"0b10"));
		REQUIRE(check_token(tokens[1], Token::ELLIPSIS, L".."));
		REQUIRE(check_token(tokens[2], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[3], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[4], Token::INTEGER, L"0x5"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 6);
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
		tokenizer l("123 0x400 -26 -0X100 0x"sv);

		const auto tokens = l.tokenize();

		REQUIRE(check_token(tokens[0], Token::INTEGER, L"123"));
		REQUIRE(check_token(tokens[1], Token::INTEGER, L"0x400"));
		REQUIRE(check_token(tokens[2], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"26"));
		REQUIRE(check_token(tokens[4], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[5], Token::INTEGER, L"0X100"));
		REQUIRE(check_token(tokens[6], Token::INVALID_HEX, L"0x"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 8);
	}


	SECTION("floating point")
	{
		tokenizer l("3.14 .123 3. -.678"sv);
		auto      tokens = l.tokenize();

		REQUIRE(check_token(tokens[0], Token::FLOATING_POINT, L"3.14"));
		REQUIRE(check_token(tokens[1], Token::FLOATING_POINT, L".123"));
		REQUIRE(check_token(tokens[2], Token::FLOATING_POINT, L"3."));
		REQUIRE(check_token(tokens[3], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[4], Token::FLOATING_POINT, L".678"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 6);
	}

	SECTION("binary integer")
	{
		tokenizer l("0b101010 0b -0B1010"sv);
		auto      tokens = l.tokenize();

		REQUIRE(check_token(tokens[0], Token::INTEGER, L"0b101010"));
		REQUIRE(check_token(tokens[1], Token::INVALID_BINARY, L"0b"));
		REQUIRE(check_token(tokens[2], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[3], Token::INTEGER, L"0B1010"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 5);
	}

	SECTION("character")
	{
		tokenizer l(R"('a' 'c)"sv);

		auto tokens = l.tokenize();

		REQUIRE(check_token(tokens[0], Token::CHARACTER, L"a"));
		REQUIRE(check_token(tokens[1], Token::INVALID_CHAR, L"c"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 3);


		l      = "'ABCD'"sv;
		tokens = l.tokenize();
		REQUIRE(check_token(tokens[0], Token::CHARACTER, L"ABCD"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == 2);
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

		int count = 0;
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_FN, L"fn"));
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"π"));
		REQUIRE(check_token(tokens[count++], Token::LEFT_PAREN, L"("));
		REQUIRE(check_token(tokens[count++], Token::RIGHT_PAREN, L")"));
		REQUIRE(check_token(tokens[count++], Token::LEFT_BRACE, L"{"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_RETURN, L"return"));
		REQUIRE(check_token(tokens[count++], Token::STRING, L"three"));
		REQUIRE(check_token(tokens[count++], Token::SEMI_COLON, L";"));
		REQUIRE(check_token(tokens[count++], Token::RIGHT_BRACE, L"}"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}

	SECTION("keyword 2")
	{
		tokenizer l(R"(let fn return if else true false struct enum)"sv);

		const auto tokens = l.tokenize();

		int count = 0;

		REQUIRE(check_token(tokens[count++], Token::KEYWORD_LET, L"let"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_FN, L"fn"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_RETURN, L"return"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_IF, L"if"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_ELSE, L"else"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_TRUE, L"true"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_FALSE, L"false"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_STRUCT, L"struct"));
		REQUIRE(check_token(tokens[count++], Token::KEYWORD_ENUM, L"enum"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));

		REQUIRE(tokens.size() == ++count);
	}
}

TEST_CASE("longer examples", "[lexer]")
{

	SECTION("simple integer math tokens ")
	{
		tokenizer l(R"(1+2*3/4=7)"sv);
		auto      tokens = l.tokenize();

		int count = 0;
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"1"));
		REQUIRE(check_token(tokens[count++], Token::PLUS, L"+"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"2"));
		REQUIRE(check_token(tokens[count++], Token::STAR, L"*"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"3"));
		REQUIRE(check_token(tokens[count++], Token::SLASH, L"/"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"4"));
		REQUIRE(check_token(tokens[count++], Token::EQUAL, L"="));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"7"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}

	SECTION("simple negative integer math token")
	{
		tokenizer l(R"(-1+-5)"sv);
		auto      tokens = l.tokenize();

		int count = 0;
		REQUIRE(check_token(tokens[count++], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"1"));
		REQUIRE(check_token(tokens[count++], Token::PLUS, L"+"));
		REQUIRE(check_token(tokens[count++], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"5"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}

	SECTION("simple negative integer and idents")
	{
		tokenizer l(R"(X+-5)"sv);
		auto      tokens = l.tokenize();

		int count = 0;
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"X"));
		REQUIRE(check_token(tokens[count++], Token::PLUS, L"+"));
		REQUIRE(check_token(tokens[count++], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"5"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}

	SECTION("simple negative float and idents")
	{
		tokenizer l(R"(X+-.5)"sv);
		auto      tokens = l.tokenize();

		int count = 0;
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"X"));
		REQUIRE(check_token(tokens[count++], Token::PLUS, L"+"));
		REQUIRE(check_token(tokens[count++], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[count++], Token::FLOATING_POINT, L".5"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}

	SECTION("mix math hex/bin/dec/float/idents")
	{
		tokenizer l(R"(C-.1*1.^0b10/ 0X42+C == "value"('ABCD'); // comment)"sv);
		auto      tokens = l.tokenize();

		int count = 0;
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"C"));
		REQUIRE(check_token(tokens[count++], Token::MINUS, L"-"));
		REQUIRE(check_token(tokens[count++], Token::FLOATING_POINT, L".1"));
		REQUIRE(check_token(tokens[count++], Token::STAR, L"*"));
		REQUIRE(check_token(tokens[count++], Token::FLOATING_POINT, L"1."));
		REQUIRE(check_token(tokens[count++], Token::XOR, L"^"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"0b10"));
		REQUIRE(check_token(tokens[count++], Token::SLASH, L"/"));
		REQUIRE(check_token(tokens[count++], Token::INTEGER, L"0X42"));
		REQUIRE(check_token(tokens[count++], Token::PLUS, L"+"));
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"C"));
		REQUIRE(check_token(tokens[count++], Token::EQUAL_EQUAL, L"=="));
		REQUIRE(check_token(tokens[count++], Token::STRING, L"value"));
		REQUIRE(check_token(tokens[count++], Token::LEFT_PAREN, L"("));
		REQUIRE(check_token(tokens[count++], Token::CHARACTER, L"ABCD"));
		REQUIRE(check_token(tokens[count++], Token::RIGHT_PAREN, L")"));
		REQUIRE(check_token(tokens[count++], Token::SEMI_COLON, L";"));
		REQUIRE(check_token(tokens[count++], Token::SLASH_SLASH, L"//"));
		REQUIRE(check_token(tokens[count++], Token::IDENTIFIER, L"comment"));
		REQUIRE(check_token(tokens.back(), Token::EOF, L""));
		REQUIRE(tokens.size() == ++count);
	}
}
#endif
