module;
#include <Windows.h>

export module deckard.system;

import std;
import deckard.assert;

namespace deckard::system
{

	using GetProcAddress_t = void*(void* mod, const char* proc_name);
	using LoadLibraryA_t   = void*(const char* module_name);
	GetProcAddress_t* GetProcAddress_;
	LoadLibraryA_t*   LoadLibraryA_;

	export template<typename T>
	T get_address(std::string_view dll, std::string_view apiname,
				  const std::source_location& loc = std::source_location::current()) noexcept
	{
		auto library = LoadLibraryA(dll.data());
		assert::check(library != nullptr, std::format("library '{}' not found.", dll), loc);

		T function = reinterpret_cast<T>(static_cast<void*>(GetProcAddress(library, apiname.data())));
		assert::check(function != nullptr, std::format("function '{}' in library '{}' not found.", apiname, dll), loc);

		return function;
	}

} // namespace deckard::system
