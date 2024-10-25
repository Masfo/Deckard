export module deckard.commandline;

import std;

namespace deckard
{
	class cli
	{
	private:
	public:
		// Parse short/long options from string
		// add_option("d,debug", "Enable debug");
		// add_option("v,verbose", "Verbose	output")
		//
		//
		// cli["v"]

		void add_option(std::string_view option, std::string_view help);

		template<typename T>
		void add_option(std::string_view option, std::string_view help, T& value);

		void add_option(std::string_view short_option, std::string_view long_option, std::string_view help);

		// example: -1, -2, -v,
		template<typename T>
		void add_flag(std::string_view flag, T& value, std::string_view help);
	};

	//
} // namespace deckard
