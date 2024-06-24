module;
#include <Windows.h>
#include <dbt.h>
#include <dwmapi.h>
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
		f32 aspect{1.777f};
		u32 dpi{USER_DEFAULT_SCREEN_DPI};
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


			client_size.width  = width;
			client_size.height = height;
			client_size.aspect = (f32)width / (f32)height;
			client_size.dpi    = USER_DEFAULT_SCREEN_DPI;

			WNDCLASSEX wc{};
			wc.cbSize        = sizeof(WNDCLASSEX);
			wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
			wc.lpszClassName = L"DeckardWindowClass";
			wc.hInstance     = GetModuleHandle(nullptr);
			wc.hbrBackground = CreateSolidBrush(RGB(0, 128, 196));

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
					return self->WndProc(hWnd, message, wParam, lParam);
				}

				return DefWindowProc(hWnd, message, wParam, lParam);
			};

			if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			{
				destroy();
				return;
			}

			// style &= ~WS_SIZEBOX;

			handle = CreateWindowEx(
			  ex_style,
			  wc.lpszClassName,
			  L"Deckard Window",
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  client_size.width,
			  client_size.height,
			  nullptr,
			  nullptr,
			  wc.hInstance,
			  this);
			if (!handle)
			{
				destroy();
				return;
			}

			// Adjust to DPI scale
			RECT wr{0, 0, (LONG)client_size.width, (LONG)client_size.height};

			if (IsWindows10OrGreater())
			{
				u32 dpi         = handle ? GetDpiForWindow(handle) : USER_DEFAULT_SCREEN_DPI;
				f32 scale       = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
				client_size.dpi = dpi;

				f32 fWidth  = as<f32>(client_size.width * scale);
				f32 fHeight = as<f32>(client_size.height * scale);

				u32 scaledWidth  = as<u32>(fWidth);
				u32 scaledHeight = as<u32>(fHeight);


				wr = {0, 0, (LONG)(scaledWidth), (LONG)(scaledHeight)};
				// scale dpi
				AdjustWindowRectExForDpi(&wr, style, FALSE, ex_style, dpi);
				scaledWidth  = wr.right - wr.left;
				scaledHeight = wr.bottom - wr.top;

				wr = {0, 0, (LONG)(scaledWidth), (LONG)(scaledHeight)};
			}
			else
			{
				AdjustWindowRectEx(&wr, style, FALSE, ex_style);
			}

			i32 adjustedWidth{wr.right - wr.left};
			i32 adjustedHeight{wr.bottom - wr.top};

			SetWindowPos(
			  handle, nullptr, 0, 0, adjustedWidth, adjustedHeight, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);


			// Init renderer

			//
			ShowWindow(handle, SW_SHOW);
			RedrawWindow(handle, nullptr, nullptr, RDW_INTERNALPAINT);


			const auto [major, minor, build] = system::OSBuildInfo();

			if (IsWindows10OrGreater() and build >= 22'621)
			{
				const auto DWMSBT_DISABLE         = 1; // Default
				const auto DWMSBT_MAINWINDOW      = 2; // Mica
				const auto DWMSBT_TRANSIENTWINDOW = 3; // Acrylic
				const auto DWMSBT_TABBEDWINDOW    = 4; // Tabbed
				auto       DWMSBT_set             = DWMSBT_DISABLE;

				::DwmSetWindowAttribute(handle, DWMWA_SYSTEMBACKDROP_TYPE, &DWMSBT_set, sizeof(int));
			}

			if (IsWindows10OrGreater() and build >= 22'000)
			{
				// Darkmode
				BOOL value = TRUE;
				::DwmSetWindowAttribute(handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

				//
				// DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUNDSMALL;
				DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DONOTROUND;
				::DwmSetWindowAttribute(handle, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
			}
			// OnInit

			SetTimer(handle, 0, 16, 0);


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
		void toggle_fullscreen()
		{
			// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

			DWORD dwStyle = GetWindowLong(handle, GWL_STYLE);
			if (dwStyle & WS_OVERLAPPEDWINDOW)
			{
				MONITORINFO mi = {sizeof(mi)};
				if (GetWindowPlacement(handle, &wp) && GetMonitorInfo(MonitorFromWindow(handle, MONITOR_DEFAULTTOPRIMARY), &mi))
				{
					SetWindowLong(handle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
					SetWindowPos(
					  handle,
					  HWND_TOP,
					  mi.rcMonitor.left,
					  mi.rcMonitor.top,
					  mi.rcMonitor.right - mi.rcMonitor.left,
					  mi.rcMonitor.bottom - mi.rcMonitor.top,
					  SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				}
				windowed = false;
			}
			else
			{
				windowed = true;
				SetWindowLong(handle, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
				SetWindowPos(handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				SetWindowPlacement(handle, &wp);
			}

			dbg::println("toggle: {}x{}", client_size.width, client_size.height);
		}

		LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
		{

			switch (uMsg)
			{

				case WM_TIMER:
				{
					RECT wr{};
					GetClientRect(handle, &wr);
					SetWindowTextA(
					  handle,
					  std::format("{}x{}, {}x{}", client_size.width, client_size.height, wr.right - wr.left, wr.bottom - wr.top).c_str());
					//
					break;
				};

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

					// case WM_GETDPISCALEDSIZE:
					//{
					//	UINT  dpi         = (UINT)wParam;
					//	SIZE* scaled_size = (SIZE*)lParam;
					//	dbg::println("getdpiscaled: {}x{}", scaled_size->cx, scaled_size->cy);
					//
					//	RECT window_rect{};
					//	window_rect.right  = MulDiv(160 * 4, dpi, USER_DEFAULT_SCREEN_DPI);
					//	window_rect.bottom = MulDiv(144 * 4, dpi, USER_DEFAULT_SCREEN_DPI);
					//	// AdjustWindowRectExForDpi(&window_rect, style, false, ex_style, dpi);
					//
					//	scaled_size->cx = window_rect.right - window_rect.left;
					//	scaled_size->cy = window_rect.bottom - window_rect.top;
					//	return TRUE;
					// }


				case WM_DPICHANGED:
				{
#if 0
					RECT* const rect = (RECT*)lParam;
					// auto rect = *reinterpret_cast<RECT *>(lParam);
					SetWindowPos(
					  hWnd,
					  0, // or NULL
					  0,
					  0,
					  rect->right - rect->left,
					  rect->bottom - rect->top,
					  SWP_NOSIZE | SWP_NOMOVE);

					// const LPRECT prcNewWindow = (LPRECT)lParam;
					// SetWindowPos(handle, NULL, 0, 0, client_size.width, client_size.height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
#else
					int const dpi2 = ::GetDpiForWindow(hWnd);
					dbg::println("GetDpiForWindow: {}", dpi2);


					u32       dpi         = HIWORD(wParam);
					int const previousDpi = LOWORD(wParam);
					dbg::println("DPI {}, old {}", dpi, previousDpi);

					client_size.dpi = dpi;

					LPRECT new_rect = reinterpret_cast<LPRECT>(lParam);

#if 1
					u32 new_width  = client_size.width;
					u32 new_height = client_size.height;
#else
					u32 new_width  = new_rect->right - new_rect->left;
					u32 new_height = new_rect->bottom - new_rect->top;
#endif

					f32 scale = 1.0f;
					// as<f32>(dpi) / USER_DEFAULT_SCREEN_DPI;

					f32 fWidth  = as<f32>(new_width * scale);
					f32 fHeight = as<f32>(new_height * scale);

					RECT r{0, 0, (LONG)(fWidth), (LONG)(fHeight)};
					AdjustWindowRectExForDpi(&r, style, false, ex_style, dpi);

					i32 adjustedWidth{r.right - r.left};
					i32 adjustedHeight{r.bottom - r.top};


					SetWindowPos(hWnd, NULL, 0, 0, adjustedWidth, adjustedHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
#endif
					return 0;
				}

#if 0
				case WM_GETDPISCALEDSIZE:
				{
					break;
					UINT  dpi            = static_cast<UINT>(wParam);
					float scaling_factor = static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;

					RECT client_area;

					if (!GetClientRect(hWnd, &client_area))
					{
						// Error handling
						return 0;
					}

					client_area.right  = static_cast<LONG>(client_area.right * scaling_factor);
					client_area.bottom = static_cast<LONG>(client_area.bottom * scaling_factor);

					if (!AdjustWindowRectExForDpi(&client_area, style, false, ex_style, dpi))
					{
						// Error handling
						return 0;
					}

					SIZE* new_size = reinterpret_cast<SIZE*>(lParam);
					new_size->cx   = client_area.right - client_area.left;
					new_size->cy   = client_area.bottom - client_area.top;
					SetWindowPos(handle, NULL, 0, 0, new_size->cx, new_size->cy, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

					return 1;
				}
#endif


				case WM_SIZE:
				{
					auto client_width  = (u32)LOWORD(lParam);
					auto client_height = (u32)HIWORD(lParam);
					dbg::println("wm_size: {}x{}", client_width, client_height);
					client_size.width  = client_width;
					client_size.height = client_height;

					RECT client_area{};

					if (!GetClientRect(hWnd, &client_area))
					{
						// Error handling
						return 0;
					}
					client_size.width = client_area.right - client_area.left;
					;
					client_size.height = client_area.bottom - client_area.top;

					client_area.right  = static_cast<LONG>(client_area.right);
					client_area.bottom = static_cast<LONG>(client_area.bottom);

					// if (windowed)
					//{
					//	u32 dpi = handle ? GetDpiForWindow(handle) : USER_DEFAULT_SCREEN_DPI;
					//
					//
					//	RECT windowRect{};
					//	GetWindowRect(hWnd, &windowRect);
					//
					//	RECT inverseRect{0};
					//	// DPI HERE?
					//	if (AdjustWindowRectExForDpi(&inverseRect, style, false, ex_style, dpi) == 1)
					//	{
					//		windowRect.left -= inverseRect.left;
					//		windowRect.top -= inverseRect.top;
					//		windowRect.right -= inverseRect.right;
					//		windowRect.bottom -= inverseRect.bottom;
					//	}
					// }
					return 0;
				}

#if 0
				case WM_SIZING:
				{

					being_dragged = wParam;
					return 0;
				}

				case WM_WINDOWPOSCHANGING:
				{
					WINDOWPOS* winPos = (WINDOWPOS*)lParam;

					// Adjust window dimensions to maintain aspect ratio
					switch (being_dragged)
					{
						case WMSZ_BOTTOM:
						case WMSZ_TOPRIGHT: winPos->cx = (int)((double)winPos->cy * window_size.aspect); break;

						case WMSZ_RIGHT:
						case WMSZ_BOTTOMLEFT:
						case WMSZ_BOTTOMRIGHT: winPos->cy = (int)((double)winPos->cx / window_size.aspect); break;

						case WMSZ_TOP:
						{
							// Adjust the x position of the window to make it appear
							// that the bottom right side is anchored
							WINDOWPOS old = *winPos;

							winPos->cx = (int)((double)winPos->cy * window_size.aspect);

							winPos->x += old.cx - winPos->cx;
							;
						}
						break;

						case WMSZ_LEFT:
						case WMSZ_TOPLEFT:
						{
							// Adjust the y position of the window to make it appear
							// the bottom right side is anchored. TOPLEFT resizing
							// will move the window around if you don't do this
							WINDOWPOS old = *winPos;
							winPos->cy    = (int)((double)winPos->cx / window_size.aspect);

							winPos->y += old.cy - winPos->cy;
						}
						break;
					}
					return 0;
				}
#endif
					// Applications running on Windows Vista and Windows Server 2008 should adhere to these guidelines
					// to ensure that the Restart Manager can shut down and restart applications if necessary to install
					// updates.
					// https://docs.microsoft.com/en-us/windows/win32/rstmgr/guidelines-for-applications

				case WM_QUERYENDSESSION:
				{
					// User logging off
					is_running = false;
					// Save states here

					return 1;
				}

				case WM_ENDSESSION:
				case WM_CLOSE:
				{
					dbg::println("WM_CLOSE");
					is_running = false;

					// Save states here

					return 0;
				}

#if 1
				case WM_SYSCOMMAND:
				{
					u32 command = as<u32>(wParam);

					switch (command)
					{
						case SC_SCREENSAVE:
						{
							dbg::println("Screensaver");
							break;
						};
						case SC_MONITORPOWER:
						{
							i32 state = as<i32>(lParam);

							switch (state)
							{
								case -1: dbg::println("Monitor off"); break;
								case 0: dbg::println("Monitor lowpower"); break;
								case 1: dbg::println("Monitor shutdown"); break;
								default: break;
							}

							break;
						}
						default: break;
					}

					return DefWindowProc(handle, uMsg, wParam, lParam);
				}
#endif

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

					dbg::trace("SysChar {}: {} - scancode {}", "DeckardApp", vk, scancode);


					return 0;
				}

				// Sys key
				case WM_SYSKEYDOWN:
				{
					auto      vk       = static_cast<int>(wParam);
					const int scancode = (lParam >> 16) & 0xff;

					// alt-enter
					if (vk == VK_RETURN and (lParam & 0x6000'0000) == 0x2000'0000)
					{
						toggle_fullscreen();
					}

					return 0;
				}

				case WM_SYSKEYUP:
				{
					auto vk = static_cast<int>(wParam);

					const int scancode = (lParam >> 16) & 0xff;

					return 0;
				}


				// Key
				case WM_KEYDOWN:
				{
					auto vk = static_cast<int>(wParam);


					return 0;
				}


				case WM_KEYUP:
				{
					auto vk = static_cast<int>(wParam);


					if (vk == VK_ESCAPE)
					{
						is_running = false;
						break;
					}


					if (vk == VK_F1)
					{
						toggle_fullscreen();
						RECT r{};
						GetClientRect(handle, &r);
						client_size.width  = r.right - r.left;
						client_size.height = r.bottom - r.top;
						break;
					}

					if (vk == VK_F2)
					{
						client_size.width  = 1'920;
						client_size.height = 1'080;
						break;
					}
					if (vk == VK_F3)
					{
						client_size.width  = 1'280;
						client_size.height = 720;
						break;
					}

					if (vk == VK_F4)
					{
						break;
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

				case WM_POWERBROADCAST:
				{
					// Suspending: System is suspending operation.

					// Resume 1: Operation is resuming from a low-power state.
					// Resume 2: Operation is resuming automatically from a low-power state.
					switch (wParam)
					{
						case PBT_APMSUSPEND:
							dbg::println("System is suspending operation.");
							{
								break;
							}
						case PBT_APMRESUMESUSPEND: [[fallthrough]];
						case PBT_APMRESUMEAUTOMATIC:
						{
							dbg::println("Operation is resuming from a low-power state. ");
							break;
						}

						case PBT_APMPOWERSTATUSCHANGE: dbg::println("Power status changed"); break;
						default: break;
					}
					return 1;
				}
#endif
			}
			return DefWindowProc(handle, uMsg, wParam, lParam);
		}

		WindowSize      client_size;
		HWND            handle{nullptr};
		DWORD           style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
		DWORD           ex_style{WS_EX_APPWINDOW};
		WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
		bool            is_running{false};
		bool            windowed{true};
		int             being_dragged{false};
	};

} // namespace deckard::app
