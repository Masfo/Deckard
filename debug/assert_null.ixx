
export module deckard.assert;

import std;

export namespace deckard::assert
{

	void warning(bool  ,  std::string_view message = "",
				 const std::source_location& loc = std::source_location::current())
	{
	}


	void check(bool  ,  std::string_view message = "",
			   const std::source_location& loc = std::source_location::current())
	{
	}

} // namespace deckard::assert
