export module deckard.grid;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;

namespace deckard::grid
{
	export template<typename T>
	class grid
	{
	private:
		extent<u32> extent{1, 1};

		std::vector<T> grid;

	public:
		grid() { resize(extent.width, extent.height); }

		grid(u32 w, u32 h) { resize(w, h); }

		bool get(u32 x, u32 y) const
		{
			todo();
			return true;
		}

		void set(u32 x, u32 y, bool value) { todo(); }

		void clear() { grid.clear(); }

		void resize(u32 new_width, u32 new_height)
		{
			assert::check(new_width > 1);
			assert::check(new_height > 1);

			extent.width  = new_width;
			extent.height = new_height;

			grid.resize(extent.width * extent.height);
		}

		u32 size_in_bytes() const { return as<u32>(grid.size()); }

		u32 width() const { return extent.width; };

		u32 height() const { return extent.height; };
	};

	export template<>
	class grid<bool>
	{
	private:
		extent<u32> extent{1, 1};

		std::vector<u8> m_grid;


	public:
		grid() { resize(extent.width, extent.height); }

		grid(u32 w, u32 h) { resize(w, h); }

		bool get(u32 x, u32 y) const
		{
			assert::check(x <= extent.width - 1);
			assert::check(y <= extent.height - 1);

			return (m_grid[(x * extent.width + y) / 8] >> ((x * extent.width + y) % 8)) & 1;
		}

		void set(u32 x, u32 y, bool value)
		{
			assert::check(x <= extent.width - 1);
			assert::check(y <= extent.height - 1);

			if (value)
				m_grid[(x * extent.width + y) / 8] |= (1 << ((x * extent.width + y) % 8));
			else
				m_grid[(x * extent.width + y) / 8] &= ~(1 << ((x * extent.width + y) % 8));
		}

		void flip(u32 x, u32 y) { set(x, y, not get(x, y)); }

		void clear() { m_grid.clear(); }

		void resize(u32 new_m_width, u32 new_m_height)
		{
			assert::check(new_m_width > 1);
			assert::check(new_m_height > 1);

			extent.width  = new_m_width;
			extent.height = new_m_height;

			m_grid.resize((extent.width * extent.height + 7) / 8);
		}

		u32 size_in_bytes() const { return as<u32>(m_grid.size()); }

		u32 width() const { return extent.width; };

		u32 height() const { return extent.height; };

		void dump() const
		{
#ifdef _DEBUG
			for (u32 y = 0; y < extent.height; y++)
			{
				for (u32 x = 0; x < extent.width; x++)
				{
					dbg::print("{:d}", get(x, y));
				}
				dbg::println();
			}
			dbg::println();
#endif
		}
	};


} // namespace deckard::grid
