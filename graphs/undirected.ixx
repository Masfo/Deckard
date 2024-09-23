export module deckard.graph:undirected;

import std;
import deckard.types;
import deckard.assert;

namespace deckard::graph
{
	template<typename T>
	class grid2d
	{
	private:
		std::vector<std::vector<T>> grid;

	public:
	};

	template<>
	class grid2d<bool>
	{
	private:
		u32 width{1};
		u32 height{1};

		std::vector<u8> grid;


	public:
		grid2d<bool>() { resize(width, height); }

		void clear() { grid.clear(); };

		void resize(u32 w, u32 h)
		{
			assert::check(w > 0);
			assert::check(h > 0);

			width  = w;
			height = h;

			const u32 byte_width  = width / 8;
			const u32 byte_height = height / 8;
			clear();

			grid.resize(byte_width * byte_height);
		}

		// 8x8   -> (0,0)-(7,7)
		// 16x16 -> (0,0)-(15,15)
	};

	class undirected_graph
	{
	private:
	public:
	};

} // namespace deckard::graph
