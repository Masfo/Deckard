export module deckard.app:renderer;

import std;
import deckard.types;
import deckard.colors;
import deckard.debug;
import deckard.image;
import deckard.random;
import deckard.vulkan;
import deckard.geometry;
import deckard.math;
import deckard.vec;

export namespace deckard::app
{
	export class render;


	export class render
	{
	private:
		using vertex2d = vulkan::vertex2d;
		using vec3     = vec::vec3;
		using mat4     = matrix::mat4;

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
		std::vector<vulkan::mesh_draw> mesh_draws;
		std::array<f32, 3>          triangle_angles{};
		std::array<f32, 3>          triangle_angular_speeds{};
		bool                        overflow_reported{false};

		// Default camera looks at the origin from slightly above and in front of it.
		vec3 camera_position{0.0f, 1.5f, 4.0f};
		vec3 camera_target{0.0f, 0.0f, 0.0f};
		vec3 camera_up{0.0f, 1.0f, 0.0f};
		f32  camera_vertical_fov{60.0f};
		f32  camera_near{0.1f};
		f32  camera_far{100.0f};

		static constexpr std::array<std::array<f32, 2>, 3> triangle_origins{{
		  {{0.55f, -0.75f}}, {{0.80f, -0.25f}}, {{0.30f, -0.25f}}}};
		static constexpr std::array<std::array<f32, 2>, 3> triangle_uvs{{
		  {{0.5f, 0.0f}}, {{1.0f, 1.0f}}, {{0.0f, 1.0f}}}};

	private:
		[[nodiscard]] static constexpr u32 pack(const std::array<u8, 3>& c, u8 a = 255) noexcept
		{
			return u32{c[0]} | (u32{c[1]} << 8) | (u32{c[2]} << 16) | (u32{a} << 24);
		}

		[[nodiscard]] mat4 view_projection() const noexcept
		{
			const f32 aspect = size.height != 0 ? static_cast<f32>(size.width) / static_cast<f32>(size.height) : 1.0f;

			mat4 projection
			  = matrix::perspective(math::to_radians(camera_vertical_fov), aspect, camera_near, camera_far);

			// Vulkan clip space has y pointing down; flip the projection instead of every vertex.
			projection[1] = projection[1] * -1.0f;

			return projection * matrix::lookat_rh(camera_position, camera_target, camera_up);
		}

		[[nodiscard]] std::array<f32, 16> view_projection_data() const noexcept
		{
			const mat4 matrix = view_projection();
			std::array<f32, 16> result{};
			std::memcpy(result.data(), matrix.mat.data(), sizeof(result));
			return result;
		}

		void update(f32 delta_time)
		{
			for (std::size_t i = 0; i < triangle_angles.size(); ++i)
				triangle_angles[i] += triangle_angular_speeds[i] * delta_time;
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
			vertices.push_back({{ax, ay}, {0.0f, 0.0f}, rgba, 0});
			vertices.push_back({{bx, by}, {0.0f, 0.0f}, rgba, 0});
			vertices.push_back({{cx, cy}, {0.0f, 0.0f}, rgba, 0});
		}

		void push_quad(const geometry::rect& r, u32 top_rgba, u32 bottom_rgba)
		{
			const auto corners = r.vertices();
			vertices.push_back({{corners[0].x, corners[0].y}, {0.0f, 0.0f}, top_rgba, 0});
			vertices.push_back({{corners[1].x, corners[1].y}, {0.0f, 0.0f}, top_rgba, 0});
			vertices.push_back({{corners[2].x, corners[2].y}, {0.0f, 0.0f}, bottom_rgba, 0});
			vertices.push_back({{corners[0].x, corners[0].y}, {0.0f, 0.0f}, top_rgba, 0});
			vertices.push_back({{corners[2].x, corners[2].y}, {0.0f, 0.0f}, bottom_rgba, 0});
			vertices.push_back({{corners[3].x, corners[3].y}, {0.0f, 0.0f}, bottom_rgba, 0});
		}

		void push_textured_quad(
		  f32 x, f32 y, f32 width, f32 height, vulkan::texture_id texture, const std::array<u8, 3>& tint)
		{
			const f32 x1 = x + width;
			const f32 y1 = y + height;
			const u32 rgba = pack(tint);
			const auto push = [&](f32 px, f32 py, f32 u, f32 v)
			{ vertices.push_back({{px, py}, {u, v}, rgba, texture.value}); };

			push(x, y, 0.0f, 0.0f);
			push(x1, y, 1.0f, 0.0f);
			push(x1, y1, 1.0f, 1.0f);
			push(x, y, 0.0f, 0.0f);
			push(x1, y1, 1.0f, 1.0f);
			push(x, y1, 0.0f, 1.0f);
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
		using render_callback = std::move_only_function<void(app::render* render, f32 delta_time) noexcept>;

		render_callback user_render;

	public:
		void initialize(vulkan::context& context, extent<u16> initial_size)
		{
			ctx  = &context;
			size = initial_size;
			vertices.reserve(4096);
			mesh_draws.reserve(256);
			for (std::size_t i = 0; i < triangle_angles.size(); ++i)
			{
				triangle_angles[i] = random::rnd<f32>(0.0f, std::numbers::pi_v<f32> * 2.0f);
				const f32 speed = random::rnd<f32>(1.0f, 3.0f);
				triangle_angular_speeds[i] = random::randbool() ? speed : -speed;
			}
			dbg::println("renderer: initialized with size {}x{}", size.width, size.height);
		}

		void deinitialize()
		{
			ctx = nullptr;
			vertices.clear();
			vertices.shrink_to_fit();
			mesh_draws.clear();
			mesh_draws.shrink_to_fit();
			dbg::println("renderer: deinitialized");
		}

		void resize(extent<u16> new_size) { notify_size(new_size); }

		void notify_size(extent<u16> new_size) { size = new_size; }

		extent<u16> get_size() const { return size; }

		void begin_frame()
		{
			vertices.clear();
			mesh_draws.clear();
			overflow_reported = false;
		}

		[[nodiscard]] bool end_frame()
		{
			if (ctx == nullptr)
				return false;

			return ctx->draw(vertices, mesh_draws, clear_color, view_projection_data());
		}

		[[nodiscard]] bool do_render(f32 delta)
		{
			update(delta);

			begin_frame();
			_ = invoke_if(user_render, this, delta);
			return end_frame();
		}

		[[nodiscard]] std::expected<image_rgb, std::string> screenshot()
		{
			if (ctx == nullptr)
				return std::unexpected("Renderer must be initialized before taking a screenshot");

			return ctx->screenshot(vertices, mesh_draws, clear_color, view_projection_data());
		}

		void clear(const std::array<u8, 3>& color = {0, 0, 0})
		{
			vertices.clear();
			mesh_draws.clear();
			clear_color = {color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f};
		}

		// #########################################################
		// 3D meshes ###############################################

		// Sets the camera used for mesh rendering. FOV is vertical and in degrees.
		void set_camera(
		  const vec3& position, const vec3& target, const vec3& up = vec3{0.0f, 1.0f, 0.0f},
		  f32 vertical_fov_degrees = 60.0f, f32 near_plane = 0.1f, f32 far_plane = 100.0f) noexcept
		{
			camera_position       = position;
			camera_target         = target;
			camera_up             = up;
			camera_vertical_fov   = vertical_fov_degrees;
			camera_near           = near_plane;
			camera_far            = far_plane;
		}

		void set_light(const vulkan::light& source) noexcept
		{
			if (ctx != nullptr)
				ctx->set_light(source);
		}

		[[nodiscard]] vulkan::light get_light() const noexcept
		{
			return ctx != nullptr ? ctx->get_light() : vulkan::light{};
		}

		[[nodiscard]] vec3 get_camera_position() const noexcept { return camera_position; }

		[[nodiscard]] vec3 get_camera_target() const noexcept { return camera_target; }

		// Uploads mesh data to the GPU once; the returned id can be drawn any number of times.
		[[nodiscard]] std::expected<vulkan::mesh_id, std::string> create_mesh(const vulkan::mesh& source)
		{
			if (ctx == nullptr)
				return std::unexpected("Renderer must be initialized before creating meshes");
			return ctx->upload_mesh(source);
		}

		// Draws a mesh at a world-space position. `rotation` is in radians (XYZ order), `scale` per-axis.
		void draw_mesh(
		  vulkan::mesh_id mesh, const vec3& position, const vec3& rotation = vec3::zero(),
		  const vec3& scale = vec3::one(), const std::array<u8, 3>& color = {255, 255, 255},
		  vulkan::texture_id texture = {}, f32 emissive = 0.0f)
		{
			if (ctx == nullptr or not mesh)
				return;

			const mat4 model = mat4::identity()
								.translate(position)
								.rotate(rotation.x, {1.0f, 0.0f, 0.0f})
								.rotate(rotation.y, {0.0f, 1.0f, 0.0f})
								.rotate(rotation.z, {0.0f, 0.0f, 1.0f})
								.scale(scale);

			vulkan::mesh_draw draw{};
			draw.id = mesh;
			draw.texture = texture;

			// Per-axis scale can be non-uniform, so normals need the inverse-transpose of the model matrix.
			const mat4 normal_matrix = matrix::transpose(matrix::inverse(model));
			for (std::size_t column = 0; column < 3; ++column)
			{
				draw.normal[column * 4 + 0] = normal_matrix[column].x;
				draw.normal[column * 4 + 1] = normal_matrix[column].y;
				draw.normal[column * 4 + 2] = normal_matrix[column].z;
			}

			std::memcpy(draw.model.data(), model.mat.data(), sizeof(draw.model));

			draw.color = {
			  color[0] / 255.0f,
			  color[1] / 255.0f,
			  color[2] / 255.0f,
			  emissive};

			mesh_draws.push_back(draw);
		}

		void draw_mesh(
		  vulkan::mesh_id mesh, f32 x, f32 y, f32 z, const std::array<u8, 3>& color = {255, 255, 255},
		  vulkan::texture_id texture = {}, f32 emissive = 0.0f)
		{
			draw_mesh(mesh, vec3{x, y, z}, vec3::zero(), vec3::one(), color, texture, emissive);
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

		[[nodiscard]] std::expected<vulkan::texture_id, std::string> load_texture(const std::filesystem::path& path)
		{
			if (ctx == nullptr)
				return std::unexpected("Renderer must be initialized before loading textures");
			auto image = deckard::image::load_png(path);
			if (not image)
				return std::unexpected(image.error());
			return ctx->upload_image(*image);
		}

		void draw_sprite(
		  vulkan::texture_id texture, f32 x, f32 y, std::array<f32, 2> sprite_size,
		  const std::array<u8, 3>& tint = {255, 255, 255})
		{
			if (not texture or not reserve_vertices(quad_vertices))
				return;
			push_textured_quad(x, y, sprite_size[0], sprite_size[1], texture, tint);
		}

		void draw_sprite(
		  f32 x, f32 y, std::array<f32, 2> sprite_size, vulkan::texture_id texture,
		  const std::array<u8, 3>& tint = {255, 255, 255})
		{
			draw_sprite(texture, x, y, sprite_size, tint);
		}

		void draw_triangle(vulkan::texture_id texture) noexcept
		{
			if (not texture or not reserve_vertices(3))
				return;

			constexpr f32 orbit_radius = 0.06f;
			const u32 tint = pack({255, 255, 255});
			for (std::size_t i = 0; i < triangle_origins.size(); ++i)
			{
				const f32 offset_x = orbit_radius * std::cos(triangle_angles[i]);
				const f32 offset_y = orbit_radius * std::sin(triangle_angles[i]);
				const f32 ndc_x = triangle_origins[i][0] + offset_x;
				const f32 ndc_y = triangle_origins[i][1] + offset_y;
				vertices.push_back({
				  {(ndc_x + 1.0f) * static_cast<f32>(size.width) * 0.5f,
				   (ndc_y + 1.0f) * static_cast<f32>(size.height) * 0.5f},
				  {triangle_uvs[i][0], triangle_uvs[i][1]}, tint, texture.value});
			}
		}

		void rectangle(f32 x, f32 y, f32 width, f32 height, const std::array<u8, 3>& color)
		{
			draw_sprite(x, y, {width, height}, color);
		}

		void rectangle(const geometry::rect& r, const std::array<u8, 3>& color)
		{
			if (not reserve_vertices(quad_vertices))
				return;

			const u32 rgba = pack(color);
			push_quad(r, rgba, rgba);
		}

		void rectangle(
		  f32 x, f32 y, f32 width, f32 height,
		  const std::array<u8, 3>& top_color,
		  const std::array<u8, 3>& bottom_color)
		{
			if (not reserve_vertices(quad_vertices))
				return;

			const f32 x1          = x + width;
			const f32 y1          = y + height;
			const u32 top_rgba    = pack(top_color);
			const u32 bottom_rgba = pack(bottom_color);

			vertices.push_back({{x, y}, {0.0f, 0.0f}, top_rgba, 0});
			vertices.push_back({{x1, y}, {0.0f, 0.0f}, top_rgba, 0});
			vertices.push_back({{x1, y1}, {0.0f, 0.0f}, bottom_rgba, 0});

			vertices.push_back({{x, y}, {0.0f, 0.0f}, top_rgba, 0});
			vertices.push_back({{x1, y1}, {0.0f, 0.0f}, bottom_rgba, 0});
			vertices.push_back({{x, y1}, {0.0f, 0.0f}, bottom_rgba, 0});
		}

		void rectangle(
		  const geometry::rect& r,
		  const std::array<u8, 3>& top_color,
		  const std::array<u8, 3>& bottom_color)
		{
			if (not reserve_vertices(quad_vertices))
				return;

			push_quad(r, pack(top_color), pack(bottom_color));
		}

		void draw_rectangle(f32 x, f32 y, f32 width, f32 height, const std::array<u8, 3>& color)
		{
			rectangle(x, y, width, height, color);
		}

		void draw_rectangle(const geometry::rect& r, const std::array<u8, 3>& color)
		{
			rectangle(r, color);
		}

		void draw_rectangle(
		  f32 x, f32 y, f32 width, f32 height,
		  const std::array<u8, 3>& top_color,
		  const std::array<u8, 3>& bottom_color)
		{
			rectangle(x, y, width, height, top_color, bottom_color);
		}

		void draw_rectangle(
		  const geometry::rect& r,
		  const std::array<u8, 3>& top_color,
		  const std::array<u8, 3>& bottom_color)
		{
			rectangle(r, top_color, bottom_color);
		}

		void button(f32 x, f32 y, f32 width, f32 height, bool highlight)
		{
			if (not reserve_vertices(quad_vertices * 2))
				return;

			constexpr f32 pad = 2.0f;

			if (highlight)
				rectangle(x - pad, y - pad, width + pad * 2.0f, height + pad * 2.0f, {255, 255, 255});
			else
				rectangle(x - pad, y - pad, width + pad * 2.0f, height + pad * 2.0f, {30, 30, 35});

			const std::array<u8, 3> top_color    = highlight ? std::array<u8, 3>{85, 85, 90} : std::array<u8, 3>{60, 60, 65};
			const std::array<u8, 3> bottom_color = highlight ? std::array<u8, 3>{60, 60, 65} : std::array<u8, 3>{40, 40, 45};

			rectangle(x, y, width, height, top_color, bottom_color);
		}

		void button(const geometry::rect& r, bool highlight)
		{
			if (not reserve_vertices(quad_vertices * 2))
				return;

			constexpr f32 pad = 2.0f;
			auto outer = r;
			outer.size.x += pad * 2.0f;
			outer.size.y += pad * 2.0f;

			const std::array<u8, 3> top_color    = highlight ? std::array<u8, 3>{85, 85, 90} : std::array<u8, 3>{60, 60, 65};
			const std::array<u8, 3> bottom_color = highlight ? std::array<u8, 3>{60, 60, 65} : std::array<u8, 3>{40, 40, 45};

			rectangle(outer, highlight ? std::array<u8, 3>{255, 255, 255} : std::array<u8, 3>{30, 30, 35});
			rectangle(r, top_color, bottom_color);
		}

		void draw_button(f32 x, f32 y, f32 width, f32 height, bool highlight)
		{
			button(x, y, width, height, highlight);
		}

		void draw_button(const geometry::rect& r, bool highlight)
		{
			button(r, highlight);
		}
	};
} // namespace deckard::app
