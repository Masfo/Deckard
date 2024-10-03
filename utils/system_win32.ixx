module;
#include <Windows.h>

export module deckard.system;

import std;
import deckard.assert;

namespace deckard::system
{


	export template<typename T>
	requires std::is_pointer_v<T>
	T get_address(std::string_view dll, std::string_view apiname, const std::source_location& loc = std::source_location::current())
	{
		auto library = LoadLibraryA(dll.data());
		assert::check(library != nullptr, std::format("library '{}' not found.", dll), loc);
		if (not library)
			return nullptr;

		T function = reinterpret_cast<T>(static_cast<void*>(GetProcAddress(library, apiname.data())));
		assert::check(function != nullptr, std::format("function '{}' in library '{}' not found.", apiname, dll), loc);
		if (not function)
			return nullptr;

		return function;
	}

} // namespace deckard::system
