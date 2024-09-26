export module deckard.graph:directed;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.arrays;

namespace deckard::graph
{

	template<typename T>
	class directed_graph
	{
	private:
		T             data{};
		array2d<bool> adjacency;

	public:
	};

} // namespace deckard::graph
