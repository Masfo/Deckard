
export module deckard.assert;
import deckard.log;

import std;
import deckard.debug;

export namespace deckard::assert
{

	void warning([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message = "",
				 [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
		if (not expr)
		{
			log::println("****\n{}({}): {}\n****", loc.file_name(), loc.line(), message);
		}
	}

	template<typename T, typename U>
	requires std::equality_comparable_with<T, U> and std::formattable<T, char> and std::formattable<U, char>
	void equal([[maybe_unused]] const T& a, const U& b, [[maybe_unused]] std::string_view message = "",
			   [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
		if constexpr (std::integral<T> and std::integral<U>)
		{
			if (std::cmp_not_equal(a, b))
			{
				log::println("\n***** EQUAL ASSERT *****\n");
				log::println("Assertion failed: '{}' != '{}'", a, b);
				warning(false, message, loc);
				dbg::stacktrace();
				log::println("\n\n***** EQUAL ASSERT *****\n");
				dbg::panic("assert");
			}
		}
		else
		{
			if (a != b)
			{
				log::println("\n***** EQUAL ASSERT *****\n");
				log::println("Assertion failed: '{}' != '{}'", a, b);
				warning(false, message, loc);

				dbg::stacktrace();
				log::println("\n\n***** EQUAL ASSERT *****\n");
				dbg::panic("assert");
			}
		}
	}

	void check([[maybe_unused]] bool expr, [[maybe_unused]] std::string_view message = "",
			   [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
		if (not expr)
		{
			log::println("\n***** ASSERT *****\n\n");

			warning(expr, message, loc);

			dbg::stacktrace();
			log::println("\n\n***** ASSERT *****\n");
			dbg::panic("assert");
		}
	}




} // namespace deckard::assert
