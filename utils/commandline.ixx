export module deckard.commandline;

import std;
import deckard.types;
import deckard.stringhelper;
import deckard.utf8;
import deckard.as;

using namespace std::literals::string_view_literals;

namespace deckard
{
	/*
	*   -v  --verbose				- Flag
	 *  -O1 -O2 -optimize=2			- Level, single (O1), clamped 0-3 (O0, O1, O2, O3)
	 *  --log=debug.log				- Option, single value (no short)
	 *  --path=path/to/file			- Option, single value (no short)
	*   
	* 
	*	-d, --debug					- Option, single value (no short)
	 *	-v, --verbose				- Flag
	 *  -dv (same as -d -v)			- Multiple flags
	 * 
	 * 
	 *	bool verbose = false;
	 *  cli.option("-v, --verbose", "Enable verbose output", verbose);
	 * 
	*/

	class cli
	{
	private:
		std::vector<std::string> args;
		std::unordered_map<std::string, std::string> options;
		std::unordered_set<std::string>              flags;
	public:

		void flag(std::string_view name) 
		{
			//
		}

	};

	export void test_cmdliner() 
	{
		//
		std::string input("XYZ -v -X "sv);
		_;
	}


} // namespace deckard
