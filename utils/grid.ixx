export module deckard.grid;

import std;
import deckard.arrays;
import deckard.debug;
import deckard.types;
import deckard.math;
import deckard.utils.hash;
import deckard.as;
import deckard.stringhelper;

namespace deckard
{
	using namespace deckard::math;
	using namespace deckard::vec;

	export enum class filled { yes, no };

	export template<typename F>
	concept floodfill_cb = requires(F fn, u32 x, u32 y) {
		{ fn(x, y) };
	};

	export template<typename T, typename F>
	concept Walkable = std::predicate<F&, const T&>;

	// TODO: non-member drawings

	export constexpr ivec2 north{0, -1};
	export constexpr ivec2 east{1, 0};
	export constexpr ivec2 south{0, 1};
	export constexpr ivec2 west{-1, 0};
	export constexpr ivec2 up{north};
	export constexpr ivec2 left{west};
	export constexpr ivec2 down{south};
	export constexpr ivec2 right{east};
	export constexpr ivec2 vertical(0, 1);
	export constexpr ivec2 horizontal(1, 0);
	export constexpr ivec2 north_east{north + east};
	export constexpr ivec2 south_east{south + east};
	export constexpr ivec2 south_west{south + west};
	export constexpr ivec2 north_west{north + west};

	export template<typename T = u8>
	class grid
	{
	private:
		array2d<T> data;

	public:
		using FunctionOp = const std::function<void(const ivec2, const T)>;

		// friend void dump(const grid&);

		grid() = default;

		grid(u32 w, u32 h) { data.resize(w, h); }

		grid(std::string_view input) { from_string(input); }

		// TODO: vec2

		void from_string(std::string_view input)
		{
			auto lines = string::split<std::string>(input, "\n\t");

			from_lines(lines);
		}

		void from_lines(const std::vector<std::string>& lines)
		{
			if (lines.empty())
			{
				dbg::println("grid::from_lines: empty lines");
				return;
			}

			i32 width  = as<i32>(lines[0].size());
			i32 height = as<i32>(lines.size());


			i32 widths = 0;
			for (const auto& line : lines)
			{
				width = std::max(width, as<i32>(line.size()));
				widths += width;
			}
			bool square = widths == width * height;

			if (not square)
				dbg::println("grid::from_lines: not square: {}x{} vs {}x{}", width, height, lines[0].size(), lines.size());

			if (lines.empty() or width == 0 or height == 0)
				return;

			clear();
			resize(width, height);

			ivec2 pos;
			for (const auto& line : lines)
			{
				pos.x = 0;
				for (const auto& c : line)
				{
					set(pos, c);
					pos.x++;
				}
				pos.y++;
			}
		}

		// TODO: subgrid
		grid<T> subgrid(const u32 x, const u32 y, const u32 w, const u32 h) const
		{
			grid<T> result{};

			if (w == 0 or h == 0)
				return result;

			if (x >= width() or y >= height())
				return result;

			u32 clamped_w = std::min(w, width() - x);
			u32 clamped_h = std::min(h, height() - y);

			result.resize(clamped_w, clamped_h);

			for (u32 yy = 0; yy < clamped_h; ++yy)
			{
				for (u32 xx = 0; xx < clamped_w; ++xx)
				{
					result.set(xx, yy, this->get(x + xx, y + yy));
				}
			}

			return result;
		}

		grid<T> subgrid(const ivec2 pos, const u32 w, const u32 h) const { return subgrid(pos.x, pos.y, w, h); }

		T operator[](const ivec2 pos) const { return data.get(pos.x, pos.y); }

		T& operator[](const ivec2 pos)
		{
			return data.get(pos.x, pos.y);
		}

		T operator[](const u32 x, const u32 y) const { return data.get(x, y); }

		T& operator[](const u32 x, const u32 y)
		{
			return data.get(x, y);
		}

		T& at(const u32 x, const u32 y)
		{
			return data.get(x, y);
		}

		T at(const u32 x, const u32 y) const { return data.get(x, y); }

		T at(const ivec2 pos) const { return data.get(pos.x, pos.y); }

		T& at(const ivec2 pos)
		{
			return data.get(pos.x, pos.y);
		}

		T get(const u32 x, const u32 y) const { return data.get(x, y); }

		//
		T& get(const u32 x, const u32 y)
		{
			return data.get(x, y);
		}

		T get(const ivec2 pos) const { return data.get(pos.x, pos.y); }

		T& get(const ivec2 pos)
		{
			return data.get(pos.x, pos.y);
		}

		std::optional<std::span<const T>> row(const u32 y) const 
		{
			if (y >= height())
				return std::nullopt;

			return std::span<const T>{data.data() + y * width(), width()}; 
		}

		std::optional<std::span<T>> row(const u32 y)
		{
			if (y >= height())
				return std::nullopt;

			return std::span<T>{&data.get(0, y), width()};
		}

		std::optional<T> try_at(const ivec2 pos) const { return valid(pos) ? get(pos) : std::optional<T>{}; }

		void for_each(const FunctionOp& op) const
		{
			for (i32 y = 0; y < as<i32>(height()); ++y)
			{
				for (i32 x = 0; x < as<i32>(width()); ++x)
				{
					if (auto v = try_at(ivec2{x, y}); v.has_value())
					{
						op(ivec2{x, y}, *v);
					}
				}
			}
		}

		auto points() const
		{
			const u32 grid_width = width();
			return std::views::iota(u32{0}, size())
				 | std::views::transform([grid_width](const u32 index)
				   {
					   return ivec2{as<i32>(index % grid_width), as<i32>(index / grid_width)};
				   });
		}

		void for_each_row(const std::function<void(usize, std::span<const T>)>& op)
		{
			for (u32 y = 0; y < height(); ++y)
			{
				if (const auto row_values = row(y))
					op(as<usize>(y), *row_values);
			}
		}



		void for_each_column(const std::function<void(const u32, const std::vector<T>&)>& op) const
		{
			for (u32 x = 0; x < width(); ++x)
			{
				std::vector<T> column{};
				column.reserve(height());
				for (u32 y = 0; y < height(); ++y)
				{
					column.push_back(get(x, y));
				}
				op(x, column);
			}
		}

		void resize(u32 nwidth, u32 nheight) { data.resize(nwidth, nheight); }

		void set(ivec2 pos, const T& value) { data.set(as<u32>(pos.x), as<u32>(pos.y), value); }

		void set(u32 x, u32 y, const T& value) { data.set(x, y, value); }

		void clear() { data.clear(); }

		u32 count(std::string_view input) const { return data.count(input); }

		bool valid(const u32 x, const u32 y) const { return data.valid(x, y); }

		bool valid(ivec2 c) const { return data.valid(c.x, c.y); }

		auto neighbours4(ivec2 pos) const
		{
			return std::array<std::optional<T>, 4>{
			  try_at(pos + north), try_at(pos + east), try_at(pos + south), try_at(pos + west)};
		}

		auto neighbours8(ivec2 pos) const
		{
			return std::array<std::optional<T>, 8>{try_at(pos + north),
												   try_at(pos + north_east),
												   try_at(pos + east),
												   try_at(pos + south_east),
												   try_at(pos + south),
												   try_at(pos + south_west),
												   try_at(pos + west),
												   try_at(pos + north_west)};
		}

		u32 width() const { return data.width(); };

		u32 height() const { return data.height(); };

		u32 size() const { return data.size(); }

		u32 size_in_bytes() const { return data.size_in_bytes(); }

		operator auto() const noexcept { return hash(); }

		auto hash(u64 seed = 0) const { return data.hash(seed); }

		bool operator==(const grid& other) const noexcept { return data == other.data; }

		auto read_from_line(const ivec2 from, const ivec2 to) -> std::vector<T>
		{
			return read_from_line(from.x, from.y, to.x, to.y);
		}

		auto read_from_line(i32 x1, i32 y1, i32 x2, i32 y2) -> std::vector<T>
		{
			std::vector<T> ret{};

			i32 dx = std::abs(x2 - x1);
			i32 dy = std::abs(y2 - y1);
			i32 sx = (x1 < x2) ? 1 : -1;
			i32 sy = (y1 < y2) ? 1 : -1;

			if (dx == 0 and dy == 0)
			{
				ret.push_back(at(x1, y1));
				return ret;
			}

			i32 err = dx - dy;

			while (true)
			{
				if (x1 < 0 or x1 >= as<i32>(width()) or y1 < 0 or y1 >= as<i32>(height()))
					break;

				ret.push_back(get(x1, y1));

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
			return ret;
		}

		void line(i32 x1, i32 y1, i32 x2, i32 y2, const T& v)
		{

			line(x1, y1, x2, y2, [v] { return v; });
		}

		void line(const ivec2 from, const ivec2 to, const T& v) { line(from.x, from.y, to.x, to.y, v); }

		template<std::invocable Func>
		void line(const ivec2 from, const ivec2 to, const Func&& cb)
		{
			line(from.x, from.y, to.x, to.y, std::move(cb));
		}

		template<std::invocable Func>
		void line(i32 x1, i32 y1, i32 x2, i32 y2, const Func&& cb)
		{

			static_assert(
			  std::is_same_v<T, std::invoke_result_t<Func>>, "Callback must return the same type T as the array2d<T>");

			i32 dx = std::abs(x2 - x1);
			i32 dy = std::abs(y2 - y1);
			i32 sx = (x1 < x2) ? 1 : -1;
			i32 sy = (y1 < y2) ? 1 : -1;

			if (dx == 0 and dy == 0)
			{
				set(x1, y1, cb());
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

		// pos, x2
		void hline(const ivec2 pos, const u32 x2, const T& v) { hline(as<u32>(pos.x), as<u32>(pos.y), x2, v); }

		// x,y,y2
		void vline(u32 x, u32 y1, u32 y2, const T& v)
		{
			if (y1 > y2)
				std::swap(y1, y2);

			for (u32 y = y1; y <= y2; ++y)
				set(x, y, v);
		}

		// pos, y2
		void vline(const ivec2 pos, const u32 y2, const T& v) { vline(as<u32>(pos.x), as<u32>(pos.y), y2, v); }

		void rectangle(const ivec2 p1, const ivec2 p2, const T& v, filled f = filled::yes)
		{
			rectangle(as<u32>(p1.x), as<u32>(p1.y), as<u32>(p2.x), as<u32>(p2.y), v, f);
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
			const i32 cx = as<i32>(xm);
			const i32 cy = as<i32>(ym);

			i32 x = -as<i32>(r), y = 0, err = 2 - 2 * as<i32>(r);
			do
			{
				// set() ignores negative (wrapped) coordinates
				set(static_cast<u32>(cx - x), static_cast<u32>(cy + y), v);
				set(static_cast<u32>(cx - y), static_cast<u32>(cy - x), v);
				set(static_cast<u32>(cx + x), static_cast<u32>(cy - y), v);
				set(static_cast<u32>(cx + y), static_cast<u32>(cy + x), v);
				const i32 e = err;
				if (e > x)
					err += ++x * 2 + 1;
				if (e <= y)
					err += ++y * 2 + 1;
			} while (x < 0);
		}

		void circle(const ivec2 center, const u32 r, const T& v) { circle(as<u32>(center.x), as<u32>(center.y), r, v); }

		void fill(const T& value) { data.fill(value); }

		template<floodfill_cb Func>
		u32 floodfill(const ivec2 pos, const Func&& cb)
		{
			return floodfill(as<u32>(pos.x), as<u32>(pos.y), std::move(cb));
		}

		u32 floodfill(const ivec2 pos, const T& r) { return floodfill(as<u32>(pos.x), as<u32>(pos.y), r); }

		template<floodfill_cb Func>
		u32 floodfill(u32 x, u32 y, const Func&& cb)
		{
			static_assert(std::is_same_v<T, std::invoke_result_t<Func, u32, u32>>,
						  "Callback must be have signature fn(u32,u32) -> T (array2d<T>)");

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

		// find_all
		[[nodiscard]] auto find_all(const T to_find) const
		{
			std::vector<ivec2> points;

			for (auto y = 0u; y < height(); ++y)
			{
				for (auto x = 0u; x < width(); ++x)
				{
					if (data.get(x, y) == to_find)
						points.emplace_back(ivec2{as<i32>(x), as<i32>(y)});
				}
			}

			std::ranges::sort(points, grid_order);

			return points;
		}

		void reverse_row(u32 row)
		{
			if (row >= height())
				return;

			for (u32 x = 0; x < width() / 2; ++x)
				std::swap(data.at(x, row), data.at(width() - 1u - x, row));
		}

		void reverse_col(u32 col)
		{
			if (col >= width())
				return;

			for (u32 y = 0; y < height() / 2; ++y)
				std::swap(data.at(col, y), data.at(col, height() - 1u - y));
		}

		void transpose()
		{
			array2d<T> transposed;
			transposed.resize(height(), width());

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


		void rotate_left() { rotate_ccw(); }

		void rotate_right() { rotate_cw(); }

		std::string print() const
		{
			std::string ret;
			for (u32 y = 0; y < height(); ++y)
			{
				for (u32 x = 0; x < width(); ++x)
				{
					ret.push_back(get(x, y));
				}
				ret.push_back('\n');
			}

			return ret;
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
			data.dump();
		}
	};

	// non-members

	// #############################
	// AStar #######################


	export template<typename T, typename WalkableFn>
	requires Walkable<T, WalkableFn>
	auto a_star(const grid<T>& g, const ivec2 start, const ivec2 goal, WalkableFn&& walkable)
	  -> std::optional<std::vector<ivec2>>
	{
		const auto can_walk = [&](const ivec2 p) { return g.valid(p) and walkable(g.get(p)); };

		if (not can_walk(start) or not can_walk(goal))
			return std::nullopt;

		// per-cell search state, indexed by position
		struct cell
		{
			u32   cost = std::numeric_limits<u32>::max(); // best known steps from start
			ivec2 parent{-1, -1};                         // where we came from
		};

														  // open-list entry, f = cost + heuristic
		struct node
		{
			ivec2 pos{};
			u32   cost{0}; // cost from start
			u32   f{0};    // f = cost + heuristic
		};

		const u32  width     = g.width();
		const auto heuristic = [goal](const ivec2 p) { return as<u32>(std::abs(p.x - goal.x) + std::abs(p.y - goal.y)); };
		const auto sorter    = [](const node& a, const node& b) { return a.f > b.f; }; // min-heap on f

		std::vector<cell>                                              cells(g.size());
		std::priority_queue<node, std::vector<node>, decltype(sorter)> queue(sorter);

		//
		const auto build_path = [&]
		{
			std::vector<ivec2> path;
			for (ivec2 p = goal; p != start; p = cells[index_from_2d(p, width)].parent)
				path.push_back(p);

			path.push_back(start);
			std::ranges::reverse(path);
			return path;
		};

		cells[index_from_2d(start, width)].cost = 0;
		queue.push({start, 0, heuristic(start)});

		constexpr std::array<ivec2, 4> directions{north, east, south, west};

		while (not queue.empty())
		{
			const node current = queue.top();
			queue.pop();

			// stale entry: a cheaper route to this cell was found after it was pushed
			if (current.cost > cells[index_from_2d(current.pos, width)].cost)
				continue;

			if (current.pos == goal)
				return build_path();

			for (const ivec2 direction : directions)
			{
				const ivec2 next      = current.pos + direction;
				const u32   next_cost = current.cost + 1;

				if (not can_walk(next))
					continue;

				cell& next_cell = cells[index_from_2d(next, width)];
				if (next_cost >= next_cell.cost)
					continue;

				next_cell = {.cost = next_cost, .parent = current.pos};
				queue.push({next, next_cost, next_cost + heuristic(next)});
			}
		}

		return std::nullopt;
	}

	// default u8 grid: 0 is walkable, anything else is a wall
	export auto a_star(const grid<u8>& g, const ivec2 start, const ivec2 goal) -> std::optional<std::vector<ivec2>>
	{
		return a_star(g, start, goal, [](const u8 v) { return v == 0; });
	}

	// #############################
	// BFS #########################


	// BFS on a 4-connected grid. Returns a shortest path from start to goal (both included), or nullopt if no path exists.
	export template<typename T, typename WalkableFn>
	requires Walkable<T, WalkableFn>
	auto bfs(const grid<T>& g, const ivec2 start, const ivec2 goal, WalkableFn&& walkable)
	  -> std::optional<std::vector<ivec2>>
	{
		if (not g.valid(start) or not g.valid(goal))
			return std::nullopt;

		if (not walkable(g.get(start)) or not walkable(g.get(goal)))
			return std::nullopt;

		const u32 w = g.width();

		const auto to_index = [w](const ivec2 p) { return as<u32>(p.y) * w + as<u32>(p.x); };
		const auto to_point = [w](const u32 i) { return ivec2{as<i32>(i % w), as<i32>(i / w)}; };

		constexpr u32 none = std::numeric_limits<u32>::max();

		std::vector<u32>  parent(g.size(), none);
		std::vector<bool> visited(g.size(), false);
		std::queue<u32>   open;

		const u32 start_index = to_index(start);
		visited[start_index]  = true;
		open.push(start_index);

		constexpr std::array<ivec2, 4> directions{north, east, south, west};

		while (not open.empty())
		{
			const u32   current = open.front();
			const ivec2 pos     = to_point(current);
			open.pop();

			if (pos == goal)
			{
				std::vector<ivec2> path;
				for (u32 i = current; i != none; i = parent[i])
					path.push_back(to_point(i));

				std::ranges::reverse(path);
				return path;
			}

			for (const auto& direction : directions)
			{
				const ivec2 next = pos + direction;

				if (not g.valid(next) or not walkable(g.get(next)))
					continue;

				const u32 next_index = to_index(next);
				if (visited[next_index])
					continue;

				visited[next_index] = true;
				parent[next_index]  = current;
				open.push(next_index);
			}
		}

		return std::nullopt;
	}

	// default u8 grid: 0 is walkable, anything else is a wall
	export auto bfs(const grid<u8>& g, const ivec2 start, const ivec2 goal) -> std::optional<std::vector<ivec2>>
	{
		return bfs(g, start, goal, [](const u8 v) { return v == 0; });
	}


} // namespace deckard

// Provide std::hash specialization for deckard::grid
export namespace std
{
	template<typename T>
	struct hash<deckard::grid<T>>
	{
		size_t operator()(const deckard::grid<T>& g) const noexcept { return static_cast<size_t>(g.hash()); }
	};
} // namespace std

// Provide std::formatter specialization for deckard::grid
export namespace std
{
	template<>
	struct formatter<deckard::grid<char>>
	{
		constexpr auto parse(std::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

		auto format(const deckard::grid<char>& g, std::format_context& ctx) const -> decltype(ctx.out())
		{
			return std::format_to(ctx.out(), "{}", g.print());
		}
	};
} // namespace std
