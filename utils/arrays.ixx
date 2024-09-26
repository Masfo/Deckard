export module deckard.arrays;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.math;

namespace deckard
{
	using namespace deckard::math;

	export template<typename T>
	class array2d
	{
	private:
		extent<u32> m_extent{1, 1};

		std::vector<T> m_data;

	public:
		array2d() { resize(m_extent.width, m_extent.height); }

		array2d(u32 w, u32 h) { resize(w, h); }

		array2d(extent<u32> e)
			: m_extent(e)
		{
		}

#ifdef __cpp_multidimensional_subscript
#error("use multisubscript")
		// T operator[](const u32 x, const u32 y) const  {  }
		// T& operator[](const u32 x, const u32 y) {  }
#endif

		T operator()(const u32 x, const u32 y) const { return get(x, y); }

		T& operator()(const u32 x, const u32 y) { return get(x, y); }

		T get(const u32 x, const u32 y) const
		{
			assert::check(x <= m_extent.width - 1);
			assert::check(y <= m_extent.height - 1);

			const u32 index = index_from_2d(x, y, m_extent.width);
			return m_data[index];
		}

		T& get(const u32 x, const u32 y)
		{
			assert::check(x <= m_extent.width - 1);
			assert::check(y <= m_extent.height - 1);

			const u32 index = index_from_2d(x, y, m_extent.width);
			return m_data[index];
		}

		void set(u32 x, u32 y, const T& value)
		{
			assert::check(x <= m_extent.width - 1);
			assert::check(y <= m_extent.height - 1);

			const u32 index = index_from_2d(x, y, m_extent.width);
			m_data[index]   = value;
		}

		void clear() { m_data.clear(); }

		void fill(const T& value) { std::ranges::fill(m_data, value); }

		void resize(u32 new_width, u32 new_height)
		{
			assert::check(new_width > 1);
			assert::check(new_height > 1);

			m_extent.width  = new_width;
			m_extent.height = new_height;

			m_data.resize(m_extent.width * m_extent.height);
		}

		void resize(extent<u32> e) { resize(e.width, e.height); }

		u32 size_in_bytes() const { return as<u32>(m_data.size() * sizeof(T)); }

		u32 width() const { return m_extent.width; };

		u32 height() const { return m_extent.height; };

		template<typename U = char>
		void dump() const
		{
#ifdef _DEBUG
			for (u32 y = 0; y < m_extent.height; y++)
			{
				for (u32 x = 0; x < m_extent.width; x++)
				{
					dbg::print("{}", static_cast<U>(get(x, y)));
				}
				dbg::println();
			}
			dbg::println();
#endif
		}
	};

	export template<>
	class array2d<bool>
	{
	private:
		extent<u32> m_extent{1, 1};

		std::vector<u8> m_data;


	public:
		array2d() { resize(m_extent.width, m_extent.height); }

		array2d(extent<u32> e)
			: m_extent(e)
		{
		}

		array2d(u32 w, u32 h) { resize(w, h); }

		bool operator()(const u32 x, const u32 y) const { return get(x, y); }

		bool get(u32 x, u32 y) const
		{
			assert::check(x <= m_extent.width - 1);
			assert::check(y <= m_extent.height - 1);

			const u32 index = index_from_2d(x, y, m_extent.width);

			return (m_data[index / 8] >> (index % 8)) & 1;
		}

		void set(u32 x, u32 y, bool value)
		{
			assert::check(x <= m_extent.width - 1);
			assert::check(y <= m_extent.height - 1);

			const u32 index = index_from_2d(x, y, m_extent.width);

			if (value)
				m_data[index / 8] |= (1 << (index % 8));
			else
				m_data[index / 8] &= ~(1 << (index % 8));
		}

		void flip(u32 x, u32 y) { set(x, y, not get(x, y)); }

		void fill(const bool value) { std::ranges::fill(m_data, static_cast<u8>(value ? 0xFF : 0x00)); }

		void clear() { m_data.clear(); }

		void resize(u32 new_m_width, u32 new_m_height)
		{
			assert::check(new_m_width > 1);
			assert::check(new_m_height > 1);

			m_extent.width  = new_m_width;
			m_extent.height = new_m_height;

			m_data.resize((m_extent.width * m_extent.height + 7) / 8);
		}

		u32 size_in_bytes() const { return as<u32>(m_data.size()); }

		u32 width() const { return m_extent.width; };

		u32 height() const { return m_extent.height; };

		void dump() const
		{
#ifdef _DEBUG
			for (u32 y = 0; y < m_extent.height; y++)
			{
				for (u32 x = 0; x < m_extent.width; x++)
				{
					dbg::print("{:d}", get(x, y));
				}
				dbg::println();
			}
			dbg::println();
#endif
		}
	};


} // namespace deckard
