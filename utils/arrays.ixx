export module deckard.arrays;


import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.debug;
import deckard.math;
import deckard.utils.hash;
import deckard.file;

namespace deckard
{
	using namespace deckard::math;
	using namespace deckard::utils;

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


#ifdef __cpp_multidimensional_subscript
#error("use multisubscript")
		// T operator[](const u32 x, const u32 y) const  {  }
		// T& operator[](const u32 x, const u32 y) {  }
#endif

		T operator()(const u32 x, const u32 y) const { return get(x, y); }

		T& operator()(const u32 x, const u32 y) { return get(x, y); }

		bool valid(const u32 x, const u32 y) const { return (x >= 0 && x < m_extent.width) && (y >= 0 && y < m_extent.height); }

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

		void set(u32 x, u32 y, const T& value)
		{
			if (not valid(x, y))
				return;

			//			assert::check(valid(x, y));

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

		// drawline
		// get line, references to each cell
		// line x = array2d.column(0)
		//  x[0] = 10;


		// line, set single value
		// line, sets value from lambda


		void transpose()
		{
			std::vector<T> transposed{};
			transposed.resize(m_extent.width * m_extent.height);


			for (u32 y = 0; y < m_extent.height; ++y)
			{
				for (u32 x = 0; x < m_extent.width; ++x)
				{
					transposed[index_from_2d(y, x, m_extent.height)] = get(x, y);
				}
			}
			std::swap(m_data, transposed);
			std::swap(m_extent.width, m_extent.height);
		}

		void rotate()
		{
			std::vector<T> rotated(m_data.size());

			for (u32 x = 0; x < m_extent.width; ++x)
			{
				for (u32 y = 0; y < m_extent.height; ++y)
				{
					u32 newIndex      = x * m_extent.width + (m_extent.width - 1 - y);
					rotated[newIndex] = get(x, y);
				}
			}

			// Swap rows and columns
			std::swap(m_extent.width, m_extent.height);
			m_data = rotated;
		}

		template<typename U = char>
		void dump() const
		{
#ifdef _DEBUG

			if (m_extent.width > 32 or m_extent.height > 32)
				return;

			for (u32 y = 0; y < m_extent.height; y++)
			{
				for (u32 x = 0; x < m_extent.width; x++)
				{
					dbg::print("{:}", static_cast<U>(get(x, y)));
				}
				dbg::println();
			}
			dbg::println();
#endif
		}

		void reverse_row(u32 row)
		{
			if (row >= m_extent.width)
				return;

			for (u32 x = 0; x < m_extent.width / 2; ++x)
				std::swap(at(x, row), at(m_extent.height - 1u - x, row));
		}

		void reverse_col(u32 col)
		{
			if (col >= m_extent.height)
				return;

			for (u32 y = 0; y < m_extent.height / 2; ++y)
			{
				std::swap(at(col, y), at(col, m_extent.height - 1u - y));
			}
		}

		operator hash_type() const noexcept { return hash(); }

		hash_type hash(u64 seed = 0) const
		{
			hash_combine(seed, m_extent.width, m_extent.height);

			for (const auto& i : m_data)
				hash_combine(seed, i);

			return seed;
		}

		// find_all, returns points in grid order
		[[nodiscard]] auto find_all(const T to_find) const
		{
			std::vector<uvec2> points;

			for (u32 y = 0; y < m_extent.height; ++y)
			{
				for (u32 x = 0; x < m_extent.width; ++x)
				{
					if (get(x, y) == to_find)
						points.emplace_back(uvec2{x, y});
				}
			}

			std::ranges::sort(points, grid_order<uvec2>());

			return points;
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

		void line(i32 x0, i32 y0, i32 x1, i32 y1, T v)
		{

			i32 dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
			i32 dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
			i32 err = dx + dy, e2; /* error value e_xy */

			for (;;)
			{                      /* loop */
				set(x0, y0, v);
				if (x0 == x1 && y0 == y1)
					break;
				e2 = 2 * err;
				if (e2 >= dy)
				{
					err += dy;
					x0 += sx;
				} /* e_xy+e_x > 0 */
				if (e2 <= dx)
				{
					err += dx;
					y0 += sy;
				} /* e_xy+e_y < 0 */
			}
		}

		void circle(i32 xm, i32 ym, i32 r, T v)
		{
			i32 x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
			do
			{
				set(xm - x, ym + y, v);         /*   I. Quadrant */
				set(xm - y, ym - x, v);         /*  II. Quadrant */
				set(xm + x, ym - y, v);         /* III. Quadrant */
				set(xm + y, ym + x, v);         /*  IV. Quadrant */
				r = err;
				if (r > x)
					err += ++x * 2 + 1;         /* e_xy+e_x > 0 */
				if (r <= y)
					err += ++y * 2 + 1;         /* e_xy+e_y < 0 */
			} while (x < 0);
		}

		bool export_ppm(std::filesystem::path path)
		{
			std::ofstream file(path.generic_string(), std::ios::out | std::ios::binary);
			if (!file)
			{
				dbg::eprintln("ppm: file not opened '{}'", path.generic_string());
				return false;
			}

			u32 colors = *std::max_element(m_data.begin(), m_data.end());

			file << "P5\n" << width() << " " << height() << "\n" << colors << "\n";

			file.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());

			file.close();
			return true;
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

		bool operator()(const u32 x, const u32 y) const { return get(x, y); }

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
