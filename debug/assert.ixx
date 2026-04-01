
export module deckard.assert;

import std;
import deckard.debug;

export namespace deckard::assert
{

	void warning([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message = "",
				 [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
		if (!expr)
		{
			dbg::println("Warning: {}({}): {}", loc.file_name(), loc.line(), message);
		}
	}

	void check([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message = "",
			   [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
#ifdef _DEBUG
		if (!expr)
		{
			dbg::println("\n***** ASSERT *****\n\n");

			warning(expr, message, loc);

			dbg::stacktrace();
			dbg::println("\n\n***** ASSERT *****\n");
			dbg::panic("assert");
		}
#endif
	}


} // namespace deckard::assert
