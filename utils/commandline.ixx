export module deckard.commandline;

import std;
import deckard.types;
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
	 *  Validate in option:
	 *  .validate, .range, is_directory, is_file, is_ipv4/6, 
	 */

	using CLIValue = std::variant<std::monostate, bool, i64, u64, std::string>;

	class cli
	{
	private:


		utf8::string commandline;

	public:
		cli(int argc, const char* const argv[])
		{
			std::vector<std::string> args(argv, argv + argc);
			commandline = args | std::views::join_with(' ') | std::ranges::to<std::string>();
		}

		cli(std::string_view sv)
			: commandline(sv)
		{
		}

		cli(utf8::string us)
			: commandline(us)
		{
		}

		void flag(std::string_view name)
		{
			//
		}



		
	};

	export void test_cmdliner()
	{
		//
		std::string input(R"(XYZ -v -X )");

		const char* argv[] = {"program", R"("C:\fol\er go\)", "arg2", "arg3"};
		int         argc   = sizeof(argv) / sizeof(argv[0]);


		cli cli(argc, argv);

		_;
	}


} // namespace deckard
