export module deckard.net:types;

import std;
import deckard.types;
import deckard.enums;

namespace deckard::net
{
	export enum class protocol {
		v4,
		v6,
	};
	consteval void enable_bitmask_operations(protocol);

	export enum class transport {
		NUL,
		TCP,
		UDP,
	};

} // namespace deckard::net
