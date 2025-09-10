export module deckard.log;

import std;

namespace deckard::log
{
	export struct log_to_file
	{
		void log(std::string_view) { }
	};

	export log_to_file global_log;

} // namespace deckard::log
