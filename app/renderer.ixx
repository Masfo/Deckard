export module deckard.app2:renderer;

import std;
import deckard.types;
import deckard.colors;
import deckard.debug;
import deckard.vulkan;

export namespace deckard::app2
{
	export class renderer2;

	using render_callback = std::move_only_function<void(renderer2* render, f32 delta_time) noexcept>;

	export class renderer2
	{
	private:
		using vertex2d = vulkan::vertex2d;

		static constexpr f32 pi{std::numbers::pi_v<f32>};
		static constexpr u32 cap_segments{8};
		static constexpr u32 quad_vertices{6};
		static constexpr u32 cap_vertices{cap_segments * 3};
		static constexpr f32 round_cap_min_half_width{1.5f}; // thinner lines look the same with butt caps

		vulkan::context* ctx{nullptr};
		extent<u16>                 size{1920, 1080};
		f32                         hue{0.0f};
		std::array<f32, 3>          clear_color{0.0f, 0.0f, 0.0f};
		std::vector<vertex2d>       vertices;
		bool                        overflow_reported{false};

	private:
		[[nodiscard]] static constexpr u32 pack(const std::array<u8, 3>& c, u8 a = 255) noexcept
		{
			return u32{c[0]} | (u32{c[1]} << 8) | (u32{c[2]} << 16) | (u32{a} << 24);
		}

		void update(f32 delta_time)
		{
			hue += 90.0f * delta_time;
			hue = std::fmod(hue, 360.0f);
			if (hue < 0.0f)
				hue += 360.0f;
		}

		bool reserve_vertices(std::size_t count)
		{
			if (vertices.size() + count <= vulkan::max_batch_vertices)
				return true;

			if (not overflow_reported)
			{
				overflow_reported = true;
				dbg::println("renderer: 2D batch full ({} vertices), dropping draws", vulkan::max_batch_vertices);
			}
			return false;
		}

		void push_triangle(f32 ax, f32 ay, f32 bx, f32 by, f32 cx, f32 cy, u32 rgba)
		{
			vertices.push_back({{ax, ay}, rgba});
			vertices.push_back({{bx, by}, rgba});
			vertices.push_back({{cx, cy}, rgba});
		}

		// half-circle fan around (cx, cy), bulging towards (dir_x, dir_y)
		void push_cap(f32 cx, f32 cy, f32 dir_x, f32 dir_y, f32 radius, u32 rgba)
		{
			const f32 nx = -dir_y;
			const f32 ny = dir_x;

			f32 prev_x = cx + nx * radius;
			f32 prev_y = cy + ny * radius;

			for (u32 i = 1; i <= cap_segments; ++i)
			{
				const f32 a  = pi * static_cast<f32>(i) / static_cast<f32>(cap_segments);
				const f32 px = cx + (nx * std::cos(a) + dir_x * std::sin(a)) * radius;
				const f32 py = cy + (ny * std::cos(a) + dir_y * std::sin(a)) * radius;

				push_triangle(cx, cy, prev_x, prev_y, px, py, rgba);
				prev_x = px;
				prev_y = py;
			}
		}

	public:
		render_callback on_render;

	public:
		void initialize(vulkan::context& context, extent<u16> initial_size)
		{
			ctx  = &context;
			size = initial_size;
			vertices.reserve(4096);
			dbg::println("renderer: initialized with size {}x{}", size.width, size.height);
		}

		void deinitialize()
		{
			ctx = nullptr;
			vertices.clear();
			vertices.shrink_to_fit();
			dbg::println("renderer: deinitialized");
		}

		void resize(extent<u16> new_size) { notify_size(new_size); }

		void notify_size(extent<u16> new_size) { size = new_size; }

		extent<u16> get_size() const { return size; }

		void begin_frame()
		{
			vertices.clear();
			overflow_reported = false;
		}

		[[nodiscard]] bool end_frame()
		{
			if (ctx == nullptr)
				return false;

			return ctx->draw(vertices, clear_color);
		}

		[[nodiscard]] bool render(f32 delta)
		{
			update(delta);

			begin_frame();
			_ = invoke_if(on_render, this, delta);
			return end_frame();
		}

		void clear(const std::array<u8, 3>& color = {0, 0, 0})
		{
			vertices.clear();
			clear_color = {color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f};
		}

		void draw_line(f32 x0, f32 y0, f32 x1, f32 y1, const std::array<u8, 3>& color, f32 thickness = 1.0f)
		{
			const f32  half       = std::max(1.0f, thickness) * 0.5f;
			const bool round_caps = half >= round_cap_min_half_width;

			if (not reserve_vertices(quad_vertices + (round_caps ? 2 * cap_vertices : 0)))
				return;

			x0 += 0.5f;
			y0 += 0.5f;
			x1 += 0.5f;
			y1 += 0.5f;

			const f32 dx  = x1 - x0;
			const f32 dy  = y1 - y0;
			const f32 len = std::hypot(dx, dy);
			const f32 ux  = len > 1e-6f ? dx / len : 1.0f; 
			const f32 uy  = len > 1e-6f ? dy / len : 0.0f;
			const f32 nx  = -uy * half;
			const f32 ny  = ux * half;

			const u32 rgba = pack(color);

			push_triangle(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, rgba);
			push_triangle(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, rgba);

			if (round_caps)
			{
				push_cap(x0, y0, -ux, -uy, half, rgba);
				push_cap(x1, y1, ux, uy, half, rgba);
			}
		}

		void draw_sprite(f32 x, f32 y, std::array<f32, 2> sprite_size, const std::array<u8, 3>& color)
		{
			if (not reserve_vertices(quad_vertices))
				return;

			const f32 x1   = x + sprite_size[0];
			const f32 y1   = y + sprite_size[1];
			const u32 rgba = pack(color);

			push_triangle(x, y, x1, y, x1, y1, rgba);
			push_triangle(x, y, x1, y1, x, y1, rgba);
		}
	};
} // namespace deckard::app2
