export module deckard.memory;

import std;
import deckard.types;

namespace deckard::memory
{
	//
	constexpr u32   BLOCK_SIZE_IN_BYTES = 16; // bytes
	constexpr u64 ABSOLUTE_MAX_ALLOCATED = 256_MiB;

	std::vector<u8> global_memory_pool;

	export void initialize(u64 max_size_in_kibibytes) 
	{

	}



}
