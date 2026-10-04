
#include <windows.h>
#include <Commctrl.h>
#include <cmath>
#include <gameinput.h>
#include <intrin.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstddef>


import std;
import deckard;
import deckard.app2;
import deckard.types;
import deckard.helpers;
import deckard.math;
import deckard.timers;
import deckard.vec;

using namespace deckard;
using namespace deckard::literals;

// using namespace std::string_literals;
namespace fs = std::filesystem;
using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace std::chrono_literals;

using namespace deckard::vec;

void destroy() noexcept { dbg::println("destroy() called"); }

/*
class Entity {
public:
	void update_physics(float fixed_dt) noexcept {
		// 1. Save state before modifying
		prev_position_ = curr_position_;

		// 2. Advance state for the new tick
		curr_position_.x += velocity_.x * fixed_dt;
		curr_position_.y += velocity_.y * fixed_dt;
	}

	// Called during the variable rendering phase
	[[nodiscard]] Vec2 get_render_position(float alpha) const noexcept {
		return prev_position_.lerp(curr_position_, alpha);
	}

	void set_velocity(Vec2 v) noexcept { velocity_ = v; }

private:
	Vec2 prev_position_{};
	Vec2 curr_position_{};
	Vec2 velocity_{50.0f, 0.0f}; // 50 units/sec
};
*/

struct grid_cell
{
	u32            x{};
	u32            y{};
	constexpr bool operator==(const grid_cell&) const noexcept = default;
};

struct grid_layout
{
	f32         origin_x{50.0f};
	f32         origin_y{50.0f};
	f32         margin{5.0f};
	u16         block_size{50};
	extent<u16> grid{8, 8};

	std::vector<u8> blocked{};

	[[nodiscard]] constexpr usize index(grid_cell c) const noexcept { return static_cast<usize>(c.y) * grid.width + c.x; }

	[[nodiscard]] bool is_blocked(grid_cell c) const noexcept { return not blocked.empty() and blocked[index(c)] != 0; }

	void generate_obstacles(usize count, std::span<const grid_cell> reserved, std::mt19937& rng)
	{
		blocked.assign(static_cast<usize>(grid.width) * grid.height, 0);

		std::vector<grid_cell> candidates;
		for (u32 y = 0; y < grid.height; ++y)
			for (u32 x = 0; x < grid.width; ++x)
				if (const grid_cell c{x, y}; std::ranges::find(reserved, c) == reserved.end())
					candidates.push_back(c);

		std::ranges::shuffle(candidates, rng);

		for (const grid_cell c : candidates | std::views::take(count))
			blocked[index(c)] = 1;
	}

	[[nodiscard]] constexpr f32 stride() const noexcept { return static_cast<f32>(block_size) + margin; }

	[[nodiscard]] constexpr vec2 cell_pos(grid_cell c) const noexcept
	{
		return {origin_x + static_cast<f32>(c.x) * stride(), origin_y + static_cast<f32>(c.y) * stride()};
	}

	[[nodiscard]] constexpr std::optional<grid_cell> nearest_cell(f32 px, f32 py) const noexcept
	{
		const f32 half = margin * 0.5f;
		const f32 lx   = px - origin_x + half;
		const f32 ly   = py - origin_y + half;

		if (lx < 0.0f or ly < 0.0f)
			return std::nullopt;

		const f32 s  = stride();
		const u32 cx = static_cast<u32>(lx / s);
		const u32 cy = static_cast<u32>(ly / s);

		if (cx >= grid.width or cy >= grid.height)
			return std::nullopt;

		return grid_cell{cx, cy};
	}
};

enum class direction : u8
{
	north,
	east,
	south,
	west
};

constexpr u32 min_route_len = 3; // manhattan distance, in cells
constexpr u32 max_route_len = 6;

struct entity
{
	enum class phase : u8
	{
		pausing,
		moving
	};

	vec2 position{0.0f, 0.0f};           // simulation position, no longer used for rendering
	vec2 size{50.0f, 50.0f};

	grid_cell              cell{};       // current cell
	grid_cell              start{};      // where the current/last route began
	std::vector<grid_cell> route{};      // kept after arrival so rendering can still sample it
	f32                    s{0.0f};      // eased distance along route, in tiles [0, route.size()]
	f32                    prev_s{0.0f}; // s at the previous logic tick
	direction              dir{direction::north};
	phase                  state{phase::pausing};
	f32                    timer{0.0f};
	f32                    pause_time{0.5f};
	f32                    speed{1.0f};
	rgb                    color{0, 255, 255};
};

constexpr f32 move_duration = 1.0f;
constexpr f32 min_turn_time = 0.5f;
constexpr f32 max_turn_time = 2.0f;

// Turn to a random direction (90 degree steps) and pick a random pause length
inline void begin_pause(entity& e, std::mt19937& rng) noexcept
{
	e.dir        = static_cast<direction>(std::uniform_int_distribution<int>{0, 3}(rng));
	e.pause_time = std::uniform_real_distribution<f32>{min_turn_time, max_turn_time}(rng);
	e.state      = entity::phase::pausing;
}

[[nodiscard]] std::vector<grid_cell> make_route(grid_cell from, const grid_layout& layout, std::mt19937& rng)
{
	constexpr u32 unvisited = std::numeric_limits<u32>::max();

	const usize w = layout.grid.width;
	const usize h = layout.grid.height;

	std::vector<u32>      dist(w * h, unvisited);
	std::vector<usize>    parent(w * h, 0);
	std::queue<grid_cell> frontier;

	dist[layout.index(from)] = 0;
	frontier.push(from);

	struct offset
	{
		i32 dx;
		i32 dy;
	};

	constexpr std::array<offset, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

	std::vector<grid_cell> reachable; // everything found within max_route_len steps

	while (not frontier.empty())
	{
		const grid_cell cur = frontier.front();
		frontier.pop();

		const u32 d = dist[layout.index(cur)];
		if (d >= max_route_len)
			continue;

		for (const auto [dx, dy] : dirs)
		{
			const i32 nx = static_cast<i32>(cur.x) + dx;
			const i32 ny = static_cast<i32>(cur.y) + dy;

			if (nx < 0 or ny < 0 or nx >= layout.grid.width or ny >= layout.grid.height)
				continue;

			const grid_cell n{static_cast<u32>(nx), static_cast<u32>(ny)};

			if (layout.is_blocked(n) or dist[layout.index(n)] != unvisited)
				continue;

			dist[layout.index(n)]   = d + 1;
			parent[layout.index(n)] = layout.index(cur);
			reachable.push_back(n);
			frontier.push(n);
		}
	}

	// prefer destinations at least min_route_len away, otherwise take anything reachable
	std::vector<grid_cell> farthest;
	std::ranges::copy_if(
	  reachable, std::back_inserter(farthest), [&](grid_cell c) { return dist[layout.index(c)] >= min_route_len; });

	const auto& pool = farthest.empty() ? reachable : farthest;
	if (pool.empty())
		return {}; // completely boxed in

	const grid_cell dest = pool[std::uniform_int_distribution<usize>{0, pool.size() - 1}(rng)];

	// walk the parent chain back to the start, then reverse
	std::vector<grid_cell> route;
	for (usize i = layout.index(dest); i != layout.index(from); i = parent[i])
		route.push_back({static_cast<u32>(i % w), static_cast<u32>(i / w)});

	std::ranges::reverse(route);
	return route;
}

[[nodiscard]] constexpr direction direction_between(grid_cell a, grid_cell b) noexcept
{
	if (b.x > a.x)
		return direction::east;
	if (b.x < a.x)
		return direction::west;
	if (b.y > a.y)
		return direction::south;
	return direction::north;
}

constexpr f32 tile_hue       = 60.0f; // yellow, matches the tile colour {255, 255, 0}
constexpr f32 tile_hue_guard = 30.0f; // excluded +/- degrees around it

// random hue in [0, 360) that stays clear of the tile hue
[[nodiscard]] f32 random_entity_hue(std::mt19937& rng)
{
	constexpr f32 lo    = tile_hue - tile_hue_guard;
	constexpr f32 width = tile_hue_guard * 2.0f;

	// sample from the allowed length, then skip over the excluded band
	f32 h = std::uniform_real_distribution<f32>{0.0f, 360.0f - width}(rng);
	if (h >= lo)
		h += width;

	return h;
}

constexpr usize entity_count = 4;

// one hue per entity, all clear of the tile hue and at least ~0.5 * (allowed span / N) apart
template<usize N>
[[nodiscard]] std::array<f32, N> distinct_hues(std::mt19937& rng)
{
	constexpr f32 width  = tile_hue_guard * 2.0f;
	constexpr f32 lo     = tile_hue - tile_hue_guard;
	constexpr f32 span   = 360.0f - width; // hues left after removing the guard band
	constexpr f32 slice  = span / static_cast<f32>(N);
	constexpr f32 jitter = slice * 0.25f;  // keeps neighbours >= slice / 2 apart

	// random rotation so the palette differs between runs
	const f32 rotation = std::uniform_real_distribution<f32>{0.0f, span}(rng);

	std::array<f32, N> hues{};
	for (usize i = 0; i < N; ++i)
	{
		const f32 centre = (static_cast<f32>(i) + 0.5f) * slice + rotation;
		const f32 j      = std::uniform_real_distribution<f32>{-jitter, jitter}(rng);

		// work in the compressed space (guard band removed), wrapping around
		f32 h = std::fmod(centre + j + span, span);

		// then skip over the excluded band
		if (h >= lo)
			h += width;

		hues[i] = h;
	}

	std::ranges::shuffle(hues, rng); // so speed doesn't correlate with hue order
	return hues;
}

[[nodiscard]] inline entity make_entity(const grid_layout& layout, grid_cell cell, f32 speed, f32 hue, std::mt19937& rng)
{
	const vec2 p = layout.cell_pos(cell);
	entity     e{.position = p, .cell = cell, .start = cell};
	e.speed = speed;
	e.color = to_rgb(hue, 1.0f, 1.0f);
	begin_pause(e, rng);
	return e;
}

struct route_sample
{
	vec2      pos;
	direction dir;
};

// position on the route after travelling `s` tiles; always axis-aligned within a leg
[[nodiscard]] route_sample sample_route(const entity& e, f32 s, const grid_layout& layout) noexcept
{
	if (e.route.empty())
		return {layout.cell_pos(e.cell), e.dir};

	const usize i    = std::min(static_cast<usize>(std::max(s, 0.0f)), e.route.size() - 1);
	const f32   frac = std::clamp(s - static_cast<f32>(i), 0.0f, 1.0f);

	const grid_cell a = (i == 0) ? e.start : e.route[i - 1];
	const grid_cell b = e.route[i];

	const vec2 from = layout.cell_pos(a);
	const vec2 to   = layout.cell_pos(b);

	return {{math::lerp(from.x, to.x, frac), math::lerp(from.y, to.y, frac)}, direction_between(a, b)};
}

void update_entity(entity& e, f32 delta, const grid_layout& layout, std::mt19937& rng)
{
	e.timer += delta;
	e.prev_s = e.s;

	if (e.state == entity::phase::pausing and e.timer >= e.pause_time)
	{
		e.timer -= e.pause_time;
		e.route = make_route(e.cell, layout, rng);

		if (e.route.empty())
		{
			begin_pause(e, rng);
		}
		else
		{
			e.start  = e.cell;
			e.s      = 0.0f;
			e.prev_s = 0.0f; // don't interpolate from the previous route's progress
			e.state  = entity::phase::moving;
		}
	}

	if (e.state == entity::phase::moving)
	{
		const f32 n         = static_cast<f32>(e.route.size());
		const f32 move_time = n / e.speed;
		const f32 t         = std::min(e.timer / move_time, 1.0f);

		e.s   = math::smootherstep(t) * n;
		e.dir = sample_route(e, e.s, layout).dir;

		if (e.timer >= move_time)
		{
			e.timer -= move_time;
			e.cell = e.route.back();
			e.s    = n; // route stays in place, rendering keeps sampling its end
			begin_pause(e, rng);
		}
	}

	e.position = sample_route(e, e.s, layout).pos;
}

void render_grid(deckard::app2::renderer2* r, const grid_layout& l, f32 mx, f32 my) noexcept
{
	const f32  stride  = l.stride();
	const f32  bs      = static_cast<f32>(l.block_size);
	const auto hovered = l.nearest_cell(mx, my);

	const f32 total_w = static_cast<f32>(l.grid.width) * stride + l.margin;
	const f32 total_h = static_cast<f32>(l.grid.height) * stride + l.margin;

	r->draw_sprite(l.origin_x - l.margin, l.origin_y - l.margin, {total_w, total_h}, {0, 0, 0});

	for (u32 y = 0; y < l.grid.height; ++y)
	{
		for (u32 x = 0; x < l.grid.width; ++x)
		{
			const grid_cell c{x, y};
			const auto [px, py] = l.cell_pos(c);
			const bool blocked  = l.is_blocked(c);

			if (hovered == c)
			{
				constexpr f32 pad       = 4.0f;
				const auto    highlight = blocked ? std::array<u8, 3>{255, 64, 64} : std::array<u8, 3>{255, 0, 255};
				r->draw_sprite(px - pad, py - pad, {bs + pad * 2.0f, bs + pad * 2.0f}, highlight);
			}

			r->draw_sprite(px, py, {bs, bs}, blocked ? std::array<u8, 3>{60, 60, 60} : std::array<u8, 3>{255, 255, 0});
		}
	}
}

void render_entity(deckard::app2::renderer2* render, const entity& e, const grid_layout& layout, f32 alpha) noexcept
{
	const vec2 pos = sample_route(e, std::lerp(e.prev_s, e.s, alpha), layout).pos;

	// 1px border: a larger sprite behind the body
	constexpr f32               border = 1.0f;
	constexpr std::array<u8, 3> border_color{0, 0, 0};

	render->draw_sprite(pos.x - border, pos.y - border, {e.size.x + border * 2.0f, e.size.y + border * 2.0f}, border_color);

	render->draw_sprite(pos.x, pos.y, {e.size.x, e.size.y}, {e.color.r, e.color.g, e.color.b});

	// direction marker unchanged
	constexpr f32 m  = 12.0f;
	f32           mx = pos.x + (e.size.x - m) * 0.5f;
	f32           my = pos.y + (e.size.y - m) * 0.5f;

	switch (e.dir)
	{
		case direction::north: my = pos.y; break;
		case direction::south: my = pos.y + e.size.y - m; break;
		case direction::west: mx = pos.x; break;
		case direction::east: mx = pos.x + e.size.x - m; break;
	}

	render->draw_sprite(mx, my, {m, m}, {255, 255, 255});
}

void render_entities(
  deckard::app2::renderer2* render, std::span<const entity> entities, const grid_layout& layout, f32 alpha) noexcept
{
	for (const auto& e : entities)
		render_entity(render, e, layout, alpha);
}

struct world
{
	grid_layout           layout{};
	std::mt19937          rng{std::random_device{}()};
	std::array<entity, 4> entities{};
	f32                   alpha{0.0f};
};

[[nodiscard]] world make_world()
{
	constexpr std::array starts{grid_cell{0, 0}, grid_cell{4, 4}, grid_cell{7, 2}, grid_cell{2, 6}};
	static_assert(starts.size() == entity_count);

	world w;
	w.layout.generate_obstacles(12, starts, w.rng);

	const auto hues = distinct_hues<entity_count>(w.rng);

	w.entities = {
	  make_entity(w.layout, starts[0], 0.5f, hues[0], w.rng),
	  make_entity(w.layout, starts[1], 1.0f, hues[1], w.rng),
	  make_entity(w.layout, starts[2], 3.0f, hues[2], w.rng),
	  make_entity(w.layout, starts[3], 5.0f, hues[3], w.rng),
	};
	return w;
}

world g_world = make_world();

// simulation only, no renderer, no input polling
void update(world& w, f32 delta) noexcept
{
	for (auto& e : w.entities)
	{
		update_entity(e, delta, w.layout, w.rng);
	}
}

void render_world(deckard::app2::renderer2* r, const world& w, f32 mx, f32 my) noexcept
{
	render_grid(r, w.layout, mx, my);
	render_entities(r, w.entities, w.layout, w.alpha);
}

grid_layout  layout{};
std::mt19937 rng{std::random_device{}()};

void render(deckard::app2::renderer2* render, [[maybe_unused]] f32 delta) noexcept
{
	POINT p{};
	GetCursorPos(&p);
	ScreenToClient(render->get_handle(), &p);

	const auto size = render->get_size();
	const f32  mx   = static_cast<f32>(p.x);
	const f32  my   = static_cast<f32>(p.y);


	// advance(g_world, delta);
	//  background

	render->draw_text(mx + 20, my, std::format("Mouse: ({:.2f}, {:.2f})", mx, my), {255, 255, 255}, 40);

	// lines
	render->draw_line(0.0f, 0.0f, mx, my, {255, 0, 0}, 5.0f);
	render->draw_line(size.width, 0.0f, mx, my, {0, 255, 0}, 5.0f);
	render->draw_line(0.0f, size.height, mx, my, {0, 0, 255}, 5.0f);
	render->draw_line(size.width, size.height, mx, my, {255, 0, 255}, 5.0f);


	render->draw_sprite(mx + 50.0f, my + 50.0f, {100, 100}, {0, 255, 0});

	// for (auto& e : entities)
	//	update_entity(e, delta, layout, rng);


	// render_grid(render, delta, mx, my, {8, 8}, 5.0f, 50);
	//	 render_grid(, layout, mx, my);
	render_world(render, g_world, mx, my);
	// render_entities(render, g_world.entities, g_world.alpha);
}

template<typename T>
class sparseset
{
private:
	std::vector<T>   dense;
	std::vector<u32> dense_keys;
	std::vector<u32> sparse;

public:
};

bool initialize() noexcept
{
	dbg::println("initialize() called");
	return true;
}

i32 deckard_main([[maybe_unused]] utf8::view commandline)
{
#ifndef _DEBUG
	std::print("dbc {} ({}), ", window::build::version_string, window::build::calver);
	std::println("deckard {} ({})", deckard_build::build::version_string, deckard_build::build::calver);
	std::print("{}\n{}\n", big_text("DECKARD"), big_text(deckard_build::build::version_string));
#endif
	// ########################################################################



	// #########################################################################

	app::on_initialize(initialize);
	app::on_destroy(destroy);

	app::on_render(render);

	app::options options{
	  .size          = {1280, 720},
	  .fullscreen    = false,
	  .resizable     = true,
	  .vsync         = true,
	  .allow_win_key = true,
	  .monitor       = 0,
	  .title         = "Deckard - Vulkan - Window  1",
	};
	options.load_and_save();


	if (auto result = app::open(options); not result)
	{
		dbg::println("Failed to open app: {}", result.error());
		return -1;
	}




	enum class TestCounter : u32
	{
		ui,
		logic,
		count
	};

	constexpr std::array intervals{
	  1.0f / 30.0f, // ui
	  1.0f / 5.0f,  // logic
	};


	fixed_timers<TestCounter, intervals> timer{};

	app::on_tick(
	  [&](const f32 delta) noexcept
	  {
		timer.update(delta);

		//
		while (timer.tick(TestCounter::logic))
		{
			update(g_world, timer.interval(TestCounter::logic));
		}
		g_world.alpha = timer.alpha(TestCounter::logic);


		//
		while (timer.tick(TestCounter::ui))
		{
			app::title(std::format(
			  "Delta: {:<3.5f} - FPS: {:<8.2f} - Client: {} - Tick: {:<8.2f}",
			  app::delta_time(),
			  app::fps(),
			  app::size(),

			  timer.alpha(TestCounter::ui)));
		}
	  });


	while (app::running())
	{
		  if (app::was_key_pressed(VK_F11))
			  app::fullscreen(not app::fullscreen());

		  if (app::was_key_pressed(VK_ESCAPE))
			  app::close();
	}

	return 0;
}
