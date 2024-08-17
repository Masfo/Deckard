module;
#include <Windows.h>
export module deckard.app:types;

import deckard.types;

namespace deckard::app
{
	export enum Key : u32 {
		Escape = VK_ESCAPE,

	};
}
