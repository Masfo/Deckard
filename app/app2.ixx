
export module deckard.app2;
import :window;
import :renderer;

import std;
import deckard.types;
#ifndef DECKARD_APP2
#define DECKARD_APP2
#endif

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
	namespace detail
	{
		app::window   window;
		app::renderer renderer;
	}; // namespace detail

	export struct options
	{
		u16         width{1920};
		u16         height{1080};
		bool        fullscreen{false};
		std::string title{"Deckard Application"};
	};

	export [[deprecated("use new app2 instead, rename app2 to app, delete old")]] auto open(const options opt = {})
	  -> std::expected<void, std::string>
	{
		if (auto result = detail::window.initialize(opt.width, opt.height, opt.fullscreen, opt.title); not result)
			return std::unexpected(result.error());


		return {};
	}

	export bool running() 
	{
		if (detail::window.is_invalidated())
		{
			dbg::println("window is invalidated");
			detail::window.clear_invalidated();
		}


		return detail::window.running(); 
	}

	export void close() { detail::window.close(); }

	// example of a quick running app
	bool quickrunning()
	{
		if (not detail::window.is_open())
		{
			if (auto result = detail::window.initialize(1920, 1080, false, "Deckard Application"); not result)
				return false;
		}



		return detail::window.running();
	}

	export [[nodiscard]] renderer& get_renderer() { return detail::renderer; }


} // namespace deckard::app
