module;
#include <Windows.h>

export module deckard.console;

import std;
import deckard.types;

namespace deckard
{

	constexpr static std::string_view DECKARD_DEBUG_CONSOLE_CLASS = "DECKARD_DEBUG_CONSOLE_CLASS";
	constexpr static std::string_view DECKARD_DEBUG_CONSOLE_TITLE = "Deckard Console";

	export class console
	{
	private:
		HWND handle{nullptr};
		u32  ex_style{};
		u32  style{};

		u32 width{1280}, height{720};

		bool initialize()
		{
			WNDCLASSEXA wc{};
			//wc.lpfnWndProc   = wndproc;
			wc.hInstance     = GetModuleHandleA(0);
			wc.lpszClassName = DECKARD_DEBUG_CONSOLE_CLASS.data();

			if (not RegisterClassExA(&wc))
				return false;

			handle = CreateWindowExA(
			  ex_style,
			  wc.lpszClassName,
			  DECKARD_DEBUG_CONSOLE_TITLE.data(),
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  width,
			  height,
			  nullptr,
			  nullptr,
			  wc.hInstance,
			  nullptr);

			return true;
		}

	public:
		console() { }
	};
} // namespace deckard
