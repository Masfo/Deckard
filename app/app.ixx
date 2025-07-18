﻿
module;
#include <Windows.h>
#include <Xinput.h>
#include <dbt.h>
#include <dwmapi.h>
#include <hidusage.h>
#include <shellscalingapi.h>
#include <versionhelpers.h>
#include <windowsx.h>

export module deckard.app;
export import :inputs;

import std;
using namespace std::chrono_literals;

import deckard.as;
import deckard.system;
import deckard.types;
import deckard.vulkan;
import deckard.win32;
import deckard.debug;
import deckard.assert;
import deckard.enums;

namespace deckard::app
{

	export enum class Attribute : u8 {
		fullscreen       = BIT(0),
		togglefullscreen = BIT(1),
		vsync            = BIT(2),
		resizable        = BIT(3),
		gameticks        = BIT(4),


		pad5 = BIT(5),
		pad6 = BIT(6),
		pad7 = BIT(7),
	};

	export consteval void enable_bitmask_operations(Attribute);


	// callback
	class vulkanapp;

	export using input_keyboard_callback_ptr = void(vulkanapp & app, i32 key, i32 scancode, Action action, i32 mods);
	export using input_mouse_callback_ptr    = void(vulkanapp & app, i32 x, i32 y);

	export using initialize_callback_ptr = void(vulkanapp & app);
	export using close_callback_ptr      = void(vulkanapp & app);

	export using fixed_update_callback_ptr = void(vulkanapp & app, f32 fixed_dt);
	export using update_callback_ptr       = void(vulkanapp & app, f32 dt);
	export using render_callback_ptr       = void(vulkanapp & app);

	enum RawInputType : u32
	{
		// Keyboard,
		// Mouse,
		Pad,

		InputCount
	};

	using DwmSetWindowAttributePtr = HRESULT(HWND, DWORD, LPCVOID, DWORD);
	DwmSetWindowAttributePtr* DwmSetWindowAttribute{nullptr};


	enum class corner
	{
		system,
		small_rounded,
		rounded,
		square,
	};

	export class vulkanapp
	{
	public:
		struct properties
		{
			std::string title;
			u16         width{1920};
			u16         height{1080};
			Attribute   flags{Attribute::resizable | Attribute::vsync};
		};

	private:
		properties m_properties;

		vulkan::vulkan vulkan;
		// std::array<RAWINPUTDEVICE, 1> raw_inputs;
		// std::vector<char>             rawinput_buffer;


		HWND        handle{nullptr};
		extent<u16> min_extent{640, 480};
		extent<u16> normalized_client_size{0, 0};
		DWORD       style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
		DWORD       ex_style{0};

		inputs m_inputs;

		input_keyboard_callback_ptr* keyboard_callback{nullptr};

		fixed_update_callback_ptr* fixed_update_callback{nullptr};
		update_callback_ptr*       update_callback{nullptr};
		render_callback_ptr*       render_callback{nullptr};

		u32                                   game_ticks_per_second{60};
		std::chrono::steady_clock::time_point start_time;
		f32                                   m_deltatime{0.0f};


		bool renderer_initialized{false};
		bool is_running{false};
		bool is_sizing{false};
		bool is_minimized{false};
		bool show_cursor{false};

		LRESULT CALLBACK wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void toggle_fullscreen()
		{
			static WINDOWPLACEMENT wp{};

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
				set(Attribute::fullscreen, true);
			}
			else
			{
				set(Attribute::fullscreen, false);


				const DWORD old_style = dwStyle | WS_OVERLAPPEDWINDOW;
				SetWindowLong(handle, GWL_STYLE, old_style);
				SetWindowPos(handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				SetWindowPlacement(handle, &wp);
			}

			extent<u16> size = get_clientsize();
		}

		extent<u16> adjust_to_current_dpi(extent<u16> old)
		{
			const u32 dpi   = current_dpi();
			const f32 scale = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;

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

		u32 current_dpi() const { return GetDpiForWindow(handle); }

		f32 current_scale() const { return as<f32>(current_dpi()) / as<f32>(USER_DEFAULT_SCREEN_DPI); }

		extent<u16> normalize_client_size()
		{
			const f32 scale = current_scale();
			assert::check(scale >= 1.0f);

			RECT client_size{};
			GetClientRect(handle, &client_size);

			extent<u16> unnormalized_size = to_extent(client_size);

			normalized_client_size.width  = as<u16>(unnormalized_size.width / scale);
			normalized_client_size.height = as<u16>(unnormalized_size.height / scale);

			return normalized_client_size;
		}

		void set_client_size(const extent<u16> size)
		{
			normalized_client_size = size;
			resize();
		}

		extent<u16> get_clientsize() const
		{
			RECT r{};
			GetClientRect(handle, &r);
			return to_extent(r);
		}

		void set_darkmode(bool force = false)
		{
			const auto [major, minor, build] = system::OSBuildInfo();

			bool darkmode = force or system::is_darkmode();


			if (DwmSetWindowAttribute and IsWindows10OrGreater() and build >= 22000)
			{
				BOOL value = darkmode;
				DwmSetWindowAttribute(handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
			}
		}

		void set_square_corners(corner option)
		{
			const auto [major, minor, build] = system::OSBuildInfo();

			if (DwmSetWindowAttribute and IsWindows10OrGreater() and build >= 22000)
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

		void set_visible(bool visible) { ShowWindow(handle, visible ? SW_SHOW : SW_HIDE); }

		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################


#pragma region !Input
#if 0
		void input_initialize()
		{

#if 0
			// Keyboard
			raw_inputs[Keyboard].usUsagePage = HID_USAGE_PAGE_GENERIC;
			raw_inputs[Keyboard].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
			raw_inputs[Keyboard].dwFlags     = RIDEV_APPKEYS | RIDEV_NOLEGACY; // | RIDEV_NOHOTKEYS;
			raw_inputs[Keyboard].hwndTarget  = handle;


			// Mouse
			raw_inputs[Mouse].usUsagePage = HID_USAGE_PAGE_GENERIC;
			raw_inputs[Mouse].usUsage     = HID_USAGE_GENERIC_MOUSE;
			raw_inputs[Mouse].dwFlags     = RIDEV_NOLEGACY;
			raw_inputs[Mouse].hwndTarget  = handle;
#endif
			// Pad
			raw_inputs[Pad].usUsagePage = HID_USAGE_PAGE_GENERIC;
			raw_inputs[Pad].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
			raw_inputs[Pad].dwFlags     = RIDEV_DEVNOTIFY | RIDEV_NOLEGACY;
			raw_inputs[Pad].hwndTarget  = handle;

			if (RegisterRawInputDevices(raw_inputs.data(), as<u32>(raw_inputs.size()), sizeof(raw_inputs[0])) == 0)
			{
				dbg::println("Raw input not initialized");
				return;
			}

			std::vector<RAWINPUTDEVICELIST> riList;
			unsigned int                    deviceCount = 0;

			GetRawInputDeviceList(nullptr, &deviceCount, sizeof(RAWINPUTDEVICELIST));
			riList.resize(deviceCount);

			GetRawInputDeviceList(riList.data(), &deviceCount, sizeof(RAWINPUTDEVICELIST));

			RID_DEVICE_INFO rdi;
			rdi.cbSize = sizeof(RID_DEVICE_INFO);

			for (const auto& i : riList)
			{

				unsigned int size = 256;
				char         deviceName[256]{};
				GetRawInputDeviceInfoA(i.hDevice, RIDI_DEVICENAME, &deviceName, &size);
				// dbg::println("\nDevice name: {}", deviceName);

				size = rdi.cbSize;
				GetRawInputDeviceInfoA(i.hDevice, RIDI_DEVICEINFO, &rdi, &size);

				if (rdi.dwType == RIM_TYPEKEYBOARD)
				{
					dbg::println("Keyboard mode: {}", rdi.keyboard.dwKeyboardMode);
					dbg::println("Keyboard: function keys: {}", rdi.keyboard.dwNumberOfFunctionKeys);
					dbg::println("Keyboard indicators: {}", rdi.keyboard.dwNumberOfIndicators);
					dbg::println("Keyboard total keys: {}", rdi.keyboard.dwNumberOfKeysTotal);

					dbg::println("Keyboard type: {}", rdi.keyboard.dwType);
					dbg::println("Keyboard sub-type: {}", rdi.keyboard.dwSubType);
				}

				if (rdi.dwType == RIM_TYPEMOUSE)
				{
					dbg::println("Mouse id: {}", rdi.mouse.dwId);
					dbg::println("Mouse buttons: {}", rdi.mouse.dwNumberOfButtons);
					dbg::println("Mouse sample rate: {}", rdi.mouse.dwSampleRate);
					dbg::println("Mouse hori wheel: {}", rdi.mouse.fHasHorizontalWheel);
				}

#if 0
				if (rdi.dwType == RIM_TYPEHID)
				{
					dbg::println("HID productID: {}", rdi.hid.dwProductId);
					dbg::println("HID vendorID: {}", rdi.hid.dwVendorId);
					dbg::println("HID version: {}", rdi.hid.dwVersionNumber);
					dbg::println("HID usage: {}", rdi.hid.usUsage);
					dbg::println("HID usage page: {}", rdi.hid.usUsagePage);
				}
#endif

				dbg::println();
			}
		}
		void input_deinitialize()
		{
//
#if 0
			raw_inputs[Keyboard].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
			raw_inputs[Keyboard].dwFlags     = RIDEV_REMOVE;
			raw_inputs[Keyboard].usUsagePage = 0x01;
			raw_inputs[Keyboard].hwndTarget  = handle;


			// Mouse
			raw_inputs[Mouse].usUsage     = HID_USAGE_GENERIC_MOUSE;
			raw_inputs[Mouse].dwFlags     = RIDEV_REMOVE;
			raw_inputs[Mouse].usUsagePage = 0x01;
			raw_inputs[Mouse].hwndTarget  = handle;
#endif
			// Pad
			raw_inputs[Pad].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
			raw_inputs[Pad].dwFlags     = RIDEV_REMOVE;
			raw_inputs[Pad].usUsagePage = 0x01;
			raw_inputs[Pad].hwndTarget  = handle;

			if (RegisterRawInputDevices(raw_inputs.data(), as<u32>(raw_inputs.size()), sizeof(raw_inputs[0])) == 0)
			{
			}
		}

		void handle_mouse(const RAWMOUSE& rawmouse)
		{
			(rawmouse);
#if 0

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
#endif


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

		void handle_keyboard(const RAWKEYBOARD& kb)
		{
			unsigned short vkey     = kb.VKey;
			unsigned short scancode = kb.MakeCode;
			unsigned short flags    = kb.Flags;
			unsigned int   message  = kb.Message;
			// unsigned int   extra    = kb.ExtraInformation;


			bool up    = ((flags & RI_KEY_BREAK) == RI_KEY_BREAK);
			bool down  = !up;
			bool right = ((flags & RI_KEY_E0) == RI_KEY_E0);
			bool e1    = ((flags & RI_KEY_E1) == RI_KEY_E1);
			bool wm_up = (message == WM_KEYUP);

			// u32 key = (scancode << 16) | ((flags & RI_KEY_E0) << 24);

			if (vkey or scancode)
			{
				keyboard_callback(*this, vkey, scancode, up ? Action::Up : Action::Down, flags);
			}
		}

		void handle_input(const HRAWINPUT input) 
		{
			u32 size = 0;
			if (GetRawInputData(input, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0)
				return;

			RAWINPUT* rawInput = nullptr;

			if (size > 0)
			{

				rawinput_buffer.resize(as<size_t>(size));

				if (GetRawInputData(input, RID_INPUT, &rawinput_buffer[0], &size, sizeof(RAWINPUTHEADER)) != size)
					return;

				rawInput = reinterpret_cast<RAWINPUT*>(rawinput_buffer.data());


				// handle_keyboard(rawInput->data.keyboard);
				// handle_mouse(rawInput->data.mouse);
			}
			DefRawInputProc(&rawInput, 2, sizeof(RAWINPUTHEADER));
		}
#endif
#pragma endregion()

		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################>

	public:
		void set_title(std::string_view title)
		{
			//
			m_properties.title = title;
			if (handle)
				SetWindowTextA(handle, title.data());
		}

		void set_fixed_update_callback(fixed_update_callback_ptr* ptr)
		{
			if (ptr)
				fixed_update_callback = ptr;
		}

		void set_update_callback(update_callback_ptr* ptr)
		{
			if (ptr)
				update_callback = ptr;
		}

		void set_keyboard_callback(input_keyboard_callback_ptr* ptr)
		{
			if (ptr)
				keyboard_callback = ptr;
		}

		void set_render_callback(render_callback_ptr* ptr)
		{
			if (ptr)
				render_callback = ptr;
		}

		void resize(u16 width, u16 height)
		{
			if (width == 0 or height == 0)
				return;
			set_client_size({width, height});
		}

		void set(Attribute flags)
		{
			switch (flags)
			{


				case Attribute::togglefullscreen: toggle_fullscreen(); break;

				case Attribute::vsync:
				{
					bool vsync = not(m_properties.flags && Attribute::vsync);

					set(Attribute::vsync, vsync);

					break;
				}
				default: break;
			}
		}

		void set(Attribute flags, bool value)
		{
			switch (flags)
			{
				case Attribute::fullscreen:
				{
					if (value)
						m_properties.flags += Attribute::fullscreen;
					else
						m_properties.flags -= Attribute::fullscreen;

					break;
				}

				case Attribute::vsync:
				{
					if (value)
						m_properties.flags += Attribute::vsync;
					else
						m_properties.flags -= Attribute::vsync;

					vulkan.vsync(value);
					vulkan.resize();

					break;
				}
				default: break;
			}
		}

		void set(Attribute flags, u32 value)
		{
			switch (flags)
			{
				case Attribute::gameticks:
				{
					game_ticks_per_second = std::clamp(value, 1u, 0xFFFF'FFFFu);
					break;
				}
				default: break;
			}
		}

		auto get(Attribute flags) const
		{
			switch (flags)
			{
				case Attribute::gameticks: return game_ticks_per_second;
				default: return 0u;
			}
		}

		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		// ####################################################################################
		void poll_inputs() { m_inputs.poll(); }

	public:
		vulkanapp(vulkanapp&&)            = delete;
		vulkanapp& operator=(vulkanapp&&) = delete;

		vulkanapp(const vulkanapp&)            = delete;
		vulkanapp& operator=(const vulkanapp&) = delete;

		vulkanapp() { m_properties = {.title = "Default Vulkan app", .width = 1280, .height = 720}; }

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

			//
			if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
			{
				dbg::println("RegisterClassEx failed: {}", system::get_windows_error());
				destroy();
				return;
			}

			if (not(m_properties.flags && Attribute::resizable))
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
				dbg::println("CreateWindowEx failed: {}", system::get_windows_error());
				destroy();
				return;
			}


			resize();
			///


#if 0
			if (IsWindows10OrGreater() and build >= 22621)
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

			bool vsync = m_properties.flags && Attribute::vsync;

			if (not vulkan.initialize(handle, vsync))
			{
				dbg::println("Vulkan not initialized");
				return;
			}

			set_title(m_properties.title);


			//	input_initialize();

			is_running = true;
			start_time = clock_now();
		}

		void destroy()
		{
			// input_deinitialize();

			if (m_properties.flags && Attribute::fullscreen)
				toggle_fullscreen();
			vulkan.deinitialize();

			DestroyWindow(handle);
			UnregisterClass(L"DeckardWindowClass", GetModuleHandle(0));
		}

		bool handle_messages()
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

		void handle_messages_slow()
		{
			MSG msg{};
			if (!GetMessage(&msg, NULL, 0, 0))
				return;

			DispatchMessage(&msg);
		}

		void fullscreen() { toggle_fullscreen(); }

		void resize()
		{
			extent adjusted = adjust_to_current_dpi(normalized_client_size);


			SetWindowPos(
			  handle, nullptr, 0, 0, adjusted.width, adjusted.height, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}

		f32 time() const
		{
			auto current_time = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<std::chrono::duration<f32>>(current_time - start_time).count();
		}

		f32 deltatime() const { return m_deltatime; }

		void gameloop()
		{
			f32  fixed_timestep = 1.0f / game_ticks_per_second;
			u32  frames         = 0;
			u64  totalframes    = 0;
			auto last_time      = clock_now();

			f32       fps = 0;
			f32       max_fps{0};
			u32       update_tick        = 0;
			u32       max_update_tick    = 0;
			u32       total_update_ticks = 0;
			u32       loops              = 0;
			const u32 MAX_FRAMESKIP      = 5;
			u32       fixed_update_count = 0;

			enum Updates : u32
			{
				FPS,
				Accumulator,
				NextTitleUpdate,

				Timer5s,

				Count
			};

			std::array<f32, as<u32>(Updates::Count)> timers;
			timers.fill(0.0f);
			game_ticks_per_second = 5;

			// TODO: minimize and rendering

			while (handle_messages())
			{


				fixed_timestep = 1.0f / game_ticks_per_second;


				auto current_time = clock_now();
				m_deltatime       = std::chrono::duration_cast<std::chrono::duration<f32>>(current_time - last_time).count();
				last_time         = current_time;

				//


				loops = 0;
				timers[Accumulator] += m_deltatime;
				while (timers[Accumulator] >= fixed_timestep and loops < MAX_FRAMESKIP)
				{
					// Fixed update
					if (fixed_update_callback)
					{
						fixed_update_count += 1;
						fixed_update_callback(*this, fixed_timestep);
					}

					{
						update_tick++;
						max_update_tick = std::max(max_update_tick, update_tick);
						total_update_ticks++;
					}
					timers[Accumulator] -= fixed_timestep;
					loops++;
				}


				frames++;
				totalframes++;

				timers[FPS] += m_deltatime;
				if (timers[FPS] > 1.0f)
				{
					fps         = frames / timers[FPS];
					max_fps     = std::max(fps, max_fps);
					update_tick = 0;
					frames      = 0;

					timers[FPS] = 0;
				}

				//timers[Timer5s] += m_deltatime;
				//if (timers[Timer5s] > 5.0f)
				//{
				//	dbg::println(
				//	  "{:05.3f}. Fixed updates: {}/{} = {}",
				//	  time(),
				//	  game_ticks_per_second,
				//	  fixed_update_count,
				//	  fixed_update_count / game_ticks_per_second);
				//	fixed_update_count = 0;
				//	timers[Timer5s]    = 0.0f;
				//}


				timers[NextTitleUpdate] += deltatime();
				if (timers[NextTitleUpdate] > 0.05f)
				{

					set_title(std::format(
					  "[{:05.3f}] FPS: {:.3f}/{:.3f}/{}, D: {:.3f}, T: ({:3}[{:3}]/{:3})/{}",
					  time(),
					  fps,
					  max_fps,
					  totalframes,
					  deltatime(),
					  update_tick,
					  max_update_tick,
					  total_update_ticks,
					  game_ticks_per_second));

					timers[NextTitleUpdate] = 0.0f;
				}


				m_inputs.poll();

				auto& pad = m_inputs.controller();
				pad.vibrate(pad.left_trigger(), pad.right_trigger());

				// Update
				if (update_callback)
					update_callback(*this, deltatime());

				// Render
				if (render_callback)
					render_callback(*this);

				render();
			}
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

				  set_visible(true);

				  system::set_thread_name("App::Run");


				  // init_inputs();


				  gameloop(); // blocks


				  set_visible(false);

				  destroy();
				  app_return = 0;
			  });

			app_thread.join();
			return app_return;
		}

		void render()
		{
			//
			if (not is_minimized)
				vulkan.draw();
		}

		void quit() { is_running = false; }
	};

	// ##################################################################################
	// ##################################################################################
	// ##################################################################################
	// ##################################################################################
	// ##################################################################################
	// ##################################################################################
	// ##################################################################################
	// ##################################################################################


	LRESULT CALLBACK vulkanapp::wnd_proc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{

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

			case WM_SETCURSOR:
			{
				if (show_cursor == false and LOWORD(lParam) == HTCLIENT)
					SetCursor(0);
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

			case WM_SIZE:
			{
				if (wParam == SIZE_MINIMIZED)
					is_minimized = true;
				if (wParam == SIZE_RESTORED or wParam == SIZE_MAXIMIZED)
					is_minimized = false;
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
			// Mouse buttons
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
			{
				return 0;
			}


			// Keys down
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{


				i32 vk       = static_cast<i32>(wParam);
				i32 scancode = (lParam >> 16) & 0xff;

				bool alt   = (GetKeyState(Key::Alt) & 0x8000);
				bool shift = (GetKeyState(Key::Shift) & 0x8000);
				bool ctrl  = (GetKeyState(Key::Ctrl) & 0x8000);


				// bool wasDown = (lParam & (1 << 30)) != 0;
				// bool isDown  = (lParam & (1 << 31)) == 0;

				dbg::println("alt: {}, ctrl: {}, shift: {} - {}", alt, ctrl, shift, vk);

				if (keyboard_callback)
					keyboard_callback(*this, vk, scancode, Action::Down, 0);

				return 0;
			}
			/*
			std::vector<unsigned char> previous(256);
			std::vector<unsigned char> current(256);

			// update:
			current.swap(previous);
			GetKeyboardState(&current[0]);

			*/

			// Keys up
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{

				i32 vk       = static_cast<i32>(wParam);
				i32 scancode = (lParam >> 16) & 0xff;

				// bool wasDown = (lParam & (1 << 30)) != 0;
				// bool isDown  = (lParam & (1 << 31)) == 0;

				if (keyboard_callback)
					keyboard_callback(*this, vk, scancode, Action::Up, 0);


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

#if 0
			case WM_INPUT:
			{
				if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
				{
					const HRAWINPUT raw = reinterpret_cast<HRAWINPUT>(lParam);


					//handle_input(raw);
				}

				return 0;
			}
#endif

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
