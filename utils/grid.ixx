export module deckard.grid;

import std;
import deckard.assert;
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
	concept floodfill_cb = requires(F fn, ivec2 pos) {
		{ fn(pos) };
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

	export constexpr std::array<ivec2, 4> directions4{north, east, south, west};
	export constexpr std::array<ivec2, 8> directions8{
	  north, north_east, east, south_east, south, south_west, west, north_west};

	export namespace grid3d_directions
	{
		inline constexpr ivec3 top{0, -1, 0};
		inline constexpr ivec3 bottom{0, 1, 0};
		inline constexpr ivec3 left{-1, 0, 0};
		inline constexpr ivec3 right{1, 0, 0};
		inline constexpr ivec3 front{0, 0, -1};
		inline constexpr ivec3 back{0, 0, 1};
	} // namespace grid3d_directions

	export template<typename T = u8>
	class grid
	{
	private:
		array2d<T>                data;
		std::unordered_set<ivec2> visited;

	public:
		using FunctionOp = const std::function<bool(const ivec2, const T)>;

		// friend void dump(const grid&);

		grid() = default;

		grid(i32 w, i32 h) { data.resize(w, h); }

		grid(std::string_view input) { from_string(input); }

		// visit
		void clear_visited() { visited.clear(); }

		i32 visited_count() const { return as<i32>(visited.size()); }

		[[nodiscard]] bool is_visited(const ivec2 pos) const { return visited.contains(pos); }

		void visit(const ivec2 pos) { visited.insert(pos); }

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
		grid<T> subgrid(const i32 x, const i32 y, const i32 w, const i32 h) const
		{
			grid<T> result{};

			if (w == 0 or h == 0)
				return result;

			if (x >= width() or y >= height())
				return result;

			i32 clamped_w = std::min(w, width() - x);
			i32 clamped_h = std::min(h, height() - y);

			result.resize(clamped_w, clamped_h);

			for (i32 yy = 0; yy < clamped_h; ++yy)
			{
				for (i32 xx = 0; xx < clamped_w; ++xx)
				{
					result.set(xx, yy, this->get(x + xx, y + yy));
				}
			}

			return result;
		}

		grid<T> subgrid(const ivec2 pos, const i32 w, const i32 h) const { return subgrid(pos.x, pos.y, w, h); }

		T operator[](const ivec2 pos) const { return data.get(pos.x, pos.y); }

		T& operator[](const ivec2 pos) { return data.get(pos.x, pos.y); }

		T operator[](const i32 x, const i32 y) const { return data.get(x, y); }

		T& operator[](const i32 x, const i32 y) { return data.get(x, y); }

		T& at(const i32 x, const i32 y) { return data.get(x, y); }

		T at(const i32 x, const i32 y) const { return data.get(x, y); }

		T at(const ivec2 pos) const { return data.get(pos.x, pos.y); }

		T& at(const ivec2 pos) { return data.get(pos.x, pos.y); }

		T get(const i32 x, const i32 y) const { return data.get(x, y); }

		//
		T& get(const i32 x, const i32 y) { return data.get(x, y); }

		T get(const ivec2 pos) const { return data.get(pos.x, pos.y); }

		T& get(const ivec2 pos) { return data.get(pos.x, pos.y); }

		std::optional<std::span<const T>> row(const i32 y) const
		{
			if (y >= height())
				return std::nullopt;

			return std::span<const T>{data.data() + y * width(), static_cast<std::size_t>(width())};
		}

		std::optional<std::span<T>> row(const i32 y)
		{
			if (y >= height())
				return std::nullopt;

			return std::span<T>{&data.get(0, y), static_cast<std::size_t>(width())};
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
						if (bool r = op(ivec2{x, y}, *v); r == false)
							return;
					}
				}
			}
		}

		void for_each_neighbour4(
		  const std::function<bool(const ivec2, const T, const std::array<std::optional<T>, 4>&)>& op) const
		{
			for (i32 y = 0; y < as<i32>(height()); ++y)
			{
				for (i32 x = 0; x < as<i32>(width()); ++x)
				{
					if (auto v = try_at(ivec2{x, y}); v.has_value())
					{
						if (bool r = op(ivec2{x, y}, *v, neighbours4(ivec2{x, y})); r == false)
							return;
					}
				}
			}
		}

		void for_each_neighbour8(
		  const std::function<bool(const ivec2, const T, const std::array<std::optional<T>, 8>&)>& op) const
		{
			for (i32 y = 0; y < as<i32>(height()); ++y)
			{
				for (i32 x = 0; x < as<i32>(width()); ++x)
				{
					if (auto v = try_at(ivec2{x, y}); v.has_value())
					{
						if (bool r = op(ivec2{x, y}, *v, neighbours8(ivec2{x, y})); r == false)
							return;
					}
				}
			}
		}

		auto points() const
		{
			const i32 grid_width = width();
			return std::views::iota(i32{0}, size())
				   | std::views::transform([grid_width](const i32 index)
										   { return ivec2{as<i32>(index % grid_width), as<i32>(index / grid_width)}; });
		}

		void for_each_row(const std::function<void(usize, std::span<const T>)>& op)
		{
			for (i32 y = 0; y < height(); ++y)
			{
				if (const auto row_values = row(y))
					op(as<usize>(y), *row_values);
			}
		}

		void for_each_column(const std::function<void(const i32, const std::vector<T>&)>& op) const
		{
			for (i32 x = 0; x < width(); ++x)
			{
				std::vector<T> column{};
				column.reserve(height());
				for (i32 y = 0; y < height(); ++y)
				{
					column.push_back(get(x, y));
				}
				op(x, column);
			}
		}

		void resize(i32 nwidth, i32 nheight) { data.resize(nwidth, nheight); }

		void set(ivec2 pos, const T& value) { data.set(as<i32>(pos.x), as<i32>(pos.y), value); }

		void set(i32 x, i32 y, const T& value) { data.set(x, y, value); }

		void clear() { data.clear(); }

		i32 count(std::string_view input) const { return data.count(input); }

		bool valid(const i32 x, const i32 y) const { return data.valid(x, y); }

		bool valid(ivec2 c) const { return data.valid(c.x, c.y); }

		auto neighbours4(ivec2 pos) const
		{
			return std::array<std::optional<T>, 4>{
			  try_at(pos + north), try_at(pos + east), try_at(pos + south), try_at(pos + west)};
		}

		auto neighbours8(ivec2 pos) const
		{
			return std::array<std::optional<T>, 8>{
			  try_at(pos + north),
			  try_at(pos + north_east),
			  try_at(pos + east),
			  try_at(pos + south_east),
			  try_at(pos + south),
			  try_at(pos + south_west),
			  try_at(pos + west),
			  try_at(pos + north_west)};
		}

		i32 width() const { return data.width(); };

		i32 height() const { return data.height(); };

		i32 size() const { return data.size(); }

		i32 size_in_bytes() const { return data.size_in_bytes(); }

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

				i32 e2 = 2 * err;

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

				i32 e2 = 2 * err;

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
		void hline(i32 x1, const i32 y, i32 x2, const T& v)
		{
			if (x1 > x2)
				std::swap(x1, x2);

			for (i32 x = x1; x <= x2; ++x)
				set(x, y, v);
		}

		// pos, x2
		void hline(const ivec2 pos, const i32 x2, const T& v) { hline(as<i32>(pos.x), as<i32>(pos.y), x2, v); }

		// x,y,y2
		void vline(i32 x, i32 y1, i32 y2, const T& v)
		{
			if (y1 > y2)
				std::swap(y1, y2);

			for (i32 y = y1; y <= y2; ++y)
				set(x, y, v);
		}

		// pos, y2
		void vline(const ivec2 pos, const i32 y2, const T& v) { vline(as<i32>(pos.x), as<i32>(pos.y), y2, v); }

		void rectangle(const ivec2 p1, const ivec2 p2, const T& v, filled f = filled::yes)
		{
			rectangle(as<i32>(p1.x), as<i32>(p1.y), as<i32>(p2.x), as<i32>(p2.y), v, f);
		}

		void rectangle(i32 x1, i32 y1, i32 x2, i32 y2, const T& v, filled f = filled::yes)
		{
			if (x1 > x2)
				std::swap(x1, x2);
			if (y1 > y2)
				std::swap(y1, y2);

			if (f == filled::yes)
			{
				for (i32 y = y1; y < y2 + 1; ++y)
					for (i32 x = x1; x < x2 + 1; ++x)
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

		void circle(i32 xm, i32 ym, i32 r, const T& v)

		{
			const i32 cx = as<i32>(xm);
			const i32 cy = as<i32>(ym);

			i32 x = -as<i32>(r), y = 0, err = 2 - 2 * as<i32>(r);
			do
			{
				// set() ignores negative (wrapped) coordinates
				set(static_cast<i32>(cx - x), static_cast<i32>(cy + y), v);
				set(static_cast<i32>(cx - y), static_cast<i32>(cy - x), v);
				set(static_cast<i32>(cx + x), static_cast<i32>(cy - y), v);
				set(static_cast<i32>(cx + y), static_cast<i32>(cy + x), v);
				const i32 e = err;
				if (e > x)
					err += ++x * 2 + 1;
				if (e <= y)
					err += ++y * 2 + 1;
			} while (x < 0);
		}

		void circle(const ivec2 center, const i32 r, const T& v) { circle(as<i32>(center.x), as<i32>(center.y), r, v); }

		void fill(const T& value) { data.fill(value); }



		template<floodfill_cb Func>
		i32 floodfill(ivec2 pos, const Func&& cb)
		{
			static_assert(std::is_same_v<T, std::invoke_result_t<Func, ivec2>>,
						  "Callback must have signature fn(ivec2) -> T (array2d<T>)");

			T value = data.get(pos.x, pos.y);

			std::set<ivec2>   seen;
			std::deque<ivec2> to_visit;

			to_visit.push_back(pos);

			i32 count{};

			while (not to_visit.empty())
			{
				const auto newpos = to_visit.front();
				to_visit.pop_front();

				if (not valid(newpos))
					continue;

				if (seen.contains(newpos))
					continue;

				seen.insert(newpos);

				if (get(newpos) != value)
					continue;

				set(newpos, cb(newpos));
				count += 1;

				to_visit.push_back(newpos + east);
				to_visit.push_back(newpos + south);
				to_visit.push_back(newpos + west);
				to_visit.push_back(newpos + north);
			}

			return count;
		}

		i32 floodfill(ivec2 pos, const T& r)
		{
			return floodfill(pos, [r](ivec2) -> T { return r; });
		}

		// find_all
		[[nodiscard]] auto find_all(const T to_find) const
		{
			std::vector<ivec2> points;

			for (auto y = 0; y < height(); ++y)
			{
				for (auto x = 0; x < width(); ++x)
				{
					if (data.get(x, y) == to_find)
						points.emplace_back(ivec2{as<i32>(x), as<i32>(y)});
				}
			}

			std::ranges::sort(points, grid_order);

			return points;
		}

		std::optional<ivec2> find(const T value) const { return data.find(value); }

		void reverse_row(i32 row)
		{
			if (row >= height())
				return;

			for (i32 x = 0; x < width() / 2; ++x)
				std::swap(data.at(x, row), data.at(width() - 1u - x, row));
		}

		void reverse_col(i32 col)
		{
			if (col >= width())
				return;

			for (i32 y = 0; y < height() / 2; ++y)
				std::swap(data.at(col, y), data.at(col, height() - 1u - y));
		}

		void transpose()
		{
			array2d<T> transposed;
			transposed.resize(height(), width());

			for (i32 y = 0; y < height(); ++y)
			{
				for (i32 x = 0; x < width(); ++x)
				{
					transposed.set(y, x, get(x, y));
				}
			}
			std::swap(data, transposed);
		}

		void reverse_rows()
		{
			for (i32 y = 0; y < height(); ++y)
				reverse_row(y);
		}

		void reverse_columns()
		{
			for (i32 x = 0; x < width(); ++x)
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
			for (i32 y = 0; y < height(); ++y)
			{
				for (i32 x = 0; x < width(); ++x)
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

			i32 colors = *std::max_element(data.begin(), data.end());

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
			i32   cost = std::numeric_limits<i32>::max(); // best known steps from start
			ivec2 parent{-1, -1};                         // where we came from
		};

														  // open-list entry, f = cost + heuristic
		struct node
		{
			ivec2 pos{};
			i32   cost{0}; // cost from start
			i32   f{0};    // f = cost + heuristic
		};

		const i32  width     = g.width();
		const auto heuristic = [goal](const ivec2 p) { return as<i32>(std::abs(p.x - goal.x) + std::abs(p.y - goal.y)); };
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
				const i32   next_cost = current.cost + 1;

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

		const i32 w = g.width();

		const auto to_index = [w](const ivec2 p) { return as<i32>(p.y) * w + as<i32>(p.x); };
		const auto to_point = [w](const i32 i) { return ivec2{as<i32>(i % w), as<i32>(i / w)}; };

		constexpr i32 none = std::numeric_limits<i32>::max();

		std::vector<i32>  parent(g.size(), none);
		std::vector<bool> visited(g.size(), false);
		std::queue<i32>   open;

		const i32 start_index = to_index(start);
		visited[start_index]  = true;
		open.push(start_index);

		constexpr std::array<ivec2, 4> directions{north, east, south, west};

		while (not open.empty())
		{
			const i32   current = open.front();
			const ivec2 pos     = to_point(current);
			open.pop();

			if (pos == goal)
			{
				std::vector<ivec2> path;
				for (i32 i = current; i != none; i = parent[i])
					path.push_back(to_point(i));

				std::ranges::reverse(path);
				return path;
			}

			for (const auto& direction : directions)
			{
				const ivec2 next = pos + direction;

				if (not g.valid(next) or not walkable(g.get(next)))
					continue;

				const i32 next_index = to_index(next);
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

	// DFS on a 4-connected grid. Returns a path from start to goal (both included), or nullopt if no path exists.
	export template<typename T, typename WalkableFn>
	requires Walkable<T, WalkableFn>
	auto dfs(const grid<T>& g, const ivec2 start, const ivec2 goal, WalkableFn&& walkable)
	  -> std::optional<std::vector<ivec2>>
	{
		if (not g.valid(start) or not g.valid(goal))
			return std::nullopt;

		if (not walkable(g.get(start)) or not walkable(g.get(goal)))
			return std::nullopt;

		const i32 w = g.width();

		const auto to_index = [w](const ivec2 p) { return as<i32>(p.y) * w + as<i32>(p.x); };
		const auto to_point = [w](const i32 i) { return ivec2{as<i32>(i % w), as<i32>(i / w)}; };

		constexpr i32 none = std::numeric_limits<i32>::max();

		std::vector<i32>  parent(g.size(), none);
		std::vector<bool> visited(g.size(), false);
		std::stack<i32>   open; // LIFO: the last pushed neighbour is explored first

		const i32 start_index = to_index(start);
		visited[start_index]  = true;
		open.push(start_index);

		constexpr std::array<ivec2, 4> directions{north, east, south, west};

		while (not open.empty())
		{
			const i32   current = open.top();
			const ivec2 pos     = to_point(current);
			open.pop();

			if (pos == goal)
			{
				std::vector<ivec2> path;
				for (i32 i = current; i != none; i = parent[i])
					path.push_back(to_point(i));

				std::ranges::reverse(path);
				return path;
			}

			for (const auto& direction : directions)
			{
				const ivec2 next = pos + direction;

				if (not g.valid(next) or not walkable(g.get(next)))
					continue;

				const i32 next_index = to_index(next);
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
	export auto dfs(const grid<u8>& g, const ivec2 start, const ivec2 goal) -> std::optional<std::vector<ivec2>>
	{
		return dfs(g, start, goal, [](const u8 v) { return v == 0; });
	}

	// ########################################################################
	// ########################################################################
	// ########################################################################
	// ########################################################################
	// ########################################################################
	// ########################################################################
	// Grid 3d

	export template<typename T = u8>
	class grid3d
	{
	private:
		struct cell_data
		{
			std::optional<T>                value;
			std::array<std::optional<T>, 6> faces{};
		};

		static constexpr std::array<ivec3, 6> face_directions{
		  grid3d_directions::top,
		  grid3d_directions::bottom,
		  grid3d_directions::left,
		  grid3d_directions::right,
		  grid3d_directions::front,
		  grid3d_directions::back};

		static auto face_index(const ivec3 dir) -> std::optional<usize>
		{
			for (usize i = 0; i < face_directions.size(); ++i)
			{
				if (face_directions[i] == dir)
					return i;
			}
			return std::nullopt;
		}

		i32 width{};
		i32 height{};
		i32 depth{};

		std::unordered_map<ivec3, cell_data> data;

	public:
		grid3d(i32 width, i32 height, i32 depth)
			: width(width)
			, height(height)
			, depth(depth)
		{
			data.reserve(width * height * depth);
		}

		bool valid_position(ivec3 pos) const
		{
			return (pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height && pos.z >= 0 && pos.z < depth);
		}

		void set_plane(const grid<T>& plane, const ivec3 dir)
		{
			const auto index = face_index(dir);
			assert::check(index.has_value(), "grid3d::set_plane: direction must be a signed unit axis");
			if (not index.has_value())
				return;

			const i32  expected_width  = dir.x != 0 ? depth : width;
			const i32  expected_height = dir.x != 0 ? height : dir.y != 0 ? depth : height;
			const bool dimensions_match
			  = as<i32>(plane.width()) == expected_width and as<i32>(plane.height()) == expected_height;
			assert::check(dimensions_match, "grid3d::set_plane: plane dimensions do not match face");
			if (not dimensions_match)
				return;

			for (i32 y = 0; y < expected_height; ++y)
			{
				for (i32 x = 0; x < expected_width; ++x)
				{
					ivec3 pos;
					if (dir.x != 0)
						pos = {dir.x < 0 ? 0 : as<i32>(width - 1), as<i32>(y), as<i32>(x)};
					else if (dir.y != 0)
						pos = {as<i32>(x), dir.y < 0 ? 0 : as<i32>(height - 1), as<i32>(y)};
					else
						pos = {as<i32>(x), as<i32>(y), dir.z < 0 ? 0 : as<i32>(depth - 1)};

					data[pos].faces[*index] = plane.get(as<i32>(x), as<i32>(y));
				}
			}
		}

		grid<T> getplane(const ivec3 dir) const
		{
			const auto index = face_index(dir);
			assert::check(index.has_value(), "grid3d::getplane: direction must be a signed unit axis");
			if (not index.has_value())
				return grid<T>{0, 0};

			const i32 plane_width  = dir.x != 0 ? depth : width;
			const i32 plane_height = dir.y != 0 ? depth : height;
			grid<T>   plane{as<i32>(plane_width), as<i32>(plane_height)};

			const i32 fixed_x = dir.x < 0 ? 0 : as<i32>(width - 1);
			const i32 fixed_y = dir.y < 0 ? 0 : as<i32>(height - 1);
			const i32 fixed_z = dir.z < 0 ? 0 : as<i32>(depth - 1);
			for (i32 y = 0; y < plane_height; ++y)
			{
				for (i32 x = 0; x < plane_width; ++x)
				{
					ivec3 pos;
					if (dir.x != 0)
						pos = {fixed_x, as<i32>(y), as<i32>(x)}; // (z,y)
					else if (dir.y != 0)
						pos = {as<i32>(x), fixed_y, as<i32>(y)}; // (x,z)
					else
						pos = {as<i32>(x), as<i32>(y), fixed_z}; // (x,y)

					plane.set(as<i32>(x), as<i32>(y), get_face(pos, dir).value_or(T{}));
				}
			}

			return plane;
		}

		std::string print() const
		{
			const auto grid_top    = getplane(grid3d_directions::top);
			const auto grid_bottom = getplane(grid3d_directions::bottom);
			const auto grid_left   = getplane(grid3d_directions::left);
			const auto grid_right  = getplane(grid3d_directions::right);
			const auto grid_front  = getplane(grid3d_directions::front);
			const auto grid_back   = getplane(grid3d_directions::back);

			std::string       result;
			const std::string indent(grid_left.width() + 1, ' ');
			const auto        append_row = [&result](const grid<T>& face, const i32 y)
			{
				for (i32 x = 0; x < face.width(); ++x)
				{
					const char value = static_cast<char>(face.get(x, y));
					result.push_back(value == '\0' ? '.' : value);
				}
			};

			for (i32 y = 0; y < grid_top.height(); ++y)
			{
				result += indent;
				append_row(grid_top, y);
				result.push_back('\n');
			}

			for (i32 y = 0; y < grid_front.height(); ++y)
			{
				append_row(grid_left, y);
				result.push_back(' ');
				append_row(grid_front, y);
				result.push_back(' ');
				append_row(grid_right, y);
				result.push_back(' ');
				append_row(grid_back, y);
				result.push_back('\n');
			}

			for (i32 y = 0; y < grid_bottom.height(); ++y)
			{
				result += indent;
				append_row(grid_bottom, y);
				result.push_back('\n');
			}

			return result;
		}

		void dump() const { dbg::print("{}", print()); }

		// Face order: front (z=0), back (z=depth-1), left (x=0),
		// right (x=width-1), top (y=0), bottom (y=height-1).
		// Face-local coordinates are (x,y) for front/back, (z,y) for
		// left/right, and (x,z) for top/bottom. Shared edge and corner
		// cubies retain a separate sticker value for each face.
		void load_from_grids(const std::array<grid<T>, 6>& grids)
		{
			const bool dimensions_match
			  = as<i32>(grids[0].width()) == width and as<i32>(grids[0].height()) == height
				and as<i32>(grids[1].width()) == width and as<i32>(grids[1].height()) == height
				and as<i32>(grids[2].width()) == depth and as<i32>(grids[2].height()) == height
				and as<i32>(grids[3].width()) == depth and as<i32>(grids[3].height()) == height
				and as<i32>(grids[4].width()) == width and as<i32>(grids[4].height()) == depth
				and as<i32>(grids[5].width()) == width and as<i32>(grids[5].height()) == depth;
			assert::check(dimensions_match, "grid3d::load_from_grids: face dimensions do not match grid dimensions");
			if (not dimensions_match)
				return;
			data.clear();
			set_plane(grids[0], grid3d_directions::front);
			set_plane(grids[1], grid3d_directions::back);
			set_plane(grids[2], grid3d_directions::left);
			set_plane(grids[3], grid3d_directions::right);
			set_plane(grids[4], grid3d_directions::top);
			set_plane(grids[5], grid3d_directions::bottom);
		}

		void set(ivec3 pos, const T& value)
		{
			if (not valid_position(pos))
			{
				dbg::println("grid3d::set: invalid position {}", pos);
				return;
			}

			data[pos].value = value;
		}

		std::optional<T> get(ivec3 pos) const
		{
			if (not valid_position(pos))
			{
				dbg::println("grid3d::get: invalid position {}", pos);
				return std::nullopt;
			}

			const auto it = data.find(pos);
			return it == data.end() ? T{} : it->second.value.value_or(T{});
		}

		void set_face(const ivec3 pos, const ivec3 dir, const T& value)
		{
			if (not valid_position(pos))
			{
				dbg::println("grid3d::set_face: invalid position {}", pos);
				return;
			}

			const auto index = face_index(dir);
			assert::check(index.has_value(), "grid3d::set_face: direction must be a signed unit axis");
			if (not index.has_value())
				return;
			bool on_boundary = false;
			switch (*index)
			{
				case 0: on_boundary = pos.y == 0; break;          // top
				case 1: on_boundary = pos.y == height - 1; break; // bottom
				case 2: on_boundary = pos.x == 0; break;          // left
				case 3: on_boundary = pos.x == width - 1; break;  // right
				case 4: on_boundary = pos.z == 0; break;          // front
				case 5: on_boundary = pos.z == depth - 1; break;  // back
			}
			assert::check(on_boundary, "grid3d::set_face: position is not on the requested face");
			if (not on_boundary)
				return;

			data[pos].faces[*index] = value;
		}

		std::optional<T> get_face(const ivec3 pos, const ivec3 dir) const
		{
			if (not valid_position(pos))
			{
				dbg::println("grid3d::get_face: invalid position {}", pos);
				return std::nullopt;
			}

			const auto index = face_index(dir);
			assert::check(index.has_value(), "grid3d::get_face: direction must be a signed unit axis");
			if (not index.has_value())
				return std::nullopt;

			const auto it = data.find(pos);
			if (it == data.end())
				return std::nullopt;
			return it->second.faces[*index];
		}

		void rotate(const ivec3 axis, const i32 angle)
		{
			const bool valid_axis = ((axis.x == 1 or axis.x == -1) and axis.y == 0 and axis.z == 0)
									or ((axis.y == 1 or axis.y == -1) and axis.x == 0 and axis.z == 0)
									or ((axis.z == 1 or axis.z == -1) and axis.x == 0 and axis.y == 0);
			assert::check(valid_axis, "grid3d::rotate: axis must be a signed unit axis");
			if (not valid_axis)
				return;

			const bool valid_angle = angle % 90 == 0;
			assert::check(valid_angle, "grid3d::rotate: angle must be a multiple of 90");
			if (not valid_angle)
				return;

			// Positive Y turns follow the cube-net convention: the front face moves right.
			// A negative axis reverses the turn direction.
			i32 turns = (angle / 90) % 4;
			if ((axis.x < 0) or (axis.y < 0) or (axis.z < 0))
				turns = -turns;
			if (turns < 0)
				turns += 4;

			const i32 old_width  = width;
			const i32 old_height = height;
			const i32 old_depth  = depth;
			auto      rotated    = std::unordered_map<ivec3, cell_data>{};
			rotated.reserve(data.size());
			const auto rotate_direction = [axis, turns](const ivec3 dir)
			{
				if (axis.x != 0)
				{
					switch (turns)
					{
						case 1: return ivec3{dir.x, -dir.z, dir.y};
						case 2: return ivec3{dir.x, -dir.y, -dir.z};
						case 3: return ivec3{dir.x, dir.z, -dir.y};
						default: return dir;
					}
				}
				if (axis.y != 0)
				{
					switch (turns)
					{
						case 1: return ivec3{-dir.z, dir.y, dir.x};
						case 2: return ivec3{-dir.x, dir.y, -dir.z};
						case 3: return ivec3{dir.z, dir.y, -dir.x};
						default: return dir;
					}
				}
				switch (turns)
				{
					case 1: return ivec3{-dir.y, dir.x, dir.z};
					case 2: return ivec3{-dir.x, -dir.y, dir.z};
					case 3: return ivec3{dir.y, -dir.x, dir.z};
					default: return dir;
				}
			};

			for (const auto& [pos, value] : data)
			{
				ivec3 next = pos;
				if (axis.x != 0)
				{
					switch (turns)
					{
						case 1: next = {pos.x, as<i32>(old_depth - 1 - pos.z), pos.y}; break;
						case 2: next = {pos.x, as<i32>(old_height - 1 - pos.y), as<i32>(old_depth - 1 - pos.z)}; break;
						case 3: next = {pos.x, pos.z, as<i32>(old_height - 1 - pos.y)}; break;
						default: break;
					}
				}
				else if (axis.y != 0)
				{
					switch (turns)
					{
						case 1: next = {as<i32>(old_depth - 1 - pos.z), pos.y, pos.x}; break;
						case 2: next = {as<i32>(old_width - 1 - pos.x), pos.y, as<i32>(old_depth - 1 - pos.z)}; break;
						case 3: next = {pos.z, pos.y, as<i32>(old_width - 1 - pos.x)}; break;
						default: break;
					}
				}
				else
				{
					switch (turns)
					{
						case 1: next = {as<i32>(old_height - 1 - pos.y), pos.x, pos.z}; break;
						case 2: next = {as<i32>(old_width - 1 - pos.x), as<i32>(old_height - 1 - pos.y), pos.z}; break;
						case 3: next = {pos.y, as<i32>(old_width - 1 - pos.x), pos.z}; break;
						default: break;
					}
				}

				cell_data next_value;
				next_value.value = value.value;
				for (usize i = 0; i < value.faces.size(); ++i)
				{
					if (not value.faces[i].has_value())
						continue;

					const auto next_index = face_index(rotate_direction(face_directions[i]));
					assert::check(next_index.has_value(), "grid3d::rotate: failed to rotate face direction");
					if (next_index.has_value())
						next_value.faces[*next_index] = value.faces[i];
				}

				rotated.emplace(next, std::move(next_value));
			}

			if (turns % 2 != 0)
			{
				if (axis.x != 0)
					std::swap(height, depth);
				else if (axis.y != 0)
					std::swap(width, depth);
				else
					std::swap(width, height);
			}

			data = std::move(rotated);
		}
	};


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
