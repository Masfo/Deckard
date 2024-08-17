
module;
#include <Windows.h>
#include <dbt.h>
#include <dwmapi.h>
#include <hidusage.h>
#include <shellscalingapi.h>
#include <versionhelpers.h>
#include <windowsx.h>

export module deckard.app;

import std;
using namespace std::chrono_literals;

import deckard.as;
import deckard.system;
import deckard.types;
import deckard.vulkan;
import deckard.win32;
import deckard.debug;
import deckard.assert;

namespace deckard::app
{
	using namespace deckard::system;

	using DwmSetWindowAttributePtr = HRESULT(HWND, DWORD, LPCVOID, DWORD);
	DwmSetWindowAttributePtr* DwmSetWindowAttribute{nullptr};


	enum class corner
	{
		system,
		small_rounded,
		rounded,
		square,
	};

	extent to_extent(RECT r) { return {as<u32>(r.right - r.left), as<u32>(r.bottom - r.top)}; }

	export class vulkanapp
	{
	public:
		struct properties
		{
			std::string title;
			u32         width{1'920};
			u32         height{1'080};
			bool        resizable{true};
			bool        fullscreen{false};
		};

	private:
		HWND            handle{nullptr};
		DWORD           style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
		DWORD           ex_style{0};
		WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
		extent          min_extent{640, 480};
		extent          normalized_client_size;
		properties      m_properties;

		std::vector<char> rawinput_buffer;

		vulkan::vulkan vulkan;

		bool renderer_initialized{false};
		bool is_running{false};
		bool is_sizing{false};

		LRESULT CALLBACK wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
				m_properties.fullscreen = true;
			}
			else
			{
				m_properties.fullscreen = false;

				const DWORD old_style = dwStyle | WS_OVERLAPPEDWINDOW;
				SetWindowLong(handle, GWL_STYLE, old_style);
				SetWindowPos(handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				SetWindowPlacement(handle, &wp);
			}

			extent size = get_clientsize();
		}

		extent adjust_to_current_dpi(extent old)
		{
			const u32 dpi   = current_dpi();
			const f32 scale = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;

			extent ext;
			ext.width  = as<u32>(old.width * scale);
			ext.height = as<u32>(old.height * scale);

			RECT wr = {0, 0, (LONG)(ext.width), (LONG)(ext.height)};

			if (IsWindows10OrGreater())
				AdjustWindowRectExForDpi(&wr, style, FALSE, ex_style, dpi);
			else
				AdjustWindowRectEx(&wr, style, FALSE, ex_style);


			return to_extent(wr);
		}

		u32 current_dpi() const { return GetDpiForWindow(handle); }

		f32 current_scale() const { return as<f32>(current_dpi()) / as<f32>(USER_DEFAULT_SCREEN_DPI); }

		extent normalize_client_size()
		{
			const f32 scale = current_scale();
			assert::check(scale >= 1.0f);

			RECT client_size{};
			GetClientRect(handle, &client_size);

			extent unnormalized_size = to_extent(client_size);

			normalized_client_size.width  = as<u32>(unnormalized_size.width / scale);
			normalized_client_size.height = as<u32>(unnormalized_size.height / scale);

			return normalized_client_size;
		}

		void set_client_size(const extent size)
		{
			normalized_client_size = size;
			resize();
		}

		extent get_clientsize() const
		{
			RECT r{};
			GetClientRect(handle, &r);
			return to_extent(r);
		}

		void set_darkmode(bool force = false)
		{
			const auto [major, minor, build] = system::OSBuildInfo();

			bool darkmode = force or system::is_darkmode();


			if (DwmSetWindowAttribute and IsWindows10OrGreater() and build >= 22'000)
			{
				BOOL value = darkmode;
				DwmSetWindowAttribute(handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
			}
		}

		void set_square_corners(corner option)
		{
			const auto [major, minor, build] = system::OSBuildInfo();

			if (DwmSetWindowAttribute and IsWindows10OrGreater() and build >= 22'000)
			{
				DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_DEFAULT;
				switch (option)
				{
					case corner::small_rounded: preference = DWMWCP_ROUNDSMALL; break;
					case corner::rounded: preference = DWMWCP_ROUND; break;
					case corner::square: preference = DWMWCP_DONOTROUND; break;
					default: break;
				}

				DwmSetWindowAttribute(handle, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
			}
		}

		void set_title(std::string_view title) const { SetWindowTextA(handle, title.data()); }


	public:
		vulkanapp(vulkanapp&&)            = delete;
		vulkanapp& operator=(vulkanapp&&) = delete;

		vulkanapp(const vulkanapp&)            = delete;
		vulkanapp& operator=(const vulkanapp&) = delete;

		vulkanapp() { m_properties = {.title = "Default Vulkan app", .width = 1'280, .height = 720}; }

		vulkanapp(const properties props) { m_properties = props; }

		~vulkanapp() { destroy(); }

		void create()
		{

			if (IsWindows7OrGreater())
			{
				DwmSetWindowAttribute = system::get_address<DwmSetWindowAttributePtr*>("dwmapi.dll", "DwmSetWindowAttribute");
				// DwmExtendFrameIntoClientArea = system::get_address<DwmExtendFrameIntoClientAreaPtr*>("dwmapi.dll",
				// "DwmExtendFrameIntoClientArea");
				//
				// DwmEnableBlurBehindWindow =
				// system::get_address<DwmEnableBlurBehindWindowPtr*>("dwmapi.dll", "DwmEnableBlurBehindWindow");
			}


			if (IsWindows8Point1OrGreater())
			{
				using SetProcessDpiAwarenessFunc = HRESULT(PROCESS_DPI_AWARENESS);
				auto SetProcessDpiAwareness      = system::get_address<SetProcessDpiAwarenessFunc*>("Shcore.dll", "SetProcessDpiAwareness");

				if (SetProcessDpiAwareness)
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

			normalized_client_size = {m_properties.width, m_properties.height};


			WNDCLASSEX wc{};
			wc.cbSize        = sizeof(WNDCLASSEX);
			wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
			wc.lpszClassName = L"DeckardWindowClass";
			wc.hInstance     = GetModuleHandle(nullptr);

			wc.lpfnWndProc = [](HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
			{
				// https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992
				vulkanapp* self{nullptr};
				if (message == WM_CREATE)
				{
					LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
					self                = static_cast<vulkanapp*>(lpcs->lpCreateParams);
					self->handle        = hWnd;
					SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
				}
				else
				{
					self = reinterpret_cast<vulkanapp*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
				}

				if (self)
				{
					return self->wnd_proc(hWnd, message, wParam, lParam);
				}

				return DefWindowProc(hWnd, message, wParam, lParam);
			};

#if 1
			if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			{
				dbg::println("RegisterClassEx failed: {}", get_windows_error());
				destroy();
				return;
			}
#endif

			if (m_properties.resizable == false)
				style &= ~WS_SIZEBOX;


			handle = CreateWindowEx(
			  ex_style,
			  L"DeckardWindowClass",
			  L"",
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
				dbg::println("CreateWindowEx failed: {}", get_windows_error());
				destroy();
				return;
			}


			resize();
			///


#if 0
			if (IsWindows10OrGreater() and build >= 22'621)
			{
				constexpr auto DWMSBT_DISABLE         = 1; // Default
				constexpr auto DWMSBT_MAINWINDOW      = 2; // Mica
				constexpr auto DWMSBT_TRANSIENTWINDOW = 3; // Acrylic
				constexpr auto DWMSBT_TABBEDWINDOW    = 4; // Tabbed
				auto           DWMSBT_set             = DWMSBT_DISABLE;


				if (DwmSetWindowAttribute)
					DwmSetWindowAttribute(handle, DWMWA_SYSTEMBACKDROP_TYPE, &DWMSBT_set, sizeof(int));
			}

#endif

			// TODO: config file
			set_darkmode(true);
			set_square_corners(corner::square);


			SetTimer(handle, 0, 16, 0);


			if (not vulkan.initialize(handle))
			{
				dbg::println("Vulkan not initialized");
				return;
			}

			set_title(m_properties.title);

			ShowWindow(handle, SW_SHOW);
			RedrawWindow(handle, nullptr, nullptr, RDW_INTERNALPAINT);

			is_running = true;
		}

		void destroy()
		{
			vulkan.deinitialize();
			is_running = false;
			if (m_properties.fullscreen)
				toggle_fullscreen();

			DestroyWindow(handle);
			UnregisterClass(L"DeckardWindowClass", GetModuleHandle(0));
		}

		void handle_messages()
		{
			assert::check(handle != nullptr);
			MSG msg{};

			while (PeekMessage(&msg, handle, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		void resize()
		{
			extent adjusted = adjust_to_current_dpi(normalized_client_size);


			SetWindowPos(
			  handle, nullptr, 0, 0, adjusted.width, adjusted.height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		bool loop()
		{
			handle_messages();

			render();

			return is_running;
		}

		// impl
		i32 run()
		{
			i32          app_return = 0;
			std::jthread app_thread(
			  [&]
			  {
				  create();

				  if (not is_running)
				  {
					  dbg::println("App not initialized");
					  app_return = -1;
					  return;
				  }
				  system::set_thread_name("App::Run");


				  // init_inputs();

				  LARGE_INTEGER freq{};
				  QueryPerformanceFrequency(&freq);
				  LARGE_INTEGER start{}, last{};
				  LARGE_INTEGER now{};
				  QueryPerformanceCounter(&start);
				  last = start;


				  u32   currentframe{0};
				  float framerate{0};
				  float oldtime{0};
				  float newtime{1};
				  i64   counter{1};
				  i64   frequency{0};
				  float timeperframe{0.0f};
				  QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
				  float freq1 = 1.0f / frequency;

				  float accumulate{0.0f};


				  while (loop())
				  {
					  QueryPerformanceCounter((LARGE_INTEGER*)&counter);
					  newtime      = (float)counter * freq1;
					  timeperframe = newtime - oldtime;
					  framerate    = 1.0f / timeperframe;
					  oldtime      = newtime;
					  currentframe++;
					  accumulate += timeperframe;

					  accumulate = std::clamp(accumulate, 0.0f, 1.0f);

					  if (accumulate >= 1.0)
					  {
						  accumulate -= 1.0;
						  // set_title(std::format("{}, {}, Delta: {}", currentframe, framerate, timeperframe));
						  currentframe = 0;
					  }
				  }


				  // destroy_inputs();
				  destroy();
				  app_return = 0;
			  });

			app_thread.join();
			return app_return;
		}

		void render()
		{
			//
			vulkan.draw();
		}
	};

	LRESULT CALLBACK vulkanapp::wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{

		switch (uMsg)
		{

			case WM_TIMER:
			{
				//				break;
#if 0
					RECT cr{};
					GetClientRect(handle, &cr);

					SetWindowTextA(
					  handle,
					  std::format("client_size: {}x{}, actual {}x{} - {}",
								  normalized_client_size.width,
								  normalized_client_size.height,
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

				dbg::trace(
				  "WM_ACTIVATE: {} focused = {}, iconified = {}", "DeckardApp", focused ? "true" : "false", iconified ? "true" : "false");

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


				dbg::trace("WM_MOUSEHOVER: [{}x{}] | VK: {:#X}", xPos, yPos, (u32)wParam);
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
				extent new_min = adjust_to_current_dpi(min_extent);

				POINT minsize{(LONG)new_min.width, (LONG)new_min.height};
				((MINMAXINFO*)lParam)->ptMinTrackSize = minsize;
				return 0;
			}

			case WM_DPICHANGED:
			{

				RECT* new_rect = reinterpret_cast<RECT*>(lParam);

				if (!SetWindowPos(
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
				return 0;
			}


			case WM_EXITSIZEMOVE:
			{
				is_sizing = false;
				resize();
				return 0;
			}

			case WM_SIZING:
			{
				if (wParam != SIZE_MINIMIZED)
				{
					if (is_sizing)
					{
						normalize_client_size();
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

				// int x = GET_X_LPARAM(lParam);
				// int y = GET_Y_LPARAM(lParam);

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
				auto vk = static_cast<int>(wParam);
				// const int scancode = (lParam >> 16) & 0xff;

				// alt-enter
				if (vk == VK_RETURN and (lParam & 0x6000'0000) == 0x2000'0000)
				{
					toggle_fullscreen();
				}

				return 0;
			}

			case WM_SYSKEYUP:
			{
				// auto vk = static_cast<int>(wParam);

				// const int scancode = (lParam >> 16) & 0xff;

				return 0;
			}


			// Key
			case WM_KEYDOWN:
			{
				// auto vk = static_cast<int>(wParam);


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
					normalized_client_size.width  = r.right - r.left;
					normalized_client_size.height = r.bottom - r.top;

					break;
				}

				if (vk == VK_F2)
				{
					break;
				}
				if (vk == VK_F3)
				{
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
					// const HRAWINPUT raw = reinterpret_cast<HRAWINPUT>(lParam);


					// handle_input(raw);
				}

				return 0;
			}

			case WM_INPUT_DEVICE_CHANGE:
			{
				// auto newDevice = reinterpret_cast<HANDLE>(lParam);

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

} // namespace deckard::app
