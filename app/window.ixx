module;
#include <Windows.h>
#include <dbt.h>
#include <dwmapi.h>
#include <shellscalingapi.h>
#include <versionhelpers.h>
#include <windowsx.h>

#define GL_GLEXT_PROTOTYPES
#include <glcorearb.h>
#include <wglext.h>


export module deckard.app:window;


import std;
import deckard;
import deckard.debug;
import deckard.types;
import deckard.helpers;
import deckard.assert;
import deckard.as;
import deckard.win32;
import deckard.system;
import deckard.vulkan;
import deckard.math;

using namespace deckard::system;
using namespace std::string_view_literals;

namespace deckard::app
{
	using namespace deckard;

	enum Mode : u32
	{
		Windowed   = 0,
		Fullscreen = 1,
	};

	struct extent
	{
		u32 width{1'920};
		u32 height{1'080};
	};

	extent to_extent(RECT r) { return {as<u32>(r.right - r.left), as<u32>(r.bottom - r.top)}; }

	struct WindowConfig
	{
		extent extent{1'920, 1'080};
		f32    aspect{1.777f};

		f32 update_aspect() noexcept
		{
			aspect = as<f32>(extent.width / extent.height);
			return aspect;
		}
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

		void destroy() noexcept
		{
			//
			vulkan.deinitialize();

			is_running = false;
			if (not windowed)
			{
				toggle_fullscreen();
			}


			DestroyWindow(handle);
			UnregisterClass(L"DeckardWindowClass", GetModuleHandle(0));
			PostQuitMessage(0);
		}

		u32 current_dpi() const { return GetDpiForWindow(handle); }

		void set_title(std::string_view title) const noexcept { SetWindowTextA(handle, title.data()); }

		extent adjust_to_dpi(extent old, u32 dpi)
		{

			const f32 scale = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;

			extent ext;
			ext.width  = math::round_to_even<u32>(old.width * scale);
			ext.height = math::round_to_even<u32>(old.height * scale);

			RECT wr = {0, 0, (LONG)(ext.width), (LONG)(ext.height)};

			if (IsWindows10OrGreater())
				AdjustWindowRectExForDpi(&wr, style, FALSE, ex_style, dpi);
			else
				AdjustWindowRectEx(&wr, style, FALSE, ex_style);


			return to_extent(wr);
		}

		void resize()
		{
			extent adjusted = adjust_to_dpi(config[current_mode].extent, current_dpi());


			SetWindowPos(
			  handle, nullptr, 0, 0, adjusted.width, adjusted.height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		void create() noexcept { create(1'280 - 1, 720 - 1); }

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

			config[Mode::Windowed].extent.width  = width;
			config[Mode::Windowed].extent.height = height;

			config[Mode::Fullscreen].extent.width  = width;
			config[Mode::Fullscreen].extent.height = height;


			WNDCLASSEX wc{};
			wc.cbSize        = sizeof(WNDCLASSEX);
			wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
			wc.lpszClassName = L"DeckardWindowClass";
			wc.hInstance     = GetModuleHandle(nullptr);
			// wc.hbrBackground = CreateSolidBrush(RGB(0, 128, 196));

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

#if 1
			if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			{
				dbg::println("RegisterClassEx failed: {}", get_windows_error());
				destroy();
				destroy();
				return;
			}
#endif

			// style &= ~WS_SIZEBOX;

			handle = CreateWindowEx(
			  ex_style,
			  L"DeckardWindowClass",
			  L"Deckard Window",
			  style,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  config[Mode::Windowed].extent.width,
			  config[Mode::Windowed].extent.height,
			  nullptr,
			  nullptr,
			  wc.hInstance,
			  this);
			if (!handle)
			{
				dbg::println("CreateWindowEx failed: {}", get_windows_error());
				destroy();
				return;
			}


			///


			const auto [major, minor, build] = system::OSBuildInfo();

			if (IsWindows10OrGreater() and build >= 22'621)
			{
				constexpr auto DWMSBT_DISABLE = 1; // Default
#if 0
				constexpr auto DWMSBT_MAINWINDOW      = 2; // Mica
				constexpr auto DWMSBT_TRANSIENTWINDOW = 3; // Acrylic
				constexpr auto DWMSBT_TABBEDWINDOW    = 4; // Tabbed
#endif
				auto DWMSBT_set = DWMSBT_DISABLE;

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

			resize();


			SetTimer(handle, 0, 16, 0);


			if (not vulkan.initialize(handle))
				return;

			// if (not vulkan.initialize(GetModuleHandle(nullptr), handle))
			//	return;


			ShowWindow(handle, SW_SHOW);
			RedrawWindow(handle, nullptr, nullptr, RDW_INTERNALPAINT);

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

			render();

			return is_running;
		}

		void render() noexcept
		{ //

			vulkan.draw();
		}

		HWND get_handle() const noexcept { return handle; };

		bool running() const noexcept { return is_running; };

		void handle_input(const HRAWINPUT input) noexcept
		{
			uint32_t size = 0;
			if (GetRawInputData(input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0)
				return;

			RAWINPUT* rawInput = nullptr;

			if (size > 0)
			{

				rawinput_buffer.resize(as<size_t>(size));


				if (GetRawInputData(input, RID_INPUT, &rawinput_buffer[0], &size, sizeof(RAWINPUTHEADER)) != size)
					return;

				rawInput = reinterpret_cast<RAWINPUT*>(rawinput_buffer.data());


				handle_keyboard(rawInput->data.keyboard);
				handle_mouse(rawInput->data.mouse);
			}
			DefRawInputProc(&rawInput, 2, sizeof(RAWINPUTHEADER));
		}

		void handle_mouse(const RAWMOUSE& rawmouse) noexcept
		{


			u16 flags       = rawmouse.usFlags;
			u16 buttonflags = rawmouse.usButtonFlags;
			u16 buttonData  = rawmouse.usButtonData;
			u32 extra       = rawmouse.ulExtraInformation;
			u32 rawButtons  = rawmouse.ulRawButtons;
			u32 buttons     = rawmouse.ulButtons;

			float scrollDelta{};
			bool  isScrollPage = false;
			if ((rawmouse.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL ||
				(rawmouse.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL)
			{
				auto  delta      = (float)(short)rawmouse.usButtonData;
				float numTicks   = delta / WHEEL_DELTA;
				bool  horizontal = (rawmouse.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL;

				scrollDelta = numTicks;

				if (horizontal)
				{
					unsigned long scrollChars = 1;
					SystemParametersInfoA(SPI_GETWHEELSCROLLCHARS, 0, &scrollChars, 0);
					scrollDelta *= static_cast<float>(scrollChars);
				}
				else
				{
					unsigned long scrollLines = 3;

					SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
					if (scrollLines == WHEEL_PAGESCROLL)
						isScrollPage = true;
					else
						scrollDelta *= static_cast<float>(scrollLines);
				}
			}


			// dbg::println(
			//   "{:4}{:4} - [MOUSE] flags: {:016b}, buttons flags: {:016b}, button data: {:016b}, extra: {:8x}, rawButtons: {:032b}, "
			//   "buttons: {:032b}",
			//   scrollDelta,
			//   isScrollPage,
			//   flags,
			//   buttonflags,
			//   buttonData,
			//   extra,
			//   rawButtons,
			//   buttons);
		}

		void handle_keyboard(const RAWKEYBOARD& kb) noexcept
		{
			unsigned short vkey     = kb.VKey;
			unsigned short scancode = kb.MakeCode;
			unsigned short flags    = kb.Flags;
			unsigned int   message  = kb.Message;
			unsigned int   extra    = kb.ExtraInformation;


			bool up    = ((flags & RI_KEY_BREAK) == RI_KEY_BREAK);
			bool down  = !up;
			bool right = ((flags & RI_KEY_E0) == RI_KEY_E0);
			bool e1    = ((flags & RI_KEY_E1) == RI_KEY_E1);
			bool wm_up = (message == WM_KEYUP);

			u32 key = (scancode << 16) | ((flags & RI_KEY_E0) << 24);

			int k = 0;

			if (vkey || scancode)
			{
				dbg::println("VK: {}, ScanCode: {}, Message: {}, Flags: {}, Up: {}", vkey, scancode, message, flags, wm_up);
				dbg::println("Up: {}, Down: {}, Right: {}, E1: {}", up, down, right, e1);
				// Log::Write("Key: ", buffer, ", Unicode: ", cBuf, "\n");
			}
			if ((vkey || scancode) && up == true)
			{


				/*	m_unicodeString.append(cBuf);
					if(count == 1)
					{
						m_asciiBuffer.push_back(cbuf2[0]);
						m_asciicount++;
					}



					std::string s = m_asciiBuffer.data();


					Log::Write("\n###\nBUFFERED: ", m_unicodeString, "\n###\n");
					Log::Write("\n###\nASCII: ", s.substr(0, m_asciicount), "\n###ASCIICOUNT: ", m_asciicount, "\n");
				*/


				switch (vkey)
				{
						/*			case VK_BACK:
									{
										m_unicodeString = m_unicodeString.substr(0, m_unicodeString.size()-1);
										m_asciiBuffer.pop_back();
										m_asciicount--;
										break;
									}*/


					case VK_ESCAPE:
					{
						is_running = false;
						break;
					}

					case VK_F1:
					{


						break;
					}

					case VK_F2:
					{
						config[current_mode].extent.width  = 1'920;
						config[current_mode].extent.height = 1'080;

						resize();

						break;
					}


					case VK_F3:
					{
						config[current_mode].extent.width  = 1'280;
						config[current_mode].extent.height = 720;

						resize();

						break;
					};

					case VK_F4:
					{

						break;
					}


					case VK_F11:
					{
						toggle_fullscreen();
						break;
					}


					default:
					{
						break;
					}
				}
			}
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
				current_mode = Mode::Fullscreen;
				windowed     = false;
			}
			else
			{
				windowed     = true;
				current_mode = Mode::Windowed;

				const DWORD old_style = dwStyle | WS_OVERLAPPEDWINDOW;
				SetWindowLong(handle, GWL_STYLE, old_style);
				SetWindowPos(handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				SetWindowPlacement(handle, &wp);
			}

			dbg::println("toggle: {}x{}", config[current_mode].extent.width, config[current_mode].extent.height);
		}

		LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
		{

			switch (uMsg)
			{

				case WM_TIMER:
				{
					//				break;
#if 1
					RECT cr{};
					GetClientRect(handle, &cr);

					SetWindowTextA(
					  handle,
					  std::format("client_size: {}x{}, client {}x{} - {}",
								  config[current_mode].extent.width,
								  config[current_mode].extent.height,
								  cr.right - cr.left,
								  cr.bottom - cr.top,
								  current_dpi())
						.c_str());
					//
					break;
#endif
				};

				case WM_CREATE:
				{
					return 0;
				}

#if 0
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
						case DBT_DEVNODES_CHANGED:
						{
							break;
						}
						case DBT_DEVICEARRIVAL:
						{

							break;
						}
					}
					break;
				}
#endif

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

				case WM_GETMINMAXINFO:
				{
					auto new_min = adjust_to_dpi(min_extent, current_dpi());

					POINT minsize{(LONG)new_min.width, (LONG)new_min.height};
					((MINMAXINFO*)lParam)->ptMinTrackSize = minsize;
					return 0;
				}

				case WM_DPICHANGED:
				{
					u32       dpi   = HIWORD(wParam);
					const f32 scale = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
					resize();

					dbg::println("new dpi: {:.3f}, {}x{}", scale, config[current_mode].extent.width, config[current_mode].extent.height);

					return 0;
				}


				case WM_EXITSIZEMOVE:
				{
					is_sizing = false;
					return 0;
				}

				case WM_SIZING:
				{
					if (wParam != SIZE_MINIMIZED)
					{
						if (is_sizing)
						{
							RECT r{};
							GetClientRect(handle, &r);
							auto ne                     = to_extent(r);
							config[current_mode].extent = ne;
						}


						//	config[current_mode].extent = new_extent;
					}
					return 0;
				}

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

				case WM_SYSCOMMAND:
				{


					switch (wParam & 0xFFF0)
					{
						case SC_SIZE:
						{
							is_sizing = true;
							break;
						}
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
						config[current_mode].extent.width  = r.right - r.left;
						config[current_mode].extent.height = r.bottom - r.top;

						break;
					}

					if (vk == VK_F2)
					{
						config[Mode::Fullscreen].extent.width  = 1'920;
						config[Mode::Fullscreen].extent.height = 1'080;
						break;
					}
					if (vk == VK_F3)
					{
						config[Mode::Fullscreen].extent.width  = 1'280;
						config[Mode::Fullscreen].extent.height = 720;
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

					if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
					{
						const HRAWINPUT raw = reinterpret_cast<HRAWINPUT>(lParam);


						handle_input(raw);
					}

					// bool foreground = GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT;
					//  if (foreground)

					// DefWindowProc(hWnd, uMsg, wParam, lParam);
					return 0;
				}

				case WM_INPUT_DEVICE_CHANGE:
				{
					auto newDevice = reinterpret_cast<HANDLE>(lParam);

					// HandleRawInputChange(newDevice, (int)GET_RAWINPUT_CODE_WPARAM(wParam));
					dbg::println("rawinput wparam: {}", (int)GET_RAWINPUT_CODE_WPARAM(wParam));

					dbg::println("WM_INPUT_DEVICE_CHANGE\n");

					switch (wParam)
					{
						default: break;

						case GIDC_ARRIVAL:
						{
							dbg::println("Input device added\n");

							break;
						}

						case GIDC_REMOVAL:
						{
							dbg::println("Input device removed\n");

							break;
						}
					}

					auto hNewDevice = reinterpret_cast<HANDLE>(lParam);
					char buffer[128]{};
					u32  buffer_size = sizeof(buffer);
					auto infoResult  = GetRawInputDeviceInfoA(hNewDevice, RIDI_DEVICENAME, buffer, &buffer_size);
					if (SUCCEEDED(infoResult))
					{
						dbg::println("Device name: {}", buffer);
					}

					u32             deviceInfoSize = sizeof(RID_DEVICE_INFO);
					RID_DEVICE_INFO deviceInfo{};
					deviceInfo.cbSize = deviceInfoSize;
					auto result       = GetRawInputDeviceInfoA(hNewDevice, RIDI_DEVICEINFO, &deviceInfo, &deviceInfoSize);
					if (SUCCEEDED(result))
					{
						//					Log::Write("Device handle: ", hNewDevice, "\n");

						if (deviceInfo.dwType == RIM_TYPEKEYBOARD)
						{
							// 						Log::Write("Keyboard\n");
							// 						Log::Write("Keyboard mode: ",
							// deviceInfo.keyboard.dwKeyboardMode,
							// "\n"); 						Log::Write("Function keys: ",
							// deviceInfo.keyboard.dwNumberOfFunctionKeys,
							// "\n"); 						Log::Write("Indicators: ",
							// deviceInfo.keyboard.dwNumberOfIndicators,
							// "\n"); 						Log::Write("Total keys: ",
							// deviceInfo.keyboard.dwNumberOfKeysTotal,
							// "\n"); 						Log::Write("Type: ",
							// deviceInfo.keyboard.dwType,
							// "\n"); Log::Write("Sub type: ", deviceInfo.keyboard.dwSubType,
							// "\n");
						}

						if (deviceInfo.dwType == RIM_TYPEMOUSE)
						{
							// 						Log::Write("Mouse\n");
							// 						Log::Write("ID: ", deviceInfo.mouse.dwId, "\n");
							// 						Log::Write("Number of buttons: ",
							// deviceInfo.mouse.dwNumberOfButtons,
							// "\n"); 						Log::Write("Sample rate: ",
							// deviceInfo.mouse.dwSampleRate,
							// "\n"); 						Log::Write("Horizonal wheel: ",
							// deviceInfo.mouse.fHasHorizontalWheel ? "true" : "false",
							// "\n");
						}

						if (deviceInfo.dwType == RIM_TYPEHID)
						{
							// 						Log::Write("HID\n");
							// 						Log::Write("Product ID: ", deviceInfo.hid.dwProductId,
							// "\n"); 						Log::Write("Vendor ID: ",
							// deviceInfo.hid.dwVendorId,
							// "\n"); 						Log::Write("Version: ",
							// deviceInfo.hid.dwVersionNumber,
							// "\n"); 						Log::Write("Usage: ", deviceInfo.hid.usUsage,
							// "\n"); 						Log::Write("Usage page: ", deviceInfo.hid.usUsagePage,
							// "\n");
						}
					}

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

		// Only client size, only detect dpi change, calc new size on the client_size
		// WM_SIZE, save actual client size
		std::array<WindowConfig, 2> config;
		Mode                        current_mode{Mode::Windowed};


		GLuint u_angle{};
		f32    angle{};

		HWND              handle{nullptr};
		DWORD             style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
		DWORD             ex_style{0};
		WINDOWPLACEMENT   wp = {sizeof(WINDOWPLACEMENT)};
		std::vector<char> rawinput_buffer;
		extent            min_extent{640, 480};


		// vulkan::vulkan  vulkan;
		vulkan::vulkan vulkan;

		bool renderer_initialized{false};
		bool is_running{false};
		bool windowed{true};
		bool is_sizing{false};
		int  being_dragged{false};
	};

} // namespace deckard::app
