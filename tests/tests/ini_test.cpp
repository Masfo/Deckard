#include <catch2/catch_test_macros.hpp>


import deckard.ini;
import deckard.config;

import std;
import deckard.utf8;
#undef EOF
using namespace deckard::ini;
using namespace deckard::config;
using namespace deckard::utf8;
using namespace std::string_literals;
using namespace std::string_view_literals;

// Encode
TEST_CASE("ini read", "[ini]")
{

	SECTION("config") 
	{ 

		config c("[sect] #     comment\r\n"_utf8);


		int j = 0;
	}

	SECTION("tokenize comments")
	{
		ini ini_win("# comment\r\n#short"_utf8);

		ini_win.tokenize();

		auto str = ini_win.format();
		CHECK(str.size() == 18);

		CHECK(str == "# comment\r\n# short"_utf8);
	}

	SECTION("tokenize section")
	{
		ini a("\"str\\\"ing\""_utf8);

		a.tokenize();
	}
}
