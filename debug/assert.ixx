
export module deckard.assert;

import std;
import deckard.debug;

export namespace deckard::assert
{


	void check([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message = "",
			   [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
#ifdef _DEBUG
		if (!expr)
		{
			dbg::println("\n***** Assert *****\n\n{}({}): {}\n\n***** Assert *****\n", loc.file_name(), loc.line(), message);

			dbg::stacktrace();

			dbg::println("\n***** Assert *****\n");
			dbg::panic("assert");
		}
#endif
	}


} // namespace deckard::assert
