export module deckard.memory:helpers;

import std;
import deckard.types;
namespace deckard::memory
{
	export using bytearray = std::unique_ptr<std::byte[]>;

	export [[nodiscard]] auto make_bytearray(size_t size) -> bytearray { return std::make_unique<std::byte[]>(size); }



}
