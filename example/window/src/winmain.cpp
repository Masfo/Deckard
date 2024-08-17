#include <Windows.h>

import deckard;
using namespace deckard;

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//
	deckard::initialize();
	{
		app::vulkanapp app01({.title = "Example 01", .width = 1'280, .height = 720});

		app01.run();
	}

	deckard::deinitialize();
	return 0;
}
