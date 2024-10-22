export module deckard.grid;

import std;
import deckard.arrays;
import deckard.debug;
import deckard.types;
import deckard.math;
import deckard.utils.hash;

namespace deckard
{
	using namespace deckard::math;

	export enum class filled { yes, no };

	export template<typename F>
	concept floodfill_cb = requires(F fn, u32 x, u32 y) {
		{ fn(x, y) };
	};

	// TODO: non-member drawings

	export template<typename T = u8, typename BufferType = array2d<T>>
	class grid
	{
	private:
		BufferType data;

	public:
		// friend void dump(const grid&);

		grid() = default;

		grid(u32 w, u32 h) { data.resize(w, h); }

		// TODO: vec2


		T operator[](const u32 x, const u32 y) const { return data.get(x, y); }

		T& operator[](const u32 x, const u32 y)
		requires not std::is_same_v<T, bool>

		{
			return data.get(x, y);
		}

		T& at(const u32 x, const u32 y)
		requires not std::is_same_v<T, bool>
		{
			return data.get(x, y);
		}

		T at(const u32 x, const u32 y) const { return data.get(x, y); }

		T get(const u32 x, const u32 y) const { return data.get(x, y); }

		T& get(const u32 x, const u32 y)
		requires not std::is_same_v<T, bool>
		{
			return data.get(x, y);
		}

		void resize(u32 nwidth, u32 nheight) { data.resize(nwidth, nheight); }

		void set(u32 x, u32 y, const T& value) { data.set(x, y, value); }

		void clear() { data.clear(); }

		u32 count(std::string_view input) const { return data.count(input); }

		bool valid(const u32 x, const u32 y) const { return data.valid(x, y); }

		u32 width() const { return data.width(); };

		u32 height() const { return data.height(); };

		u32 size() const { return data.size(); }

		u32 size_in_bytes() const { return data.size_in_bytes(); }

		operator auto() const noexcept { return hash(); }

		auto hash(u64 seed = 0) const { return data.hash(seed); }

		void line(i32 x1, i32 y1, i32 x2, i32 y2, const T& v)
		{

			line(x1, y1, x2, y2, [v] { return v; });
		}

		template<std::invocable Func>
		void line(i32 x1, i32 y1, i32 x2, i32 y2, const Func&& cb)
		{

			static_assert(std::is_same_v<T, std::invoke_result_t<Func>>, "Callback must return the same type T as the array2d<T>");

			i32 dx = std::abs(x2 - x1);
			i32 dy = std::abs(y2 - y1);
			i32 sx = (x1 < x2) ? 1 : -1;
			i32 sy = (y1 < y2) ? 1 : -1;

			if(dx == 0 and dy == 0)
			{
				set(x1,x2, cb());
				return;
			}

			if (dx == 0)
			{
				vline(x1, y1, y2, cb());
				return;
			}

			if (dy == 0)
			{
				hline(x1, y1, x2, cb());
				return;
			}


			i32 err = dx - dy;

			while (true)
			{
				set(x1, y1, cb());

				if (x1 == x2 && y1 == y2)
					break;

				i64 e2 = 2 * err;

				if (e2 > -dy)
				{
					err -= dy;
					x1 += sx;
				}

				if (e2 < dx)
				{
					err += dx;
					y1 += sy;
				}
			}
		}

		// x,y,x2
		void hline(u32 x1, const u32 y, u32 x2, const T& v)
		{
			if (x1 > x2)
				std::swap(x1, x2);

			for (u32 x = x1; x <= x2; ++x)
				set(x, y, v);
		}

		// x,y,y2
		void vline(u32 x, u32 y1, u32 y2, T const& v)
		{
			if (y1 > y2)
				std::swap(y1, y2);

			for (u32 y = y1; y <= y2; ++y)
				set(x, y, v);
		}

		void rectangle(u32 x1, u32 y1, u32 x2, u32 y2, const T& v, filled f = filled::yes)
		{
			if (x1 > x2)
				std::swap(x1, x2);
			if (y1 > y2)
				std::swap(y1, y2);

			if (f == filled::yes)
			{
				for (u32 y = y1; y < y2 + 1; ++y)
					for (u32 x = x1; x < x2 + 1; ++x)
						set(x, y, v);
			}
			else
			{
				line(x1, y1, x2, y1, v); // T
				line(x1, y2, x2, y2, v); // B

				line(x1, y1, x1, y2, v); // L
				line(x2, y1, x2, y2, v); // R
			}
		}

		void circle(u32 xm, u32 ym, u32 r, const T& v)

		{
			u32 x = -r, y = 0, err = 2 - 2 * r;
			do
			{
				set(xm - x, ym + y, v);
				set(xm - y, ym - x, v);
				set(xm + x, ym - y, v);
				set(xm + y, ym + x, v);
				r = err;
				if (r > x)
					err += ++x * 2 + 1;
				if (r <= y)
					err += ++y * 2 + 1;
			} while (x < 0);
		}

		void fill(const T& value) { data.fill(value); }

		template<floodfill_cb Func>
		u32 floodfill(u32 x, u32 y, const Func&& cb)
		{
			static_assert(
			  std::is_same_v<T, std::invoke_result_t<Func, u32, u32>>, "Callback must be have signature fn(u32,u32) -> T (array2d<T>)");

			T value = data.get(x, y);

			std::set<std::pair<u32, u32>>   seen;
			std::deque<std::pair<u32, u32>> to_visit;

			to_visit.push_back({x, y});

			u32 count{};

			while (not to_visit.empty())
			{
				const auto [dx, dy] = to_visit.front();
				to_visit.pop_front();

				if (not valid(dx, dy))
					continue;

				if (seen.contains({dx, dy}))
					continue;

				seen.insert({dx, dy});

				if (get(dx, dy) != value)
					continue;

				set(dx, dy, cb(dx, dy));
				count += 1;

				to_visit.push_back({dx + 1, dy + 0});
				to_visit.push_back({dx + 0, dy + 1});
				to_visit.push_back({dx - 1, dy - 0});
				to_visit.push_back({dx - 0, dy - 1});
			}

			return count;
		}

		u32 floodfill(u32 x, u32 y, const T& r)
		{
			return floodfill(x, y, [r](u32, u32) -> T { return r; });
		}

		// returns points in grid order
		[[nodiscard]] auto find_all(const T to_find) const
		{
			std::vector<uvec2> points;

			for (u32 y = 0; y < height(); ++y)
			{
				for (u32 x = 0; x < width(); ++x)
				{
					if (data.get(x, y) == to_find)
						points.emplace_back(uvec2{x, y});
				}
			}

			std::ranges::sort(points, grid_order<uvec2>());

			return points;
		}

		void reverse_row(u32 row)
		{
			if (row >= width())
				return;

			for (u32 x = 0; x < width() / 2; ++x)
			{
				if constexpr (std::is_same_v<T, bool>)
				{
					// no references to bool

					auto temp = data.get(x, row);
					data.set(x, row, data.get(height() - 1u - x, row));
					data.set(height() - 1u - x, row, temp);
				}
				else
				{
					std::swap(data.at(x, row), data.at(height() - 1u - x, row));
				}
			}
		}

		void reverse_col(u32 col)
		{
			if (col >= height())
				return;

			for (u32 y = 0; y < height() / 2; ++y)
			{
				if constexpr (std::is_same_v<T, bool>)
				{
					// no references to bool
					auto temp = data.get(col, y);
					data.set(col, y, data.get(col, height() - 1u - y));
					data.set(col, height() - 1u - y, temp);
				}
				else
				{
					std::swap(data.at(col, y), data.at(col, height() - 1u - y));
				}
			}
		}

		void transpose()
		{
			array2d<T> transposed;
			transposed.resize(width(), height());

			for (u32 y = 0; y < height(); ++y)
			{
				for (u32 x = 0; x < width(); ++x)
				{
					transposed.set(y, x, get(x, y));
				}
			}
			std::swap(data, transposed);
		}

		void reverse_rows()
		{
			for (u32 y = 0; y < height(); ++y)
				reverse_row(y);
		}

		void reverse_columns()
		{
			for (u32 x = 0; x < width(); ++x)
				reverse_col(x);
		}

		void rotate_cw()
		{
			transpose();
			reverse_rows();
		}

		void rotate_ccw()
		{
			transpose();
			reverse_columns();
		}

		// TODO: serialize to/from disk
		// TODO: non-member export
		bool export_ppm(std::filesystem::path path)
		{
			std::ofstream file(path.generic_string(), std::ios::out | std::ios::binary);
			if (!file)
			{
				dbg::eprintln("ppm: file not opened '{}'", path.generic_string());
				return false;
			}

			u32 colors = *std::max_element(data.begin(), data.end());

			file << "P5\n" << width() << " " << height() << "\n" << colors << "\n";

			file.write(reinterpret_cast<const char*>(data.data()), data.size_in_bytes());

			file.close();
			return true;
		}

		template<typename U = char>
		void dump() const
		{
			data.dump<U>();
		}
	};

	// non-members


	export template<typename T>
	void dump(const grid<T>& buffer)
	{
		// dump<T>(buffer.data);
	}
} // namespace deckard
