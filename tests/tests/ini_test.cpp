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
		ini ini_win("# comment\r\n#short"_utf8);

		ini_win.tokenize();

		auto str = ini_win.format();
		CHECK(str == "# comment\n# short"_utf8);
	}

	SECTION("tokenize section")
	{
	}
}
