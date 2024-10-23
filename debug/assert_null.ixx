
export module deckard.assert;

import std;

export namespace deckard::assert
{


	void check(bool, [[maybe_unused]] std::string_view message = "",
			   [[maybe_unused]] const std::source_location& loc = std::source_location::current())
	{
	}

} // namespace deckard::assert
