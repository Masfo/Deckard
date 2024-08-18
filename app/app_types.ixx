module;
#include <Windows.h>
export module deckard.app:types;

import deckard.types;

namespace deckard::app
{
	export enum Key : u32 {
		Escape = VK_ESCAPE,

		F1  = VK_F1,
		F10 = VK_F10,
		F11 = VK_F11,

		// Numpad
		NUM1 = VK_NUMPAD1,
		NUM2 = VK_NUMPAD2,
		NUM3 = VK_NUMPAD3,


	};
}
