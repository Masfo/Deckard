module;

#include <Windows.h>


export module deckard.assert;

import std;
import deckard.debug;

using namespace deckard;

export namespace deckard
{

#ifdef _DEBUG

	[[noreturn]] void debug_and_halt()
	{
		if (IsDebuggerPresent())
		{
			DebugBreak();
			FatalExit(0);
		}
	}

	export void assert_msg(bool expr, std::string_view message, const std::source_location &loc = std::source_location::current()) noexcept
	{
		if (!expr)
		{
			// std::println("\nAssert *****\n\n {}({}): {}", loc.file_name(), loc.line(), message);
			dbgln("\n***** Assert *****\n\n{}({}): {}\n\n***** Assert *****\n", loc.file_name(), loc.line(), message);

			auto traces = std::stacktrace::current();

			for (const auto &traceline : traces)
			{
				if (traceline.source_file().contains(__FILE__))
					continue;

				dbgln("{}({}): {}", traceline.source_file(), traceline.source_line(), traceline.description());
			}

			dbgln("\nAssert *****\n\n");

			debug_and_halt();
		}
	}

	export void assert(bool expr = false, const std::source_location &loc = std::source_location::current()) noexcept
	{
		assert_msg(expr, "assert", loc);
	}


#else
	export void assert_msg(bool, std::string_view) noexcept { }

	export void assert(bool) noexcept { }
}
#endif
} // namespace deckard
