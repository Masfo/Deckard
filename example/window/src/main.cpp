#include <Windows.h>

import deckard;
using namespace deckard;

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	//
	deckard::initialize();

	app::app app01;

	app01.run();


	deckard::deinitialize();
	return 0;
}
