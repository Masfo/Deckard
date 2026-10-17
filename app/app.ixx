export module deckard.app;
export import :window;
export import :renderer;
export import :inputs;

import std;
import deckard.callbacks;
import deckard.config;
import deckard.types;
import deckard.timers;
import deckard.vulkan;
import deckard.image;

// vulkan window api design:
//  namespace deckard
//			* app::open({.width=1920, .height=1080, .title="Deckard Application"});
//
//			* while(app::running())
//
//			* begin_frame() / end_frame()
//
//
// Keep Vulkan as low level (commandbuffer, alloc buffer), build higher level top of it, renderer, scene


namespace deckard::app
{

	// TODO: fixed_callbacks for preupdate, update, fixedupdate


	using namespace deckard::literals;
	using namespace std::chrono_literals;

	inline namespace detail
	{
		app::window     window;
		app::inputs     inputs;
		app::render m_render;
		vulkan::context vulkan;

		u64 frames{0};

		frame_timer fps_timer;
		frame_timer resize_timer;
		bool        is_resizing{false};


		constexpr i32 max_ticks_per_frame{5};
		f32           tick_accumulator{0.0f};
		constexpr f32 tick_rate{60.0f};
		f32           fixed_delta_time{1.0f / tick_rate};

		f32           delta_time{0.0f};
		constexpr f32 target_fps{60.0f};
		f32           fps{target_fps};
		bool          limit_fps{false};

		f32 input_poll_timer{0.0f};


		constexpr f32 target_delta{1.0f / target_fps};

		constexpr f32 fixed_delta{target_delta};
		constexpr f32 fps_alpha{0.50f};

		void update_fps(f32 delta) noexcept
		{
			if (delta > 0.0f)
				detail::fps = std::lerp(detail::fps, 1.0f / delta, detail::fps_alpha);
		}

		constexpr u32 MAX_CALLBACKS{8};

		void sync_swapchain() noexcept
		{
			if (is_resizing)
				return;

			if (window.is_invalidated() or vulkan.needs_resize())
			{
				if (vulkan.resize())
				{
					m_render.notify_size(window.get_clientsize());
					window.clear_invalidated();
				}
			}
		}

		using callback_t = std::move_only_function<void(f32 delta) noexcept>;

		fixed_callbacks<MAX_CALLBACKS, callback_t> startup;

		fixed_callbacks<MAX_CALLBACKS, callback_t> pre_update;
		fixed_callbacks<MAX_CALLBACKS, callback_t> update;
		fixed_callbacks<MAX_CALLBACKS, callback_t> post_update;
		fixed_callbacks<MAX_CALLBACKS, callback_t> pre_update_fixed;
		fixed_callbacks<MAX_CALLBACKS, callback_t> update_fixed;
		fixed_callbacks<MAX_CALLBACKS, callback_t> post_update_fixed;
		fixed_callbacks<MAX_CALLBACKS, callback_t> pre_render;
		fixed_callbacks<MAX_CALLBACKS, callback_t> render_callbacks;
		fixed_callbacks<MAX_CALLBACKS, callback_t> post_render;

		using tick_fn = std::move_only_function<void(f32 delta) noexcept>;
		tick_fn on_tick_callback;

	} // namespace detail

	// ########################
	// callbacks
	using initialize_callback = std::move_only_function<bool() noexcept>;
	using render_callback     = std::move_only_function<void(app::render* render, f32 delta_time) noexcept>;
	using destroy_callback    = std::move_only_function<void() noexcept>;
	using tick_callback       = std::move_only_function<void(f32 delta) noexcept>;

	export void on_tick(tick_callback callback) { detail::on_tick_callback = std::move(callback); }

	// ########################

	export struct options
	{
		extent<u16> size{1920, 1080};
		bool        fullscreen{false};
		bool        resizable{true};
		bool        vsync{true};
		bool        allow_win_key{true};
		u8          monitor{0};
		std::string title{"Deckard Application"};

		config cfg;

		void load_and_save()
		{
			// set defaults
			cfg.set("window.width", size.width);
			cfg.set("window.height", size.height);
			cfg.set("window.fullscreen", fullscreen);
			cfg.set("window.resizable", resizable);
			cfg.set("window.vsync", vsync);
			cfg.set("window.allow_win_key", allow_win_key);
			cfg.set("window.monitor", monitor);
			cfg.set("window.title", title);

			cfg.load_from_file("settings.txt"_path);
			size.width    = cfg.get<u16>("window.width", size.width);
			size.height   = cfg.get<u16>("window.height", size.height);
			fullscreen    = cfg.get<bool>("window.fullscreen", fullscreen);
			resizable     = cfg.get<bool>("window.resizable", resizable);
			vsync         = cfg.get<bool>("window.vsync", vsync);
			allow_win_key = cfg.get<bool>("window.allow_win_key", allow_win_key);
			monitor       = cfg.get<u8>("window.monitor", monitor);
			title         = cfg.get<std::string>("window.title", title);

			_ = cfg.save("settings.txt"_path);
		}

		void save()
		{
			cfg.set("window.width", size.width);
			cfg.set("window.height", size.height);
			cfg.set("window.fullscreen", fullscreen);
			cfg.set("window.resizable", resizable);
			cfg.set("window.vsync", vsync);
			cfg.set("window.allow_win_key", allow_win_key);
			cfg.set("window.monitor", monitor);
			cfg.set("window.title", title);
			_ = cfg.save("settings.txt"_path);
		}
	};

	// ########################


	export auto open(const options& opt) -> std::expected<void, std::string>
	{

		// input
		if (auto result = detail::inputs.initialize(); not result)
			return std::unexpected(result.error());

		// window
		if (auto result = detail::window.initialize(opt.size, opt.fullscreen, opt.monitor, opt.title); not result)
			return std::unexpected(result.error());

		// vulkan
		detail::vulkan.set_debug_callback(
		  [](const std::string_view severity, const std::string_view type, std::string_view message) noexcept
		  { dbg::println("Vulkan [{}] [{}]: {}", severity, type, message); });
		detail::vulkan.set_vsync(opt.vsync);


		if (auto result = detail::vulkan.initialize(detail::window.get_handle()); not result)
			return std::unexpected(result.error());

		detail::m_render.initialize(detail::vulkan, opt.size);

		// setup window and render
		detail::window.set_resizable(opt.resizable);
		detail::window.allow_win_key(opt.allow_win_key);

		detail::window.set_input(&detail::inputs);

		detail::window.on_size = [](extent<u16>) noexcept
		{
			if (not detail::window.is_open())
				return;
			detail::inputs.poll(0.0f);

			f32 delta = 0.0f;
			if (detail::is_resizing)
			{
				delta = detail::resize_timer.tick();
				delta = std::min(delta, detail::target_delta * 4.0f);
			}

			detail::delta_time = delta;
#if 0
				detail::vulkan.advance_triangle2(delta);
				detail::vulkan.draw();
#endif
			// detail::render.render(delta);

			_ = invoke_if(detail::on_tick_callback, delta);
		};

		detail::window.on_resize_begin = []() noexcept { detail::is_resizing = true; };

		detail::window.on_resize_end = []() noexcept { detail::is_resizing = false; };


		//
		detail::window.process_frame = []() noexcept
		{
			if (not detail::window.is_running())
			{
				detail::inputs.deinitialize();
				detail::vulkan.deinitialize();
				detail::m_render.deinitialize();
				detail::window.deinitialize();
				return false;
			}

			//


			// update
			detail::delta_time = detail::fps_timer.tick();
			detail::delta_time = std::min(detail::delta_time, 0.25f);

			detail::inputs.poll(detail::delta_time);
			detail::frames++;


			_ = invoke_if(detail::on_tick_callback, detail::delta_time);


			detail::update_fps(detail::delta_time);

			const auto client    = detail::window.get_clientsize();
			const bool minimized = client.width == 0 or client.height == 0;

			if (minimized)
			{
				std::this_thread::sleep_for(10ms); // don't spin while minimized
			}
			else
			{
				detail::sync_swapchain();
				if (not detail::m_render.do_render(detail::delta_time))
					return false;
			}


			return true;
		};


		// vulkan


		detail::fps_timer.reset();
		detail::resize_timer.reset();
		detail::delta_time = 0.0f;
		detail::fps        = 0.0f;

		return {};
	}

	export void close() { detail::window.close(); }

	export [[nodiscard]] std::expected<vulkan::texture_id, std::string> load_texture(const std::filesystem::path& path)
	{
		return detail::vulkan.load_texture(path);
	}

	export void fullscreen(bool value) { detail::window.set_fullscreen(value); }

	export void resize(extent<u16> size) { detail::window.set_size(size); }

	export [[nodiscard("check if the app if fullscreen")]] bool fullscreen() { return detail::window.is_fullscreen(); }

	export void title(std::string_view title) { detail::window.set_title(title); }

	export [[nodiscard("check if the window is still running")]] bool running() { return detail::window.process_frame(); }

	export [[nodiscard("check the size of the window")]] extent<u16> size() { return detail::window.get_clientsize(); }

	export [[nodiscard]] app::render& get_renderer() { return detail::m_render; }

	export [[nodiscard]] HWND window_handle() { return detail::window.get_handle(); }

	export [[nodiscard]] f32 delta_time() { return detail::delta_time; }

	export [[nodiscard]] f32 fps() { return detail::fps; }

	export [[nodiscard]] void screenshot()
	{
		auto img = detail::m_render.screenshot();
		if (not img)
		{
			dbg::println("Failed to capture screenshot: {}", img.error());
			return;
		}
		std::jthread(
		  [img = *img]()
		  {
			  ScopeTimer timer("screenshot");
			  if (auto result = deckard::image::save_qoi("screenshot.qoi"_path, img); not result)
				  dbg::println("Failed to save screenshot: {}", result.error());
			  else
				  dbg::println("Screenshot saved to screenshot.qoi");
		  })
		  .detach();
	}

	// input

	export [[nodiscard]] bool is_key_down(u32 vk) { return detail::inputs.is_key_down(vk); }

	export [[nodiscard]] bool is_key_up(u32 vk) { return detail::inputs.is_key_up(vk); }

	export [[nodiscard]] bool was_key_pressed(u32 vk) { return detail::inputs.is_key_just_pressed(vk); }

	export [[nodiscard]] bool was_key_released(u32 vk) { return detail::inputs.is_key_just_released(vk); }

	export [[nodiscard]] auto key_buffer() { return detail::inputs.take_character_buffer(); }

	export void on_initialize(initialize_callback callback)
	{
		if (not detail::window.on_initialize)
			detail::window.on_initialize = std::move(callback);
		else
			dbg::println("window: on_initialize callback already set");
	}

	export void on_render(render_callback callback)
	{
		if (not detail::m_render.user_render)
			detail::m_render.user_render = std::move(callback);
		else
			dbg::println("window: on_render callback already set");
	}

	export void on_destroy(destroy_callback callback)
	{
		if (not detail::window.on_destroy)
			detail::window.on_destroy = std::move(callback);
		else
			dbg::println("window: on_destroy callback already set");
	}

} // namespace deckard::app
