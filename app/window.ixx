module;
#include <Windows.h>
#include <windowsx.h>

#include <versionhelpers.h>


export module deckard.app2:window;


import std;
import deckard.as;
import deckard.assert;
import deckard.debug;
import deckard.types;
import deckard.platform;

namespace deckard::app
{

	LRESULT CALLBACK low_level_keyboard_proc(int code, WPARAM wparam, LPARAM lparam) noexcept
	{
		if (code == HC_ACTION)
		{
			auto* info = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);
			if (info->vkCode == VK_LWIN or info->vkCode == VK_RWIN)
				return 1;
		}
		return CallNextHookEx(nullptr, code, wparam, lparam);
	}

	struct monitor
	{
		HMONITOR    handle{nullptr};
		extent<u16> size{0, 0};
		bool        primary{false};
	};

	constexpr std::wstring_view window_class_name = L"DeckardWindowClass";


	using resize_callback = std::move_only_function<void() noexcept>;
	using frame_callback  = std::move_only_function<bool() noexcept>;
	using size_callback   = std::move_only_function<void(extent<u16>) noexcept>;

	using initialize_callback = std::move_only_function<bool() noexcept>;
	using destroy_callback    = std::move_only_function<void() noexcept>;

	constexpr u32 RESIZE_TIMER_ID = 0xDEAD'BEEF;

	export class window
	{
	private:
		HWND handle{nullptr};

		DWORD style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SIZEBOX};
		DWORD ex_style{0};

		LRESULT CALLBACK wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);

		extent<u16>       size{1920, 1080};
		extent<u16>       last_size{1920, 1080};
		const extent<u16> min_size{640, 480};
		extent<u16>       normalized_client_size{0, 0};
		extent<u16>       physical_client_size{0, 0};

		ivec2                window_position{CW_USEDEFAULT, CW_USEDEFAULT};
		std::vector<monitor> monitors;


		bool running{false};
		bool sizing{false};
		bool minimized{false};
		bool show_cursor{true};
		bool invalidated{false};
		bool resizable{true};
		bool fullscreen{false};
		bool active{true};

		bool win_key_allowed{true};

		//
		HHOOK keyboard_hook{};

		WINDOWPLACEMENT wp{};

	private:
		u8 monitor_count() const { return static_cast<u8>(GetSystemMetrics(SM_CMONITORS)); }

		u8 primary_monitor_index() const
		{
			HMONITOR    monitor = MonitorFromWindow(handle, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi{.cbSize = sizeof(MONITORINFO)};
			if (not GetMonitorInfoW(monitor, &mi))
				return 0;
			return static_cast<u8>(mi.dwFlags & MONITORINFOF_PRIMARY ? 0 : 1);
		}

		ivec2 get_window_position() const
		{
			RECT wr{};
			if (GetWindowRect(handle, &wr) == 0)
				return {0, 0};
			return {wr.left, wr.top};
		}

		void get_monitors()
		{
			monitors.clear();

			EnumDisplayMonitors(
			  nullptr,
			  nullptr,
			  [](HMONITOR monitor, HDC, LPRECT, LPARAM lparam) -> BOOL
			  {
				  auto*       mons = reinterpret_cast<decltype(&monitors)>(lparam);
				  extent<u16> size{0, 0};
				  MONITORINFO mi{.cbSize = sizeof(MONITORINFO)};

				  if (GetMonitorInfoW(monitor, &mi))
					  size = to_extent(mi.rcMonitor);

				  bool primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;

				  mons->push_back({monitor, size, primary});

				  return TRUE; // Continue enumeration
			  },
			  reinterpret_cast<LPARAM>(&monitors));
		}

		ivec2 get_monitor_center_for_window(u8 index) const
		{
			HMONITOR monitor
			  = monitors.size() > index ? monitors[index].handle : MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi{.cbSize = sizeof(MONITORINFO)};
			if (not GetMonitorInfoW(monitor, &mi))
				return {CW_USEDEFAULT, CW_USEDEFAULT};

			const RECT& work       = mi.rcWork;
			const int   workWidth  = work.right - work.left;
			const int   workHeight = work.bottom - work.top;

			ivec2 center{work.left + workWidth / 2, work.top + workHeight / 2};
			center.x -= size.width / 2;
			center.y -= size.height / 2;
			return center;
		}

		std::optional<extent<u16>> get_monitor_size(u8 monitor_index) const
		{

			HMONITOR monitor = monitors.size() > monitor_index ? monitors[monitor_index].handle
															   : MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);

			MONITORINFO mi{.cbSize = sizeof(MONITORINFO)};
			if (not GetMonitorInfoW(monitor, &mi))
				return std::nullopt;

			return to_extent(mi.rcMonitor);
		}

		extent<u16> get_current_monitor_size() const

		{
			HMONITOR    monitor = MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi{};
			mi.cbSize = sizeof(mi);
			if (GetMonitorInfo(monitor, &mi) == 0)
				return {0, 0};
			return {static_cast<u16>(mi.rcMonitor.right - mi.rcMonitor.left),
					static_cast<u16>(mi.rcMonitor.bottom - mi.rcMonitor.top)};
		}

		void set_client_size(const extent<u16> new_size)
		{
			normalized_client_size = new_size;
			invalidated            = true;
			resize();
			invalidated = true;
		}

		u32 current_dpi() const { return GetDpiForWindow(handle); }

		f32 current_scale() const { return as<f32>(current_dpi()) / as<f32>(USER_DEFAULT_SCREEN_DPI); }

		extent<u16> normalize_client_size()
		{
			const f32 scale = current_scale();
			assert::check(scale >= 1.0f);

			const extent<u16> new_physical = get_clientsize();
			const extent<u16> new_normalized{as<u16>(new_physical.width / scale), as<u16>(new_physical.height / scale)};

			// if client changes, invalidate
			if (new_physical.width != physical_client_size.width or new_physical.height != physical_client_size.height
				or new_normalized.width != normalized_client_size.width
				or new_normalized.height != normalized_client_size.height)
			{
				physical_client_size   = new_physical;
				normalized_client_size = new_normalized;
				invalidated            = true;

				dbg::println(
				  "window: resized physical: {}x{}, normalized: {}x{}",
				  physical_client_size.width,
				  physical_client_size.height,
				  normalized_client_size.width,
				  normalized_client_size.height);
			}

			return normalized_client_size;
		}

		extent<u16> adjust_to_current_dpi(extent<u16> client_size)
		{
			const u32 dpi = current_dpi();

			RECT wr = {0, 0, static_cast<LONG>(client_size.width), static_cast<LONG>(client_size.height)};

			RECT wr = {0, 0, (LONG)(ext.width), (LONG)(ext.height)};

			if (IsWindows10OrGreater())
				AdjustWindowRectExForDpi(&wr, style, FALSE, ex_style, dpi);
			else
				AdjustWindowRectEx(&wr, style, FALSE, ex_style);


			return to_extent(wr);
		}

		void resize()
		{
			extent adjusted = adjust_to_current_dpi(normalized_client_size);

			invalidated = true;


			if (fullscreen)
			{
				const extent<u16> monitor_size = get_current_monitor_size();
			SetWindowPos(
			  handle,
				  HWND_TOP,
				  0,
				  0,
				  monitor_size.width,
				  monitor_size.height,
				  SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
				return;
			}

			extent adjusted = adjust_to_current_dpi(size);


			SetWindowPos(
			  handle,
			  nullptr,
			  0,
			  0,
			  adjusted.width,
			  adjusted.height,
			  SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		bool handle_messages()
		{
			assert::check(handle != nullptr);

			MSG msg{};
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					running = false;
					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if (handle == nullptr)
				{
					running = false;
					break;
			}
			}
			return running;
		}

	private:

		void update_keyboard_hook()
		{
			const bool should_block = active and (not win_key_allowed);

			if (should_block and not keyboard_hook)
			{
				keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL, low_level_keyboard_proc, GetModuleHandleW(nullptr), 0);
		}
			else if (not should_block and keyboard_hook)
		{
			if (keyboard_hook)
			{
				UnhookWindowsHookEx(keyboard_hook);
				keyboard_hook = nullptr;
			}
		}


	public:
		window() = default;

		~window() { deinitialize(); }

		window(const window&)            = delete;
		window& operator=(const window&) = delete;
		window(window&&)                 = delete;
		window& operator=(window&&)      = delete;

		// callbacks
		resize_callback on_resize_begin;
		resize_callback on_resize_end;
		size_callback   on_size;
		frame_callback  process_frame;

		initialize_callback on_initialize;
		destroy_callback    on_destroy;

		//
		void set_title(std::string_view title) { SetWindowTextA(handle, std::string{title}.c_str()); }

		void clear_invalidated() { invalidated = false; }

		void invalidate() { invalidated = true; }

		[[nodiscard]] bool is_minimized() const { return minimized; }

		[[nodiscard]] bool is_invalidated() const { return invalidated; }



		void allow_win_key(bool allowed)
		{
			win_key_allowed = allowed;
			update_keyboard_hook();
			}
			else
			{
				dbg::println("Uninstall hook");
				remove_hook();
			}
		}

		[[nodiscard]] HWND get_handle() const { return handle; }

		[[nodiscard]] HINSTANCE get_instance() const { return GetModuleHandle(nullptr); }

		[[nodiscard]] extent<u16> get_clientsize() const
		{
			RECT r{};
			GetClientRect(handle, &r);
			return to_extent(r);
		}

		void set_size(extent<u16> new_size) { set_client_size(new_size); }

		void set_resizable(bool is_resizable)
		{
			this->resizable = is_resizable;

			if (handle == nullptr)
			{
				if (is_resizable)
					style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
				else
					style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
				return;
			}

			if (fullscreen)
				return;

			DWORD live_style = GetWindowLong(handle, GWL_STYLE);
			if (is_resizable)
				live_style |= (WS_SIZEBOX | WS_MAXIMIZEBOX);
			else
				live_style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);

			SetWindowLongPtr(handle, GWL_STYLE, static_cast<LONG_PTR>(live_style));
			style = live_style; // keep cache in sync

			SetWindowPos(
			  handle, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}

		bool is_fullscreen() const { return fullscreen; }

		void set_fullscreen(bool make_fullscreen)
		{

			if (make_fullscreen == fullscreen)
				return;

			invalidated = true;

			fullscreen = make_fullscreen;

			const DWORD live_style = GetWindowLong(handle, GWL_STYLE);

			if (fullscreen)
			{
				MONITORINFO mi = {sizeof(mi)};
				if (GetWindowPlacement(handle, &wp)
					and GetMonitorInfo(MonitorFromWindow(handle, MONITOR_DEFAULTTONEAREST), &mi))
				{
					SetWindowLong(handle, GWL_STYLE, live_style & ~WS_OVERLAPPEDWINDOW);
					SetWindowPos(
					  handle,
					  HWND_TOP,
					  mi.rcMonitor.left,
					  mi.rcMonitor.top,
					  mi.rcMonitor.right - mi.rcMonitor.left,
					  mi.rcMonitor.bottom - mi.rcMonitor.top,
					  SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				}
			}
			else
			{
				SetWindowLong(handle, GWL_STYLE, live_style | WS_OVERLAPPEDWINDOW);
				SetWindowPos(
				  handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				SetWindowPlacement(handle, &wp);

				set_resizable(resizable); // WS_OVERLAPPEDWINDOW above reset THICKFRAME/MAXIMIZEBOX — reapply real state
			}
		}

		void toggle_fullscreen()
		{
			set_fullscreen(not is_fullscreen());
			invalidated = true;
		}

		bool is_running()
		{
			if (handle_messages() == false)
			{
				deinitialize();
				return false;


			return true;
		}

		void close() { running = false; }

		bool is_open() const { return handle != nullptr and running == true; }

		auto initialize(extent<u16> newsize, bool init_fullscreen, u8 monitor, std::string_view title)
		  -> std::expected<void, std::string>
		{
			if (handle != nullptr)
				return std::unexpected("window: already initialized");

			SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

			if (monitor >= monitor_count())
				monitor = primary_monitor_index();

			dbg::println("window: initializing on monitor {} of {}", monitor, monitor_count() - 1);

			get_monitors();

			size                   = newsize;
			normalized_client_size = newsize;

			WNDCLASSEX wc{};
			wc.cbSize        = sizeof(WNDCLASSEX);
			wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
			wc.lpszClassName = window_class_name.data();
			wc.hInstance     = GetModuleHandle(nullptr);

			wc.lpfnWndProc = [](HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				// https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992
				window* self{nullptr};
				if (message == WM_CREATE)
				{
					LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
					self                = static_cast<window*>(lpcs->lpCreateParams);
					self->handle        = hWnd;
					SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
				}
				else
				{
					self = reinterpret_cast<window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
				}

				if (self)
				{
					return self->wnd_proc(hWnd, message, wParam, lParam);
				}

				return DefWindowProc(hWnd, message, wParam, lParam);
			};

			//
			if (RegisterClassEx(&wc) == 0 && platform::get_error() != ERROR_CLASS_ALREADY_EXISTS)
			{
				deinitialize();
				return std::unexpected("window: failed to register window class");
			}

			set_resizable(resizable);


			const extent<u16> initial_outer = adjust_to_current_dpi(normalized_client_size);

			//
			handle = CreateWindowEx(
			  ex_style,
			  window_class_name.data(),
			  platform::string_to_wide(title).c_str(),
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  initial_outer.width,
			  initial_outer.height,
			  nullptr,
			  nullptr,
			  wc.hInstance,
			  this);

			if (not handle)
			{
				deinitialize();
				return std::unexpected(std::format("window: failed to create window: {}", platform::get_error_string()));
			}

			ivec2 pos = get_monitor_center_for_window(monitor);
			SetWindowPos(handle, nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

			set_client_size(size);
			set_fullscreen(init_fullscreen);
			resize();

			running     = true;
			invalidated = true;

			ShowWindow(handle, SW_SHOW);
			SetForegroundWindow(handle);


			set_client_size(size);
			set_fullscreen(init_fullscreen);


			if (auto result = invoke_if(on_initialize); not result)
			{
				if (result.error() == invoke_error::conditional_failed)
				{
					deinitialize();
					return std::unexpected("window: failed to initialize");
			}
			}
			else if (*result == false)
				{
					deinitialize();
					return std::unexpected("window: failed to initialize");
				}
			}

			//

			return {};
		}

		void deinitialize()
		{
			if (handle == nullptr)
				return;

			allow_win_key(false);

			if (is_fullscreen())
				toggle_fullscreen();

			resize();
			running = false;

			_ = invoke_if(on_destroy);

			HWND h = handle;
			handle = nullptr;
			DestroyWindow(h);
			UnregisterClass(window_class_name.data(), GetModuleHandle(nullptr));
		}

		void destroy() { deinitialize(); }

		// input

		void set_input(inputs* input) { m_inputs = input; }
	};

	LRESULT CALLBACK window::wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			case WM_CREATE:
			{
				return 0;
			}

			//
			case WM_ERASEBKGND: return 1;

			case WM_PAINT:
			{
				ValidateRect(handle, nullptr);

				if (running and sizing and not minimized)
				{
					process_frame();
				}
				return 0;
			}

			case WM_ACTIVATEAPP:
			{
				active = wParam != 0;

				update_keyboard_hook();
				return 0;
			}

			case WM_SETCURSOR:
			{
				if (not show_cursor and LOWORD(lParam) == HTCLIENT)
				{
					SetCursor(nullptr);
					return TRUE;
				}
				break;
			}

			case WM_ACTIVATE:
			{
				// const bool focused   = LOWORD(wParam) != WA_INACTIVE;
				// const bool iconified = HIWORD(wParam) ? true : false;


				return 0;
			}


			case WM_DISPLAYCHANGE:
			{
				// DEVMODE devmode;
				// ZeroMemory(&devmode, sizeof(devmode));
				// devmode.dmSize = sizeof(DEVMODE);
				// EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devmode);
				//
				// int orientation = (int)(90 * devmode.dmDisplayOrientation);

				return 0;
			}

			case WM_GETMINMAXINFO:
			{

				const extent<u16> min_outer = adjust_to_current_dpi(min_size);

				auto* mmi             = reinterpret_cast<MINMAXINFO*>(lParam);
				mmi->ptMinTrackSize.x = min_outer.width;
				mmi->ptMinTrackSize.y = min_outer.height;

				return 0;
			}

			case WM_DPICHANGED:
			{

				const auto* new_rect = reinterpret_cast<const RECT*>(lParam);

				SetWindowPos(
					  handle,
					  nullptr,
					  new_rect->left,
					  new_rect->top,
					  new_rect->right - new_rect->left,
					  new_rect->bottom - new_rect->top,
				  SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

				invalidated = true;
				normalize_client_size();
				return 0;
			}

			case WM_ENTERSIZEMOVE:
			{
				sizing = true;

				if (on_resize_begin)
					on_resize_begin();

				SetTimer(handle, RESIZE_TIMER_ID, USER_TIMER_MINIMUM, nullptr);
				return 0;
			}

			case WM_EXITSIZEMOVE:
			{
				KillTimer(handle, RESIZE_TIMER_ID);
				sizing = false;

				const extent<u16> current      = get_clientsize();
				const bool        size_changed = current != physical_client_size;

				if (running and not minimized)
					normalize_client_size();


				_ = invoke_if(size_changed, on_resize_end);


				return 0;
			}

			case WM_TIMER:
			{
				if (wParam == RESIZE_TIMER_ID)
				{
					InvalidateRect(handle, nullptr, FALSE);

					_ = invoke_if(on_size, get_clientsize());
				}
				return 0;
			}


			case WM_SIZE:
			{
				if (wParam == SIZE_MINIMIZED)
					minimized = true;

				if (wParam == SIZE_RESTORED or wParam == SIZE_MAXIMIZED)
				{
					minimized = false;

					size.width  = LOWORD(lParam);
					size.height = HIWORD(lParam);

					if (running and not sizing)
						normalize_client_size();
				}

				if (on_size)
					on_size(get_clientsize());

				_ = invoke_if(not sizing, on_size, get_clientsize());


				return 0;
			}


			case WM_CHAR:
			{
				char32_t ch = static_cast<char32_t>(wParam);

				if (m_inputs)
					m_inputs->character_input(ch);

				return 0;
			};

				// Applications running on Windows Vista and Windows Server 2008 should adhere to these guidelines
				// to ensure that the Restart Manager can shut down and restart applications if necessary to install
				// updates.
				// https://docs.microsoft.com/en-us/windows/win32/rstmgr/guidelines-for-applications

			case WM_DESTROY:
			{
				SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
				running = false;
				PostQuitMessage(0);
				return 0;
			}
			case WM_QUERYENDSESSION:
			{
				// User logging off
				running = false;
				// Save states here


				return TRUE;
			}

			case WM_ENDSESSION:
			{
				// User logged off
				running = false;
				return 0;
			}

			case WM_CLOSE:
			{
				// Save states here
				running = false;
				// Save states here

				if (on_destroy)
					on_destroy();
				return 0;
			}
			case WM_DESTROY:
			{
				SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
				running = false;
				PostQuitMessage(0);
			
				return 0;
			}
		}

		return DefWindowProc(handle, uMsg, wParam, lParam);
	}


} // namespace deckard::app
