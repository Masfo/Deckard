
export module deckard.assert;

import std;
import deckard.debug;
import deckard.math;

export namespace deckard::assert
{


	void if_true([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message = "",
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
			dbg::panic("assert");
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

	void if_equal([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message,
				  [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		if_true(expr, message, loc);
#endif
	}

	void if_not_equal([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message,
					  [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		if_equal(not expr, message, loc);
#endif
	}

	template<std::floating_point T>
	void if_close_enough([[maybe_unused]] T &A, T &B, [[maybe_unused]] std::string_view message,
						 [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	{
#ifdef _DEBUG
		if_true(math::is_close_enough(A, B), message, loc);
#endif
	}

	template<typename T>
	void if_null([[maybe_unused]] T expr, [[maybe_unused]] std::string_view message,
				 [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	requires(std::is_pointer_v<T>)
	{
#ifdef _DEBUG
		if_true(expr == nullptr, message, loc);
#endif
	}

	template<typename T>
	void if_not_null([[maybe_unused]] T expr, [[maybe_unused]] std::string_view message,
					 [[maybe_unused]] const std::source_location &loc = std::source_location::current()) noexcept
	requires(std::is_pointer_v<T>)
	{
#ifdef _DEBUG
		if_null(expr != nullptr, message, loc);
#endif
	}


} // namespace deckard::assert
