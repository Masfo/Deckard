
export module deckard.assert;

import std;
import deckard.debug;

export namespace deckard::assert
{

	void if_true([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message,
				 [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		if (!expr)
		{
			dbg::println("\n***** Assert *****\n\n{}({}): {}\n\n***** Assert *****\n", loc.file_name(), loc.line(), message);

			auto traces = std::stacktrace::current();

			for (const auto &traceline : traces)
			{
				if (traceline.source_file().contains(__FILE__))
					continue;

				dbg::println("{}({}): {}", traceline.source_file(), traceline.source_line(), traceline.description());
			}

			dbg::println("\n***** Assert *****\n");

			dbg::panic();
		}
#endif
	}

	void if_false([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message,
				  [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		if_true(not expr, message, loc);
#endif
	}


} // namespace deckard::assert
