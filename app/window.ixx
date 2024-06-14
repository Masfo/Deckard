module;
#include <Windows.h>
#include <dbt.h>
#include <shellscalingapi.h>
#include <versionhelpers.h>
#include <windowsx.h>


export module deckard.app:window;

import std;
import deckard.debug;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.win32;

namespace deckard::app
{
	using namespace deckard;

	struct WindowSize
	{
		u32 width{1'920};
		u32 height{1'080};
	};

	export class window
	{
	public:
		window() = default;

		virtual ~window() noexcept { destroy(); }

		window(window&&)            = delete;
		window& operator=(window&&) = delete;

		window(const window&)            = delete;
		window& operator=(const window&) = delete;

		void destroy() noexcept { }

		void create() noexcept { create(1'280, 720); }

		void create(u32 width, u32 height) noexcept
		{
			if (IsWindows8Point1OrGreater())
			{
				SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
			}

			if (IsWindows10OrGreater())
			{
				SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
				SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
				SetThreadDpiHostingBehavior(DPI_HOSTING_BEHAVIOR_DEFAULT);
			}

			if (handle != nullptr)
				return;

			window_size.width  = width;
			window_size.height = height;

			WNDCLASSEX wc{};
			wc.cbSize        = sizeof(WNDCLASSEX);
			wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
			wc.lpszClassName = L"DeckardWindowClass";
			wc.hInstance     = GetModuleHandle(nullptr);
			wc.hbrBackground = (HBRUSH)30;

			wc.lpfnWndProc = [](HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				// https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992
				window* self{nullptr};
				if (message == WM_NCCREATE)
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
					return self->WndProc(hWnd, message, wParam, lParam);
				}

				return DefWindowProc(hWnd, message, wParam, lParam);
			};

			if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			{
				destroy();
				return;
			}


			handle = CreateWindowEx(
			  ex_style,
			  wc.lpszClassName,
			  L"",
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  window_size.width,
			  window_size.height,
			  nullptr,
			  nullptr,
			  wc.hInstance,
			  this);
			if (!handle)
			{
				destroy();
				return;
			}


			// SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));


			adjust_window_size();


			// Init renderer

			ShowWindow(handle, SW_SHOW);

			// OnInit


			is_running = true;
		}

		void handle_messages() noexcept
		{
			MSG msg{};

			while (PeekMessage(&msg, handle, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		bool loop() noexcept
		{
			handle_messages();


			return is_running;
		}

	private:
		void adjust_window_size()
		{
			assert::check(handle != nullptr);

			//
			DEVMODEA dev{};
			dev.dmSize = sizeof(DEVMODE);
			EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &dev);
			RECT monitorRect{0, 0, (LONG)(dev.dmPelsWidth), (LONG)(dev.dmPelsHeight)};

			// If same as desktop resolution, switch to windowed fullscreen
			bool adjust = window_size.width >= (u32)dev.dmPelsWidth && window_size.height >= (u32)dev.dmPelsHeight;

			RECT wr{0l, 0l, (LONG)(window_size.width), (LONG)(window_size.height)};

			client_size.width  = window_size.width;
			client_size.height = window_size.height;
			dbg::println("before adjust: {}x{}", client_size.width, client_size.height);


			u32   dpi = USER_DEFAULT_SCREEN_DPI;
			float scale{static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI};

			if (windowed == true)
			{
				if (IsWindows10OrGreater())
				{
					dpi   = handle ? GetDpiForWindow(handle) : USER_DEFAULT_SCREEN_DPI;
					scale = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;

					f32 fWidth  = as<f32>(window_size.width * scale);
					f32 fHeight = as<f32>(window_size.height * scale);

					u32 scaledWidth  = as<u32>(std::min((f32)dev.dmPelsWidth, fWidth));
					u32 scaledHeight = as<u32>(std::min((f32)dev.dmPelsHeight, fHeight));

					client_size.width  = scaledWidth;
					client_size.height = scaledHeight;

					wr = {0, 0, (LONG)(scaledWidth), (LONG)(scaledHeight)};

					if (adjust)
						AdjustWindowRectExForDpi(&wr, style, FALSE, ex_style, dpi);
				}
				else
				{
					if (adjust)
						AdjustWindowRectEx(&wr, style, FALSE, ex_style);
				}
			}

			i32 adjustedWidth{wr.right - wr.left};
			i32 adjustedHeight{wr.bottom - wr.top};
			dbg::println("Adjusted size: {}x{}", adjustedWidth, adjustedHeight);


			SetWindowPos(
			  handle, nullptr, 0, 0, adjustedWidth, adjustedHeight, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
		{

			switch (uMsg)
			{
				case WM_CREATE:
				{
					return 0;
				}

				case WM_DEVICECHANGE:
				{
					auto                           lpDev = reinterpret_cast<PDEV_BROADCAST_HDR>(lParam);
					PDEV_BROADCAST_DEVICEINTERFACE dev   = nullptr;

					WCHAR tPayloadPath[MAX_PATH] = {0};
					if (!lpDev)
						break;
					if (lpDev->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
						dev = (PDEV_BROADCAST_DEVICEINTERFACE)lpDev;

					switch (wParam)
					{
							//                     case DBT_DEVNODES_CHANGED:
							//                     {
							//                         Sleep(10);
							//                         break;
							//                     }
						case DBT_DEVICEARRIVAL:
						{

							break;
						}
					}
					break;
				}


				case WM_ACTIVATEAPP:
				{

					return 0;
				}

				case WM_ACTIVATE:
				{
					const bool focused   = LOWORD(wParam) != WA_INACTIVE;
					const bool iconified = HIWORD(wParam) ? true : false;

					dbg::trace("WM_ACTIVATE: {} focused = {}, iconified = {}",
							   "DeckardApp",
							   focused ? "true" : "false",
							   iconified ? "true" : "false");

					return 0;
				}

				case WM_MOUSEACTIVATE:
				{
					auto HitTest = static_cast<int>(lParam);
					dbg::trace("WM_MOUSEACTIVATE: {}", HitTest);
					return MA_ACTIVATEANDEAT;
				}

				case WM_MOUSEHOVER:
				{
					int xPos = GET_X_LPARAM(lParam);
					int yPos = GET_Y_LPARAM(lParam);

					dbg::trace("WM_MOUSEHOVER: [{}x{}] | VK: {:#X}", xPos, yPos, (uint32_t)wParam);
					return 0;
				}

				case WM_SHOWWINDOW:
				{
					dbg::trace("WM_SHOWWINDOW: {}, {}", "DeckardApp", wParam ? "true" : "false");
					return 0;
				}

				case WM_SETTINGCHANGE:
				{

					return 0;
				}

				case WM_DISPLAYCHANGE:
				{
					DEVMODE devmode;
					ZeroMemory(&devmode, sizeof(devmode));
					devmode.dmSize = sizeof(DEVMODE);
					EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devmode);

					int orientation = (int)(90 * devmode.dmDisplayOrientation);


					dbg::trace(
					  "Display change: {}° {}x{}, {} BPP", orientation, devmode.dmPelsWidth, devmode.dmPelsHeight, devmode.dmBitsPerPel);

					return 0;
				}

				case WM_DPICHANGED:
				{
					u32    dpi      = HIWORD(wParam);
					LPRECT new_rect = reinterpret_cast<LPRECT>(lParam);


					f32 scale = as<f32>(dpi) / USER_DEFAULT_SCREEN_DPI;

					f32 fWidth  = as<f32>(window_size.width * scale);
					f32 fHeight = as<f32>(window_size.height * scale);

					RECT r{0, 0, (LONG)(fWidth), (LONG)(fHeight)};
					if (!AdjustWindowRectExForDpi(&r, style, false, 0, dpi))
					{
						return 1;
					}

					if (!SetWindowPos(hWnd, NULL, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE))
						return 1;

					dbg::println("New DPI {}, {}x{}", dpi, r.right - r.left, r.bottom - r.top);

					GetClientRect(hWnd, new_rect);

					i32 adjustedWidth{new_rect->right - new_rect->left};
					i32 adjustedHeight{new_rect->bottom - new_rect->top};
					dbg::println("Adjusted client rect: {}x{}", adjustedWidth, adjustedHeight);

					return 0;
				}


				case WM_SIZE:
				{
					auto client_width  = (u32)LOWORD(lParam);
					auto client_height = (u32)HIWORD(lParam);


					if (windowed)
					{

						RECT windowRect{};
						GetWindowRect(hWnd, &windowRect);

						RECT inverseRect{0};
						// DPI HERE?
						if (AdjustWindowRectEx(&inverseRect, style, false, ex_style) == 1)
						{
							windowRect.left -= inverseRect.left;
							windowRect.top -= inverseRect.top;
							windowRect.right -= inverseRect.right;
							windowRect.bottom -= inverseRect.bottom;
						}
					}
					return 0;
				}

					// Applications running on Windows Vista and Windows Server 2008 should adhere to these guidelines
					// to ensure that the Restart Manager can shut down and restart applications if necessary to install
					// updates.
					// https://docs.microsoft.com/en-us/windows/win32/rstmgr/guidelines-for-applications

				case WM_QUERYENDSESSION:
				{
					is_running = false;
					// Save states here

					return 1;
				}

				case WM_ENDSESSION:
				case WM_CLOSE:
				{
					dbg::println("WM_CLOSE");


					// Save states here
					break;
				}

				case WM_DESTROY:
				{
					dbg::println("WM_DESTROY");

					// if (m_DeviceNotify)
					//	UnregisterDeviceNotification(m_DeviceNotify);


					SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
					is_running = false;
					return 0;
				}
				case WM_QUIT:
				{
					dbg::println("WM_QUIT");
					is_running = false;
					break;
				}


				case WM_SETFOCUS:
				{
					dbg::trace("Focus: {} true", "DeckardApp");

					return 0;
				}

				case WM_KILLFOCUS:
				{
					dbg::trace("Focus: {} false", "DeckardApp");
					return 0;
				}

#if 1
					// Mouse
				case WM_MOUSEMOVE:
				{
					// m_mouseVK = static_cast<uint32_t>(wParam);

					int x = GET_X_LPARAM(lParam);
					int y = GET_Y_LPARAM(lParam);

					// m_mouseWindowCoord.x = x;
					// m_mouseWindowCoord.y = y;

					return 0;
				}


				case WM_SYSCHAR:
				{
					auto vk = static_cast<int>(wParam);

					const int scancode = (lParam >> 16) & 0xff;

					dbg::trace("SysChar {}: {} - scancode {}", "DeckardApp>", vk, scancode);


					return 0;
				}

				// Sys key
				case WM_SYSKEYDOWN:
				{
					auto vk = static_cast<int>(wParam);

					const int scancode = (lParam >> 16) & 0xff;


					dbg::trace("SysKeyDown {}: {} - scancode {}", "DeckardApp", vk, scancode);
					return 0;
				}

				case WM_SYSKEYUP:
				{
					auto vk = static_cast<int>(wParam);

					const int scancode = (lParam >> 16) & 0xff;


					dbg::trace("SysKeyUp {}: {} - scancode {}", "DeckardApp", vk, scancode);

					return 0;
				}


				// Key
				case WM_KEYDOWN:
				{
					auto vk = static_cast<int>(wParam);


					dbg::trace("KeyDown {}: {}", "DeckardApp", vk);

					return 0;
				}


				case WM_KEYUP:
				{
					auto vk = static_cast<int>(wParam);


					dbg::trace("KeyUp {}: {}", "DeckardApp", vk);

					if (vk == VK_ESCAPE)
					{
						is_running = false;
					}

					return 0;
				}
#endif
				case WM_MOUSEWHEEL:
				{
#if 1
					auto xPos   = GET_X_LPARAM(lParam);
					auto yPos   = GET_Y_LPARAM(lParam);
					auto fwKeys = GET_KEYSTATE_WPARAM(wParam);

					int delta = GET_WHEEL_DELTA_WPARAM(wParam);
					if (delta > 0)
						dbg::trace("[Mouse wheel] up {}", delta);
					else
						dbg::trace("[Mouse wheel] down {}", delta);

					dbg::trace("[Mouse wheel] keys {}, xpos {}, ypos {}", fwKeys, xPos, yPos);
#endif
					return 0;
				}

				case WM_INPUTLANGCHANGE:
				{
					auto  hkl = (HKL)lParam;
					WCHAR localeName[LOCALE_NAME_MAX_LENGTH]{0};
					LCIDToLocaleName(MAKELCID(LOWORD(hkl), SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0);

					WCHAR lang[36]{0};
					GetLocaleInfoEx(localeName, LOCALE_SISO639LANGNAME2, lang, sizeof(lang));


					dbg::trace("Input language changed to '{}'", system::from_wide(localeName));

					return 1;
				}

				case WM_INPUT:
				{
					const HRAWINPUT raw{reinterpret_cast<HRAWINPUT>(lParam)};

					// bool foreground = GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT;
					//  if (foreground)
					//	HandleRawInput(raw);

					DefWindowProc(hWnd, uMsg, wParam, lParam);
					return 0;
				}

				case WM_INPUT_DEVICE_CHANGE:
				{
					auto newDevice = reinterpret_cast<HANDLE>(lParam);

					// HandleRawInputChange(newDevice, (int)GET_RAWINPUT_CODE_WPARAM(wParam));
					return 0;
				}

#if 1
				case WM_CHAR:
				{
					dbg::trace("[{}] WM_CHAR:", "DeckardApp");

					auto vk = static_cast<u16>(wParam);

					if (vk == VK_RETURN)
					{
						dbg::trace("\t{} - Enter", vk);
					}
					else if (vk == VK_ESCAPE)
					{
						dbg::trace("\t{} - Escape", vk);
					}
					else if (vk == VK_SPACE)
					{
						dbg::trace("\t{} - Space", vk);
					}
					else if (vk == VK_BACK)
					{
						dbg::trace("\t{} - Back", vk);
					}
					else
					{
						dbg::trace("\t{0} - '{0}'", (char)vk);
					}

					return 0;
				}

				case WM_HOTKEY:
				{
					auto id   = wParam;
					auto key  = LOWORD(lParam);
					auto hkey = HIWORD(lParam);

					dbg::trace("Hotkey: {}, key = {}, hkey = {}", id, key, hkey);

					return 0;
				}

				case WM_APPCOMMAND:
				{
					auto cmd     = GET_APPCOMMAND_LPARAM(lParam);
					auto uDevice = GET_DEVICE_LPARAM(lParam);
					auto dwKeys  = GET_KEYSTATE_LPARAM(lParam);

					dbg::trace("WM_APPCOMMAND: cmd: {}, device: {}, keys: {}", cmd, uDevice, dwKeys);

					return 0;
				}

#endif
			}
			return DefWindowProc(handle, uMsg, wParam, lParam);
		}

		WindowSize      window_size;
		WindowSize      client_size;
		HWND            handle{nullptr};
		DWORD           style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
		DWORD           ex_style{WS_EX_APPWINDOW};
		WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
		bool            is_running{false};
		bool            windowed{true};
	};

} // namespace deckard::app
