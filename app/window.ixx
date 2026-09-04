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

	constexpr std::wstring_view window_class_name = L"DeckardWindowClass";

	export class window
	{
	private:
		HWND handle{nullptr};

		DWORD style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_SIZEBOX};
		DWORD ex_style{0};

		LRESULT CALLBACK wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);

		extent<u16> size{1920, 1080};
		extent<u16> min_size{1280, 720};
		extent<u16> normalized_client_size{0, 0};
		extent<u16> physical_client_size{0, 0};

		bool is_running{false};
		bool is_sizing{false};
		bool is_minimized{false};
		bool show_cursor{true};
		bool invalidated{false};



	private:
		extent<u16> get_clientsize() const
		{
			RECT r{};
			GetClientRect(handle, &r);
			return to_extent(r);
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
			size = adjust_to_current_dpi(new_size);
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

		extent<u16> adjust_to_current_dpi(extent<u16> old)
		{
			const u32 dpi   = current_dpi();
			const f32 scale = as<f32>(dpi) / USER_DEFAULT_SCREEN_DPI;

			extent<u16> ext;
			ext.width  = as<u16>(old.width * scale);
			ext.height = as<u16>(old.height * scale);

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

			SetWindowPos(
			  handle,
			  nullptr,
			  0,
			  0,
			  adjusted.width,
			  adjusted.height,
			  SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		bool handle_messages() const
		{
			assert::check(handle != nullptr);
			MSG msg{};
			while (PeekMessage(&msg, handle, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			return is_running;
		}

		bool is_fullscreen() const
		{
			DWORD dwStyle = GetWindowLong(handle, GWL_STYLE);
			return (dwStyle & WS_OVERLAPPEDWINDOW) == 0;
		}

	public:
		window() = default;

		~window() { deinitialize(); }

		window(const window&)            = delete;
		window& operator=(const window&) = delete;
		window(window&&)                 = delete;
		window& operator=(window&&)      = delete;

		[[nodiscard]] bool is_invalidated() const { return invalidated; }

		void set_title(std::string_view title) { SetWindowTextA(handle, title.data()); }

		void clear_invalidated() { invalidated = false; }

		void invalidate() { invalidated = true; }

		[[nodiscard]] HWND get_handle() const { return handle; }

		void toggle_fullscreen()
		{
			static WINDOWPLACEMENT wp{};

			// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

			DWORD dwStyle = GetWindowLong(handle, GWL_STYLE);
			if (dwStyle & WS_OVERLAPPEDWINDOW)
			{
				MONITORINFO mi = {sizeof(mi)};
				if (GetWindowPlacement(handle, &wp)
					&& GetMonitorInfo(MonitorFromWindow(handle, MONITOR_DEFAULTTOPRIMARY), &mi))
				{
					const DWORD old_style = dwStyle & ~WS_OVERLAPPEDWINDOW;
					SetWindowLong(handle, GWL_STYLE, old_style);
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

				const DWORD old_style = dwStyle | WS_OVERLAPPEDWINDOW;
				SetWindowLong(handle, GWL_STYLE, old_style);
				SetWindowPos(
				  handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				SetWindowPlacement(handle, &wp);
			}

			invalidated = true;
		}

		bool running()
		{
			if (handle_messages() == false)
			{
				deinitialize();
				return false;
			}

			return true;
		}

		void close() { is_running = false; }

		bool is_open() const { return handle != nullptr and is_running == true; }

		auto initialize(u16 width, u16 height, bool fullscreen, std::string_view title) -> std::expected<void, std::string>
		{
			if (handle != nullptr)
				return std::unexpected("window: already initialized");

			SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);


			size.width  = width;
			size.height = height;

			normalized_client_size = {width, height};

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

			//
			handle = CreateWindowEx(
			  ex_style,
			  window_class_name.data(),
			  platform::string_to_wide(title).c_str(),
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  normalized_client_size.width,
			  normalized_client_size.height,
			  nullptr,
			  nullptr,
			  wc.hInstance,
			  this);
			if (!handle)
			{
				deinitialize();
				return std::unexpected(std::format("window: failed to create window: {}", platform::get_error_string()));
			}

			set_client_size({1920, 1080});

			resize();

			is_running  = true;
			invalidated = true;

			ShowWindow(handle, SW_SHOW);

			return {};
		}

		void deinitialize()
		{

			if (is_fullscreen())
				toggle_fullscreen();

			resize();
			is_running = false;

			DestroyWindow(handle);
			handle = nullptr;
			UnregisterClass(window_class_name.data(), GetModuleHandle(0));
		}
	};

	LRESULT CALLBACK window::wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
			case WM_CREATE:
			{
				return 0;
			}

			case WM_ERASEBKGND:
			{
				// Prevent flickering by not erasing the background
				return 1;
			}

			//case WM_PAINT:
			//{
			//	ValidateRect(handle, nullptr);
			//	return 0;
			//}

			case WM_ACTIVATEAPP:
			{

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

				dbg::println("WM_DPICHANGED: new dpi: {}, new rect: {}x{}",
							 HIWORD(wParam),
							 new_rect->right - new_rect->left,
							 new_rect->bottom - new_rect->top);

				if (not SetWindowPos(
					  handle,
					  nullptr,
					  new_rect->left,
					  new_rect->top,
					  new_rect->right - new_rect->left,
					  new_rect->bottom - new_rect->top,
					  SWP_NOZORDER | SWP_NOACTIVATE))
				{
					return 1;
				}

				invalidated = true;
				normalize_client_size();
				return 0;
			}

			case WM_ENTERSIZEMOVE:
			{
				is_sizing = true;
				return 0;
			}

			case WM_EXITSIZEMOVE:
			{
				is_sizing = false;
				if (is_running and not is_minimized)
					normalize_client_size();
				return 0;
			}


			case WM_SIZE:
			{
				if (wParam == SIZE_MINIMIZED)
					is_minimized = true;
				if (wParam == SIZE_RESTORED or wParam == SIZE_MAXIMIZED)
				{
					is_minimized = false;

					size.width  = LOWORD(lParam);
					size.height = HIWORD(lParam);

					if (is_running and not is_sizing)
						normalize_client_size();
				}
				return 0;
			}

				// Applications running on Windows Vista and Windows Server 2008 should adhere to these guidelines
				// to ensure that the Restart Manager can shut down and restart applications if necessary to install
				// updates.
				// https://docs.microsoft.com/en-us/windows/win32/rstmgr/guidelines-for-applications

			case WM_DESTROY:
			{
				SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
				is_running = false;
				PostQuitMessage(0);
				return 0;
			}
			case WM_QUERYENDSESSION:
			{
				// User logging off
				is_running = false;
				// Save states here


				return TRUE;
			}

			case WM_ENDSESSION:
			case WM_CLOSE:
			{
				is_running = false;
				// Save states here

				return 0;
			}


			case WM_QUIT:
			{
				is_running = false;
				break;
			}
		}

		return DefWindowProc(handle, uMsg, wParam, lParam);
	}


} // namespace deckard::app
