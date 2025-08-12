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



	export class cli
	{
	private:
		std::unordered_map<std::string, std::string> options;
		std::unordered_map<std::string, bool>        flags;

	public:
		// Parse short/long options from string
		// add_option("d,debug", "Enable debug");
		// add_option("v,verbose", "Verbose	output")
		//
		//
		// cli["v"]

		void option(std::string_view option, std::string_view help) { options[std::string(option)] = std::string(help); }

		template<typename T>
		requires( not std::is_convertible_v<T, std::string_view>)
		void option(std::string_view option, std::string_view help, T& value)
		{
			options[std::string(option)] = std::string(help);
		}

		void option(std::string_view short_option, std::string_view long_option, std::string_view help)
		{
			options[std::string(short_option)] = std::string(help);
			options[std::string(long_option)]  = std::string(help);
		}

		// example: -1, -2, -v,
		template<typename T>
		void flag(std::string_view flag, T& value, std::string_view help)
		{
			flags[std::string(flag)] = false;
		}

		void parse(int argc, const char* argv[]) 
		{ 
			std::vector<std::string> args(argv + 1, argv + argc);

			parse(args);
		}

		void parse(const std::vector<std::string> &args)
		{
			for (int i = 1; i < args.size(); ++i)
			{
				auto arg = args[i];
				if (arg[0] == '-')
				{
					if (arg[1] == '-')
					{
						// Long option
						arg = arg.substr(2);
					}
					else
					{
						// Short option
						arg = arg.substr(1);
					}
					if (options.find(arg) != options.end())
					{
						if (i + 1 < args.size())
						{
							options[arg] = args[i];
						}
					}
					else if (flags.find(arg) != flags.end())
					{
						flags[arg] = true;
					}
				}
			}
		}

		void parse(std::string_view commandline)
		{

			using namespace deckard::string;

			auto args = split<std::string>(commandline);

			parse(args);
		}

		std::string operator[](std::string_view option) const
		{
			auto it = options.find(std::string(option));
			if (it != options.end())
			{
				return it->second;
			}
			return "";
		}

		bool is_flag(std::string_view flag) const
		{
			auto it = flags.find(std::string(flag));
			if (it != flags.end())
			{
				return it->second;
			}
			return false;
		}
	};
} // namespace deckard
