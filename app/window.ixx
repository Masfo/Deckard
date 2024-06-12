module;
#include <Windows.h>

export module deckard.app:window;

import std;
import deckard.types;

namespace deckard::app
{
	struct WindowSize
	{
		u32 width{1'920};
		u32 height{1'080};
	};

	class window

	{
	public:
	private:
		WindowSize windowsize{1'920, 1'080};
		HWND       handle{nullptr};
	};

}; // namespace deckard::app
