#include <catch2/catch_test_macros.hpp>


import deckard.ini;
import std;
import deckard.utf8;
#undef EOF
using namespace deckard::ini;
using namespace deckard::utf8;
using namespace std::string_literals;
using namespace std::string_view_literals;


// Encode
TEST_CASE("ini read", "[ini]")
{


	SECTION("tokenize comments")
	{
		ini r("# comment\r\n#short"_utf8);

		r.tokenize();

		// dont actually check each token
		// just check if r["section.key"] == "value"

		CHECK(r.at(0) == Token{.type = TokenType::COMMENT, .value = "comment"_utf8});
		CHECK(r.at(1) == Token{.type = TokenType::NEW_LINE});
		CHECK(r.at(2) == Token{.type = TokenType::COMMENT, .value = "short"_utf8});
		CHECK(r.at(3) == Token{.type = TokenType::END_OF_FILE});

	}

	SECTION("tokenize section")
	{
		ini r("# comment\n[section]\nkey=value"_utf8);

		r.tokenize();

		//CHECK(r.at(0) == Token{.type = TokenType::COMMENT, .value = "comment"_utf8});
		//CHECK(r.at(1) == Token{.type = TokenType::NEW_LINE});
		//CHECK(r.at(2) == Token{.type = TokenType::COMMENT, .value = "short"_utf8});
		//CHECK(r.at(3) == Token{.type = TokenType::END_OF_FILE});
	}
}
