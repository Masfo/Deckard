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
	using namespace deckard::vec;

	template<typename T>
	class array2d;

	// template<typename T>
	// void dump(const array2d<T>& );

	export template<typename T>
	class array2d
	{
	private:
	public:
		extent<i32> m_extent{1, 1};

		std::vector<T> m_data;

		using hash_type = u64;

		array2d() { resize(m_extent.width, m_extent.height); }

		array2d(i32 w, i32 h) { resize(w, h); }

		array2d(extent<i32> e) { resize(e); }

		T operator[](const ivec2 v) const { return at(v.x, v.y); }

		T& operator[](const ivec2 v) { return at(v.x, v.y); }

		T operator[](const i32 index) const { return m_data[index]; }

		T& operator[](const i32 index) { return m_data[index]; }

		T operator[](const i32 x, const i32 y) const { return at(x, y); }

		T& operator[](const i32 x, const i32 y) { return at(x, y); }

		bool valid(const i32 x, const i32 y) const
		{
			return (x >= 0 && x < m_extent.width) && (y >= 0 && y < m_extent.height);
		}

		std::optional<ivec2> find(const T& value) const
		{
			for (i32 y = 0; y < (i32)height(); ++y)
			{
				for (i32 x = 0; x < (i32)width(); ++x)
				{
					if (get(x, y) == value)
						return ivec2{x, y};
				}
			}
			return {};

		}

		bool valid(const ivec2& v) const { return valid(v.x, v.y); }

		auto data() const { return m_data.data(); }

		auto begin() const { return m_data.begin(); }

		auto end() const { return m_data.end(); }

		T& at(const i32 x, const i32 y) { return get(x, y); }

		T at(const i32 x, const i32 y) const { return get(x, y); }

		T get(const i32 x, const i32 y) const
		{
			assert::check(valid(x, y));

			const auto index = index_from_2d(x, y, m_extent.width);
			return m_data[index];
		}

		T& get(const i32 x, const i32 y)
		{
			assert::check(valid(x, y));

			const auto index = index_from_2d(x, y, m_extent.width);
			return m_data[index];
		}

		auto get(const ivec2& v) const { return get(v.x, v.y); }

		void set(const ivec2& v, const T& value) { set(v.x, v.y, value); }

		void set(i32 x, i32 y, const T& value)
		{
			if (not valid(x, y))
				return;


			const auto index = index_from_2d(x, y, m_extent.width);
			m_data[index]   = value;
		}

		void reset() { fill({}); }

		void clear() { m_data.clear(); }

		i32 count(std::string_view input) const
		{
			i32 count = 0;
			for (i32 y = 0; y < height(); ++y)
				for (i32 x = 0; x < width(); ++x)
					count += input.contains(static_cast<char>(at(x, y))) ? 1 : 0;
			return count;
		}

		void fill(const T& value) { std::ranges::fill(m_data, value); }

		void resize(i32 new_width, i32 new_height)
		{

			assert::check(new_width >= 1);
			assert::check(new_height >= 1);

			m_extent.width  = new_width;
			m_extent.height = new_height;

			m_data.resize(m_extent.width * m_extent.height);
		}

		void resize(extent<i32> e) { resize(e.width, e.height); }

		i32 size() const { return as<i32>(m_data.size()); }

		i32 size_in_bytes() const { return as<i32>(m_data.size() * sizeof(T)); }

		i32 width() const { return m_extent.width; };

		i32 height() const { return m_extent.height; };

		void dump() const
		{

			if constexpr (is_debug_build)
			{
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
			}
		}

		operator hash_type() const noexcept { return hash(); }

		hash_type hash(u64 seed = 0) const
		{
			seed = hash_combine(seed, m_extent.width, m_extent.height);

			for (const auto& i : m_data)
				seed = hash_combine(seed, i);

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

	

} // namespace deckard

export namespace std

{

	template<typename T>
	struct std::hash<deckard::array2d<T>>
	{
		deckard::array2d<T>::hash_type operator()(const deckard::array2d<T>& a) const { return a.hash(); }
	};

} // namespace std
