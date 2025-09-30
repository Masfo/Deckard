export module deckard.commandline;

import std;
import deckard.stringhelper;
import deckard.as;

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



	class commandlineparser
	{
	private:
		std::vector<std::string> args;
		std::unordered_map<std::string, std::string> options;
		std::unordered_set<std::string>              flags;
	public:

	};

	export void test_cmdliner() 
	{
		//

	}


} // namespace deckard
