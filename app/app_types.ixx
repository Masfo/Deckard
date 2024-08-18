module;
#include <Windows.h>
export module deckard.app:types;

import deckard.types;

namespace deckard::app
{
	export enum Key : u32 {
		Escape = VK_ESCAPE,
		F10    = VK_F10,
		F11    = VK_F11,


	};
}
