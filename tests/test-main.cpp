#include <catch2/catch_all.hpp>

import std;
import deckard.log;

/*
#include <catch2/catch_all.hpp>
//#include <catch2/catch_test_macros.hpp>
//#include <catch2/catch_approx.hpp>
//#include <catch2/matchers/catch_matchers_floating_point.hpp>
//#include <catch2/catch_approx.hpp>


import std;


TEST_CASE("", "[tag]")
{
	SECTION("")
	{
		//
		CHECK(1==1);
	}


}

*/


int main(int argc, char* argv[])
{
	// Suppress logger initialization during test discovery
	bool is_test_discovery = false;
	for (int i = 1; i < argc; ++i)
	{
		if (std::string_view(argv[i]) == "--list-tests")
		{
			is_test_discovery = true;
			break;
		}
	}

	if (is_test_discovery)
		deckard::log::set_suppress_init(true);
	else
		deckard::log::initialize();

	return Catch::Session().run(argc, argv);
}
