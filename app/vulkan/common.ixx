export module deckard.vulkan.common;

import std;
import deckard.types;
import deckard.vec;

namespace deckard::vulkan
{

	export struct vertex2d
	{
		vec::vec2 v;
		vec::vec2 uv;
		u32       rgba;       // r | g<<8 | b<<16 | a<<24 -> bytes R,G,B,A in memory (little-endian)
		u32       texture_id; // Index into the context's stable texture table
	};
};
