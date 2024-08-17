#include <Windows.h>

import deckard;
using namespace deckard;

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//
	deckard::initialize();


	deckard::deinitialize();
	return 0;
}
