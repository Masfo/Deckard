export module deckard.arrays;


import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.math;
import deckard.utils.hash;
import deckard.file;
import deckard.function_ref;
import deckard.helpers;


namespace deckard
{
	using namespace deckard::math;
	using namespace deckard::utils;

	template<typename T>
	class array2d;

	// template<typename T>
	// void dump(const array2d<T>& );

	export template<typename T>
	class array2d
	{
	private:
	public:
		extent<u32> m_extent{1, 1};

		std::vector<T> m_data;

		using hash_type = u64;

		array2d() { resize(m_extent.width, m_extent.height); }

		array2d(u32 w, u32 h) { resize(w, h); }

		array2d(extent<u32> e)
			: m_extent(e)
		{
		}

		T operator[](const ivec2 v) const { return at(v.x, v.y); }

		T& operator[](const ivec2 v) { return at(v.x, v.y); }

		T operator[](const u32 index) const { return m_data[index]; }

		T& operator[](const u32 index) { return m_data[index]; }

		T operator[](const u32 x, const u32 y) const { return at(x, y); }

		T& operator[](const u32 x, const u32 y) { return at(x, y); }

		bool valid(const u32 x, const u32 y) const { return (x >= 0 && x < m_extent.width) && (y >= 0 && y < m_extent.height); }

		bool valid(const ivec2& v) const { return valid(v.x, v.y); }

		auto data() const { return m_data.data(); }

		auto begin() const { return m_data.begin(); }

		auto end() const { return m_data.end(); }

		T& at(const u32 x, const u32 y) { return get(x, y); }

		T at(const u32 x, const u32 y) const { return get(x, y); }

		T get(const u32 x, const u32 y) const
		{
			assert::check(valid(x, y));

			const u32 index = index_from_2d(x, y, m_extent.width);
			return m_data[index];
		}

		T& get(const u32 x, const u32 y)
		{
			assert::check(valid(x, y));

			const u32 index = index_from_2d(x, y, m_extent.width);
			return m_data[index];
		}

		void set(const ivec2& v, const T& value) { set(v.x, v.y, value); }

		void set(u32 x, u32 y, const T& value)
		{
			if (not valid(x, y))
				return;


			const u32 index = index_from_2d(x, y, m_extent.width);
			m_data[index]   = value;
		}

		void reset() { fill({}); }

		void clear() { m_data.clear(); }

		u32 count(std::string_view input) const
		{
			u32 count = 0;
			for (u32 y = 0; y < height(); ++y)
				for (u32 x = 0; x < width(); ++x)
					count += input.contains(at(x, y)) ? 1 : 0;
			return count;
		}

		void fill(const T& value) { std::ranges::fill(m_data, value); }

		void resize(u32 new_width, u32 new_height)
		{

			assert::check(new_width >= 1);
			assert::check(new_height >= 1);

			m_extent.width  = new_width;
			m_extent.height = new_height;

			m_data.resize(m_extent.width * m_extent.height);
		}

		void resize(extent<u32> e) { resize(e.width, e.height); }

		u32 size() const { return as<u32>(m_data.size()); }

		u32 size_in_bytes() const { return as<u32>(m_data.size() * sizeof(T)); }

		u32 width() const { return m_extent.width; };

		u32 height() const { return m_extent.height; };

		void dump() const
		{
#ifdef _DEBUG
			constexpr u32 MIN_DUMPSIZE = 256;

			u32 w = std::min(width(), MIN_DUMPSIZE);


			u32 maxunit{1};
			for (i32 i : upto(count_digits(w) - 1))
				maxunit *= 10;

			i32 dwidth = count_digits(w);

			for (int units = maxunit; units >= 1; units /= 10)
			{
				dbg::print("{:{}}", "", dwidth + 2);

				for (int i : upto(w))
				{
					int dv = i / units;
					if (units == 1)
						dbg::print("{}", i % 10);
					else
					{
						if (i < units)
							dbg::print(" ");
						else
							dbg::print("{}", dv % 10);
					}
				}
				dbg::println();
			}


			for (u32 y = 0; y < height(); y++)
			{
				dbg::print("{:0{}}. ", y, dwidth);

				for (u32 x = 0; x < width(); x++)
					dbg::print("{:}", static_cast<char>(get(x, y)));

				dbg::println();
			}
			dbg::println();
#endif
		}

		operator hash_type() const noexcept { return hash(); }

		hash_type hash(u64 seed = 0) const
		{
			hash_combine(seed, m_extent.width, m_extent.height);

			for (const auto& i : m_data)
				hash_combine(seed, i);

			return seed;
		}

		void swap(array2d<T>& other) noexcept
		{
			using std::swap;
			swap(m_extent.width, other.m_extent.width);
			swap(m_extent.height, other.m_extent.height);
			swap(m_data, other.m_data);
		}

		array2d<T>& operator=(array2d<T> lhs) noexcept
		{
			swap(lhs);
			return *this;
		}

		bool operator==(const array2d<T>& lhs) const noexcept
		{
			return m_extent.width == lhs.m_extent.width && m_extent.height == lhs.m_extent.height && m_data == lhs.m_data;
		}
	};

	export template<>
	class array2d<bool>
	{
	private:
		extent<u32> m_extent{1, 1};

		std::vector<u8> m_data;


	public:
		using hash_type = u64;

		array2d() { resize(m_extent.width, m_extent.height); }

		array2d(extent<u32> e)
			: m_extent(e)
		{
		}

		array2d(u32 w, u32 h) { resize(w, h); }

		bool valid(const u32 x, const u32 y) const { return (x >= 0 && x < m_extent.width) && (y >= 0 && y < m_extent.height); }

		bool operator[](const u32 index) const { return (m_data[index / 8] >> (index % 8)) & 1; }

		bool operator[](const u32 x, const u32 y) const { return at(x, y); }

		bool at(const u32 x, const u32 y) const { return get(x, y); }

		bool get(u32 x, u32 y) const
		{
			assert::check(valid(x, y));

			const u32 index = index_from_2d(x, y, m_extent.width);

			return (m_data[index / 8] >> (index % 8)) & 1;
		}

		void set(u32 x, u32 y, bool value)
		{
			assert::check(valid(x, y));

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
			assert::check(new_m_width >= 1);
			assert::check(new_m_height >= 1);

			m_extent.width  = new_m_width;
			m_extent.height = new_m_height;

			m_data.resize((m_extent.width * m_extent.height + 7) / 8);
		}

		auto data() const { return m_data.data(); }

		u32 size_in_bytes() const { return as<u32>(m_data.size()); }

		u32 width() const { return m_extent.width; };

		u32 height() const { return m_extent.height; };
#if 0
		void dump()
		{
#ifdef _DEBUG
			constexpr u32 MIN_DUMPSIZE = 256;

			u32 width  = std::min(width(), MIN_DUMPSIZE);
			u32 height = std::min(height(), MIN_DUMPSIZE);

			int maxd = width;

			u32 maxunit{1};
			for (i32 i : upto_from(0, digits(maxd) - 1))
				maxunit *= 10;

			i32 dwidth = digits(maxd);

			for (int units = maxunit; units >= 1; units /= 10)
			{
				dbg::print("{:{}}", "", dwidth + 1);

				for (int i : upto(maxd))
				{
					int dv = i / units;
					if (units == 1)
						dbg::print("{}", i % 10);
					else
					{
						if (i < units)
							dbg::print(" ");
						else
							dbg::print("{}", dv % 10);
					}
				}
				dbg::println();
			}


			for (u32 y = 0; y < height; y++)
			{
				dbg::print("{:0{}}. ", y, dwidth);

				for (u32 x = 0; x < width; x++)
					dbg::print("{:}", buffer.get(x, y) ? 1 : 0);
				dbg::println();
			}
			dbg::println();
#endif
		}
#endif

		operator hash_type() const noexcept { return hash(); }

		hash_type hash(u64 seed = 0) const
		{
			hash_combine(seed, m_extent.width, m_extent.height);

			for (const auto& i : m_data)
				hash_combine(seed, i);

			return seed;
		}

		void swap(array2d<bool>& other) noexcept
		{
			using std::swap;
			swap(m_extent.width, other.m_extent.width);
			swap(m_extent.height, other.m_extent.height);
			swap(m_data, other.m_data);
		}

		array2d<bool>& operator=(array2d<bool> lhs) noexcept
		{
			swap(lhs);
			return *this;
		}

		bool operator==(const array2d<bool>& lhs) const noexcept
		{
			return m_extent.width == lhs.m_extent.width && m_extent.height == lhs.m_extent.height && m_data == lhs.m_data;
		}
	};

} // namespace deckard

export namespace std

{

	template<typename T>
	struct std::hash<deckard::array2d<T>>
	{
		deckard::array2d<T>::hash_type operator()(const deckard::array2d<T>& a) const { return a.hash(); }
	};

} // namespace std
