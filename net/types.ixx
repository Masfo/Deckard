export module deckard.net:types;

import std;
import deckard.types;
import deckard.enums;

namespace deckard::net
{
	export enum class protocol : u8 {
		v4,
		v6,
	};
	consteval void enable_bitmask_operations(protocol);

	export enum class transport : u8 {
		nul,
		tcp,
		udp,
	};

} // namespace deckard::net
