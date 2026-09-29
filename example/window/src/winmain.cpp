
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

	[[nodiscard]] constexpr f32 stride() const noexcept { return static_cast<f32>(block_size) + margin; }

	[[nodiscard]] constexpr vec2 cell_pos(grid_cell c) const noexcept
	{
		return {origin_x + static_cast<f32>(c.x) * stride(), origin_y + static_cast<f32>(c.y) * stride()};
	}

	[[nodiscard]] constexpr std::optional<grid_cell> cell_at(f32 px, f32 py) const noexcept
	{
		const f32 lx = px - origin_x;
		const f32 ly = py - origin_y;

		if (lx < 0.0f or ly < 0.0f)
			return std::nullopt;

		const f32 s  = stride();
		const u32 cx = static_cast<u32>(lx / s);
		const u32 cy = static_cast<u32>(ly / s);

		if (cx >= grid.width or cy >= grid.height)
			return std::nullopt;

		const f32 bs = static_cast<f32>(block_size);
		if (lx - static_cast<f32>(cx) * s >= bs or ly - static_cast<f32>(cy) * s >= bs)
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

constexpr f32 min_speed = 1.5f; // tiles per second
constexpr f32 max_speed = 5.0f;

struct entity
{
	enum class phase : u8
	{
		pausing,
		moving
	};

	vec2 position{0.0f, 0.0f};
	vec2 prev_position{0.0f, 0.0f};
	vec2 size{50.0f, 50.0f};

	grid_cell cell{};
	grid_cell target{};
	direction dir{direction::north};
	phase     state{phase::pausing};
	f32       timer{0.0f};
	f32       pause_time{0.5f};
	f32       speed{1.0f}; // tiles per second, set once at creation
	rgb       color{0, 255, 255};
};

constexpr f32 move_duration = 1.0f;
constexpr f32 min_turn_time = 0.5f;
constexpr f32 max_turn_time = 2.0f;

[[nodiscard]] constexpr std::optional<grid_cell> step_cell(grid_cell from, direction dir, const grid_layout& layout) noexcept
{
	i32 dx = 0;
	i32 dy = 0;

	switch (dir)
	{
		case direction::north: dy = -1; break;
		case direction::east: dx = 1; break;
		case direction::south: dy = 1; break;
		case direction::west: dx = -1; break;
	}

	const i32 nx = static_cast<i32>(from.x) + dx;
	const i32 ny = static_cast<i32>(from.y) + dy;

	if (nx < 0 || ny < 0 || nx >= layout.grid.width || ny >= layout.grid.height)
		return std::nullopt;

	return grid_cell{static_cast<u32>(nx), static_cast<u32>(ny)};
}

// Turn to a random direction (90 degree steps) and pick a random pause length
inline void begin_pause(entity& e, std::mt19937& rng) noexcept
{
	e.dir        = static_cast<direction>(std::uniform_int_distribution<int>{0, 3}(rng));
	e.pause_time = std::uniform_real_distribution<f32>{min_turn_time, max_turn_time}(rng);
	e.state      = entity::phase::pausing;
}

[[nodiscard]] inline entity make_entity(const grid_layout& layout, grid_cell cell, std::mt19937& rng) noexcept
{
	const vec2 p = layout.cell_pos(cell);
	entity     e{.position = p, .prev_position = p, .cell = cell, .target = cell};
	e.speed = std::uniform_real_distribution<f32>{min_speed, max_speed}(rng);
	e.color = to_rgb(std::uniform_real_distribution<f32>{0.0f, 360.0f}(rng), 1.0f, 1.0f);
	begin_pause(e, rng);
	return e;
}

// Pick a random in-bounds neighbour (4-directional)
[[nodiscard]] grid_cell random_neighbour(grid_cell from, const grid_layout& layout, std::mt19937& rng) noexcept
{
	struct offset
	{
		i32 dx;
		i32 dy;
	};

	constexpr std::array<offset, 4> dirs{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};

	std::array<grid_cell, 4> valid{};
	usize                    count = 0;

	for (const auto [dx, dy] : dirs)
	{
		const i32 nx = static_cast<i32>(from.x) + dx;
		const i32 ny = static_cast<i32>(from.y) + dy;

		if (nx < 0 || ny < 0 || nx >= layout.grid.width || ny >= layout.grid.height)
			continue;

		valid[count++] = {static_cast<u32>(nx), static_cast<u32>(ny)};
	}

	if (count == 0)
		return from;

	return valid[std::uniform_int_distribution<usize>{0, count - 1}(rng)];
}

void update_entity(entity& e, f32 delta, const grid_layout& layout, std::mt19937& rng) noexcept
{
	e.timer += delta;
	e.prev_position = e.position;

	if (e.state == entity::phase::pausing and e.timer >= e.pause_time)
	{
		e.timer -= e.pause_time;

		if (const auto next = step_cell(e.cell, e.dir, layout))
		{
			e.target = *next;
			e.state  = entity::phase::moving;
		}
		else
		{
			// facing the edge: turn again instead of moving
			begin_pause(e, rng);
		}
	}

	if (e.state == entity::phase::moving)
	{
		const f32  move_time = 1.0f / e.speed;
		const f32  t         = std::min(e.timer / move_time, 1.0f);
		const vec2 from      = layout.cell_pos(e.cell);
		const vec2 to        = layout.cell_pos(e.target);

		e.position = {std::lerp(from.x, to.x, t), std::lerp(from.y, to.y, t)};

		if (e.timer >= move_time)
		{
			e.timer -= move_time;
			e.cell     = e.target;
			e.position = to;
			begin_pause(e, rng);
		}
	}
}

// Returns the cell under the point, or nullopt if outside the grid or in a margin gap
[[nodiscard]] constexpr std::optional<grid_cell> cell_at(
  f32 px, f32 py, f32 origin_x, f32 origin_y, extent<u16> grid, f32 margin, u16 block_size) noexcept
{
	const f32 stride = static_cast<f32>(block_size) + margin;

	const f32 lx = px - origin_x;
	const f32 ly = py - origin_y;

	if (lx < 0.0f || ly < 0.0f)
		return std::nullopt;

	const u32 cx = static_cast<u32>(lx / stride);
	const u32 cy = static_cast<u32>(ly / stride);

	if (cx >= grid.width || cy >= grid.height)
		return std::nullopt;

	// reject the margin gap between blocks
	if (lx - static_cast<f32>(cx) * stride >= static_cast<f32>(block_size)
		|| ly - static_cast<f32>(cy) * stride >= static_cast<f32>(block_size))
		return std::nullopt;

	return grid_cell{cx, cy};
}

void render_grid(deckard::app2::renderer2* r, const grid_layout& l, f32 mx, f32 my) noexcept
{
	const f32  stride  = l.stride();
	const f32  bs      = static_cast<f32>(l.block_size);
	const auto hovered = l.cell_at(mx, my);

	const f32 total_w = static_cast<f32>(l.grid.width) * stride + l.margin;
	const f32 total_h = static_cast<f32>(l.grid.height) * stride + l.margin;

	r->draw_sprite(l.origin_x - l.margin, l.origin_y - l.margin, {total_w, total_h}, {0, 0, 0});

	for (u32 y = 0; y < l.grid.height; ++y)
	{
		for (u32 x = 0; x < l.grid.width; ++x)
		{
			const auto [px, py] = l.cell_pos({x, y});

			if (hovered == grid_cell{x, y})
			{
				constexpr f32 pad = 4.0f;
				r->draw_sprite(px - pad, py - pad, {bs + pad * 2.0f, bs + pad * 2.0f}, {255, 0, 255});
			}

			r->draw_sprite(px, py, {bs, bs}, {255, 255, 0});
		}
	}
}

void render_entity(deckard::app2::renderer2* render, const entity& e, f32 alpha) noexcept
{
	const vec2 pos{std::lerp(e.prev_position.x, e.position.x, alpha), std::lerp(e.prev_position.y, e.position.y, alpha)};

	render->draw_sprite(pos.x, pos.y, {e.size.x, e.size.y}, {e.color.r, e.color.g, e.color.b});

	// direction marker: use pos instead of e.position
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

void render_entities(deckard::app2::renderer2* render, std::span<const entity> entities, f32 alpha) noexcept
{
	for (const auto& e : entities)
		render_entity(render, e, alpha);
}

struct world
{
	grid_layout           layout{};
	std::mt19937          rng{std::random_device{}()};
	std::array<entity, 3> entities{};
	f32                   alpha{0.0f};
	f32                   accumulator{0.0f};
};


[[nodiscard]] world make_world()
{
	world w;
	w.entities = {
	  make_entity(w.layout, {0, 0}, w.rng),
	  make_entity(w.layout, {4, 4}, w.rng),
	  make_entity(w.layout, {7, 2}, w.rng),
	};
	return w;
}

world g_world = make_world();

// simulation only, no renderer, no input polling
void update(world& w, f32 delta) noexcept
{
	for (auto& e : w.entities)
		update_entity(e, 1.0f / 20.0f, w.layout, w.rng);
}

void advance(world& w, f32 delta) noexcept
{
	w.accumulator += delta;
	while (w.accumulator >= 1.0f / 20.0f)
	{
		update(w, 1.0f / 20.0f);
		w.accumulator -= 1.0f / 20.0f;
	}
	w.alpha = w.accumulator / (1.0f / 20.0f);
}

void render_world(deckard::app2::renderer2* r, const world& w, f32 mx, f32 my) noexcept
{
	render_grid(r, w.layout, mx, my);
	render_entities(r, w.entities, w.alpha);
}
grid_layout  layout{};
std::mt19937 rng{std::random_device{}()};

std::array entities{
  make_entity(layout, {0, 0}, rng),
  make_entity(layout, {4, 4}, rng),
  make_entity(layout, {7, 2}, rng),
};

void render(deckard::app2::renderer2* render, [[maybe_unused]] f32 delta) noexcept
{
	POINT p{};
	GetCursorPos(&p);
	ScreenToClient(render->get_handle(), &p);

	const auto size = render->get_size();
	const f32  mx   = static_cast<f32>(p.x);
	const f32  my   = static_cast<f32>(p.y);


	advance(g_world, delta);

	// background

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
	//render_entities(render, g_world.entities, g_world.alpha);
}

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


	// usage

	enum class TestCounter : u32
	{
		fps,
		ui,
		network,
		count
	};

	constexpr std::array intervals{
	  1.0f / 60.0f, // fps
	  1.0f / 30.0f, // ui
	  1.0f / 5.0f,  // network
	};


	fixed_timers<TestCounter, intervals> timer{};

	f32 u = timer[TestCounter::ui];

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

	app::on_tick(
	  [&](const f32 delta) noexcept
	  {
		  app::title(std::format(
			"Delta: {:<3.5f} - FPS: {:<8.2f} - Client: {} - Tick {:8.2f}",
			app::delta_time(),
			app::fps(),
			app::size(),
			1.0f / delta));

		  if (app::was_key_pressed(VK_F11))
			  app::fullscreen(not app::fullscreen());

		  if (app::was_key_pressed(VK_ESCAPE))
			  app::close();
	  });

	u32 uiticks  = 0;
	u32 netticks = 0;


	f32                       logic_delta    = 1.0f / 20.0f; // 20 ticks per second
	f32                       max_frame_time = 0.25f;        // seconds
	f32                       accumulator    = 0.0f;
	deckard::app2::renderer2* render         = &app::get_renderer();
	while (app::running())
	{

		const f32 delta = app::delta_time();

		timer.update(delta);
		accumulator += delta;

		while (accumulator >= logic_delta)
		{
			update(g_world, logic_delta);
			accumulator -= logic_delta;
		}
		g_world.alpha = accumulator / logic_delta;


		// app::close();

		if (app::was_key_pressed(VK_F11))
		{
			app::fullscreen(not app::fullscreen());
		}


		if (app::was_key_pressed(VK_ESCAPE))
		{
			app::close();
		}
	}

	dbg::println("ratio: {}", uiticks / static_cast<f32>(netticks));
	return 0;
}
