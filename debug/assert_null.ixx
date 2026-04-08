
export module deckard.assert;

import std;

export namespace deckard::assert
{

	void warning(bool  ,  std::string_view message = "",
				 const std::source_location& loc = std::source_location::current())
	{
	}

	template<typename T, typename U>
	requires std::equality_comparable_with<T, U> and std::formattable<T, char> and std::formattable<U, char>
	void equal([[maybe_unused]] const T& , [[maybe_unused]] const U& , [[maybe_unused]] std::string_view message = "",
			   [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
	}

	void check(bool  ,  std::string_view message = "",
			   const std::source_location& loc = std::source_location::current())
	{
	}

} // namespace deckard::assert
