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

using namespace deckard::system;
using namespace std::string_view_literals;

// GL
namespace gl
{
	PFNGLCREATESHADERPROC            CreateShader;
	PFNGLSHADERSOURCEPROC            ShaderSource;
	PFNGLCREATEPROGRAMPROC           CreateProgram;
	PFNGLCOMPILESHADERPROC           CompileShader;
	PFNGLATTACHSHADERPROC            AttachShader;
	PFNGLLINKPROGRAMPROC             LinkProgram;
	PFNGLGETUNIFORMLOCATIONPROC      GetUniformLocation;
	PFNGLUSEPROGRAMPROC              UseProgram;
	PFNGLGENVERTEXARRAYSPROC         GenVertexArrays;
	PFNGLBINDVERTEXARRAYPROC         BindVertexArray;
	PFNGLGENBUFFERSPROC              GenBuffers;
	PFNGLBINDBUFFERPROC              BindBuffer;
	PFNGLBUFFERDATAPROC              BufferData;
	PFNGLVERTEXATTRIBPOINTERPROC     VertexAttribPointer;
	PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
	PFNGLUNIFORM1FPROC               Uniform1f;
	PFNGLGETSTRINGIPROC              GetStringi;

	// debug
	PFNGLDEBUGMESSAGECONTROLPROC  DebugMessageControl;
	PFNGLDEBUGMESSAGECALLBACKPROC DebugMessageCallback;
	PFNGLGETPROGRAMIVPROC         GetProgramiv;
	PFNGLGETPROGRAMINFOLOGPROC    GetProgramInfoLog;
	PFNGLGETSHADERIVPROC          GetShaderiv;
	PFNGLGETSHADERINFOLOGPROC     GetShaderInfoLog;

	PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT;
	PFNWGLGETSWAPINTERVALEXTPROC      wglGetSwapIntervalEXT;
	PFNWGLGETEXTENSIONSSTRINGEXTPROC  wglGetExtensionsStringEXT;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;


	std::unordered_set<std::string_view> extensions;
	std::unordered_set<std::string_view> wgl_extensions;

	bool has_gl_extension(std::string_view name) noexcept { return extensions.contains(name); }

	bool has_wgl_extension(std::string_view name) noexcept { return wgl_extensions.contains(name); }
} // namespace gl

namespace deckard::app
{
	using namespace deckard;

	struct WindowSize
	{
		u32 width{1'920};
		u32 height{1'080};
		f32 aspect{1.777f};
		u32 dpi{USER_DEFAULT_SCREEN_DPI};

		f32 update_aspect() noexcept
		{
			aspect = (f32)width / (f32)height;
			return aspect;
		}
	};

	struct Size
	{
		u32 width{};
		u32 height{};
	};

	auto RectToSize(const RECT& r) noexcept -> Size { return {(u32)r.right - r.left, (u32)r.bottom - r.top}; }

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
			vulkan.wait();

			is_running = false;
			if (not windowed)
			{
				toggle_fullscreen();
			}


			close_renderer();

			ReleaseDC(handle, dc);
			DestroyWindow(handle);
			UnregisterClass(L"DeckardWindowClass", GetModuleHandle(0));
			PostQuitMessage(0);
		}

		void set_title(std::string_view title) const noexcept { SetWindowTextA(handle, title.data()); }

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
			client_size.update_aspect();
			client_size.dpi = USER_DEFAULT_SCREEN_DPI;

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
			  client_size.width,
			  client_size.height,
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


			// Adjust to DPI scale
			RECT wr{0, 0, (LONG)client_size.width, (LONG)client_size.height};

			if (IsWindows10OrGreater())
			{
				u32 dpi         = GetDpiForWindow(handle);
				f32 scale       = as<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
				client_size.dpi = dpi;

				f32 fWidth  = as<f32>(client_size.width * scale);
				f32 fHeight = as<f32>(client_size.height * scale);

				u32 scaledWidth  = as<u32>(fWidth);
				u32 scaledHeight = as<u32>(fHeight);


				wr = {0, 0, (LONG)(scaledWidth), (LONG)(scaledHeight)};
				// scale dpi
				AdjustWindowRectExForDpi(&wr, style, FALSE, ex_style, dpi);

				auto [swidth, sheight] = RectToSize(wr);

				wr = {0, 0, (LONG)(swidth), (LONG)(sheight)};
			}
			else
			{
				AdjustWindowRectEx(&wr, style, FALSE, ex_style);
			}

			auto [adjustedWidth, adjustedHeight] = RectToSize(wr);

			SetWindowPos(
			  handle, nullptr, 0, 0, adjustedWidth, adjustedHeight, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);


			const auto [major, minor, build] = system::OSBuildInfo();

			if (IsWindows10OrGreater() and build >= 22'621)
			{
				constexpr auto DWMSBT_DISABLE         = 1; // Default
				constexpr auto DWMSBT_MAINWINDOW      = 2; // Mica
				constexpr auto DWMSBT_TRANSIENTWINDOW = 3; // Acrylic
				constexpr auto DWMSBT_TABBEDWINDOW    = 4; // Tabbed
				auto           DWMSBT_set             = DWMSBT_DISABLE;

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

			minsize.x = GetSystemMetrics(SM_CXMINTRACK);
			minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;


			SetTimer(handle, 0, 16, 0);


			// Init renderer
			dc = GetDC(handle);

			// vulkan2.initialize(handle);

			if (not vulkan.initialize(GetModuleHandle(nullptr), handle))
				return;


			// init_gl_renderer();

			//
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
						client_size.width  = 1'920;
						client_size.height = 1'080;
						break;
					}


					case VK_F3:
					{
						client_size.width  = 1'280;
						client_size.height = 720;
						break;
					};

					case VK_F4:
					{
						const int toggle_swap = 1 - gl::wglGetSwapIntervalEXT();
						gl::wglSwapIntervalEXT(toggle_swap);

						dbg::println("Toggle vsync: {}", toggle_swap);

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
				windowed = false;
			}
			else
			{
				windowed              = true;
				const DWORD old_style = dwStyle | WS_OVERLAPPEDWINDOW;
				SetWindowLong(handle, GWL_STYLE, old_style);
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
					//				break;
#if 1
					f32  dpi      = GetDpiForWindow(handle);
					f32  DPIScale = dpi / USER_DEFAULT_SCREEN_DPI;
					RECT cr{};
					GetClientRect(handle, &cr);

					RECT wr{};
					GetWindowRect(handle, &wr);

					SetWindowTextA(
					  handle,
					  std::format(
						"client_size: {}x{}, window {}x{}, client {}x{} - {:.3f}",
						client_size.width,
						client_size.height,
						wr.right - wr.left,
						wr.bottom - wr.top,
						cr.right - cr.left,
						cr.bottom - cr.top,
						DPIScale)
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
				{ // set window's minimum size
					((MINMAXINFO*)lParam)->ptMinTrackSize = minsize;
					// vulkan.resized                        = true;
					return 0;
				}

				case WM_GETDPISCALEDSIZE:
				{
					RECT  source = {0}, target = {0};
					SIZE* size = (SIZE*)lParam;

					AdjustWindowRectExForDpi(&source, style, FALSE, ex_style, GetDpiForWindow(handle));
					AdjustWindowRectExForDpi(&target, style, FALSE, ex_style, LOWORD(wParam));

					client_size.width = target.right - target.left;

					UINT dpi = (UINT)wParam;
					dbg::println("getdpiscaled: {}x{}", size->cx, size->cy);


					return TRUE;
				}


				case WM_DPICHANGED:
				{
#if 0
					{
						RECT* suggested = (RECT*)lParam;
						SetWindowPos(
						  handle,
						  nullptr,
						  0,
						  0,
						  suggested->right - suggested->left,
						  suggested->bottom - suggested->top,
						  SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
						break;
					}
#endif
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
					{
						dbg::println("<DPI_CHANGE>");
						{
							RECT cr{};
							GetClientRect(handle, &cr);

							RECT wr{};
							GetWindowRect(handle, &wr);
							dbg::println(
							  "client_size: {}x{}, window {}x{}, client {}x{}",
							  client_size.width,
							  client_size.height,
							  wr.right - wr.left,
							  wr.bottom - wr.top,
							  cr.right - cr.left,
							  cr.bottom - cr.top);
						}

						RECT* const new_size = (RECT*)lParam;

						u32 new_width = new_size->right - new_size->left;

						u32 new_height = new_size->bottom - new_size->top;


						auto newdpi     = HIWORD(wParam);
						f32  scale      = (f32)newdpi / USER_DEFAULT_SCREEN_DPI;
						client_size.dpi = newdpi;
						dbg::println("scaler DPI: {}, {:f}", newdpi, scale);


						RECT old{};
						GetWindowRect(handle, &old);


						f32 width  = as<f32>(old.right - old.left);
						f32 height = as<f32>(old.bottom - old.top);
						dbg::println("pre scale({}): {}x{}", scale, width, height);

						width /= scale;
						height /= scale;

						u32 cwidth  = std::ceil(width);
						u32 cheight = std::ceil(height);

						dbg::println("post scale({}): {}x{}", scale, cwidth, cheight);


						SetWindowPos(handle, nullptr, 0, 0, cwidth, cheight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

						RECT cr{}, wr;
						GetClientRect(hWnd, &cr);
						auto [sw, sh] = RectToSize(cr);

						GetWindowRect(hWnd, &wr);
						auto [cw, ch] = RectToSize(wr);

						dbg::println("{}x{} # {} - client {}x{}, window {}x{}", width, height, scale, sw, sh, cw, ch);

						dbg::println("</DPI_CHANGE>");

						return 0;
					}

					int const dpi2 = ::GetDpiForWindow(hWnd);
					dbg::println("GetDpiForWindow: {}", dpi2);


					u32       dpi         = HIWORD(wParam);
					f32       dpi3        = (f32)LOWORD(wParam) / (f32)USER_DEFAULT_SCREEN_DPI;
					int const previousDpi = LOWORD(wParam);
					dbg::println("DPI {}, old {}, manual {}", dpi, previousDpi, dpi3);

					client_size.dpi = dpi;
					LPRECT new_rect = reinterpret_cast<LPRECT>(lParam);

#if 0
					u32 new_width  = client_size.width;
					u32 new_height = client_size.height;
#else
					auto [new_width, new_height] = RectToSize(*new_rect);
#endif

					f32 scale = dpi3;
					// as<f32>(dpi) / USER_DEFAULT_SCREEN_DPI;

					f32 fWidth  = as<f32>(new_width * scale);
					f32 fHeight = as<f32>(new_height * scale);

					// RECT r{0, 0, (LONG)(fWidth), (LONG)(fHeight)};
					RECT r{0, 0, (LONG)(new_width), (LONG)(new_height)};

					AdjustWindowRectExForDpi(&r, style, false, ex_style, dpi2);

					i32 adjustedWidth{r.right - r.left};
					i32 adjustedHeight{r.bottom - r.top};
					u32 nw = std::floor(fWidth);
					u32 nh = std::floor(fHeight);
					dbg::println("Setting {}x{} => {}x{}", fWidth, fHeight, nw, nh);

					// SetWindowPos(hWnd, NULL, 0, 0, adjustedWidth, adjustedHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
					SetWindowPos(hWnd, NULL, 0, 0, new_width, new_height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

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
					if (wParam != SIZE_MINIMIZED)
					{
						int width  = lParam & 0xFFFF;
						int height = (lParam & 0xFFFF'0000) >> 16;
						// vulkan.resize(width, height);
					}

					break;
				}
#if 0
				case WM_SIZE:
				{
					return DefWindowProc(hWnd, uMsg, wParam, lParam);

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
#endif

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

		GLuint compile_shader(GLenum type, const char* source)
		{
			GLuint shader = gl::CreateShader(type);
			gl::ShaderSource(shader, 1, &source, NULL);
			gl::CompileShader(shader);

			GLint param;
			gl::GetShaderiv(shader, GL_COMPILE_STATUS, &param);
			if (!param)
			{
				char log[4'096]{};
				gl::GetShaderInfoLog(shader, sizeof(log), NULL, log);
				dbg::println("Compile: {}", log);
			}

			return shader;
		}

		GLuint link_program(GLuint vert, GLuint frag)
		{
			//
			GLuint program = gl::CreateProgram();
			gl::AttachShader(program, vert);
			gl::AttachShader(program, frag);
			gl::LinkProgram(program);

			GLint param;
			gl::GetProgramiv(program, GL_LINK_STATUS, &param);
			if (!param)
			{
				char log[4'096]{};
				gl::GetProgramInfoLog(program, sizeof(log), NULL, log);
				dbg::println("Link: {}", log);
			}

			return program;
		}

		void init_gl_renderer() noexcept
		{
			PIXELFORMATDESCRIPTOR pdf = {
			  .nSize        = sizeof(pdf),
			  .nVersion     = 1,
			  .dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			  .iPixelType   = PFD_TYPE_RGBA,
			  .cColorBits   = 32,
			  .cDepthBits   = 24,
			  .cStencilBits = 8,
			  .iLayerType   = PFD_MAIN_PLANE,
			};
			if (SetPixelFormat(dc, ChoosePixelFormat(dc, &pdf), &pdf) == 0)
			{
				dbg::println("SetPixelFormat failed: {}", get_windows_error());
				return;
			}

			HGLRC old = wglCreateContext(dc);

			if (old == nullptr)
			{
				dbg::println("wglCreateContext failed: {}", get_windows_error());
				return;
			}

			if (wglMakeCurrent(dc, old) == 0)
			{
				dbg::println("wglMakeCurrent failed: {}", get_windows_error());
				return;
			}


			int attribs[] = {
			  // WGL_CONTEXT_MAJOR_VERSION_ARB,
			  // 4,
			  // WGL_CONTEXT_MINOR_VERSION_ARB,
			  // 6,
			  WGL_CONTEXT_PROFILE_MASK_ARB,
			  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			  WGL_CONTEXT_FLAGS_ARB,
#ifdef _DEBUG
			  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else
			  WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,

#endif
			  0};

			// wgl
			gl::wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
			gl::wglGetExtensionsStringEXT  = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");


			HGLRC hglrc = gl::wglCreateContextAttribsARB(dc, old, attribs);
			if (hglrc == nullptr)
			{
				dbg::println("wglCreateContextAttribs failed: {}", glGetError());
				return;
			}

			if (wglMakeCurrent(dc, hglrc) == 0)
			{
				dbg::println("wglMakeCurrent failed: {}", get_windows_error());
				return;
			}

			if (wglDeleteContext(old) == 0)
			{
				dbg::println("wglDeleteContext failed: {}", get_windows_error());
				return;
			}


			gl::CreateShader            = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
			gl::ShaderSource            = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
			gl::CompileShader           = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
			gl::CreateProgram           = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
			gl::AttachShader            = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
			gl::LinkProgram             = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
			gl::GetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
			gl::UseProgram              = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
			gl::GenVertexArrays         = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
			gl::BindVertexArray         = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
			gl::GenBuffers              = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
			gl::BindBuffer              = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
			gl::BufferData              = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
			gl::VertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
			gl::EnableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
			gl::Uniform1f               = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
			gl::GetStringi              = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

			gl::DebugMessageControl  = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");
			gl::DebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
			gl::GetShaderiv          = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
			gl::GetShaderInfoLog     = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
			gl::GetProgramiv         = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
			gl::GetProgramInfoLog    = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");

			if (gl::wglGetExtensionsStringEXT)
			{
				const std::string_view extensions = gl::wglGetExtensionsStringEXT();
				for (const auto& extension : split(extensions))
					gl::wgl_extensions.insert(extension);
			}


			dbg::println("GL Vendor   : {}", (const char*)glGetString(GL_VENDOR));
			dbg::println("GL Renderer : {}", (const char*)glGetString(GL_RENDERER));
			dbg::println("GL Version  : {}", (const char*)glGetString(GL_VERSION));
			dbg::println("GL GLSL     : {}", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));


			int num_ext{};
			glGetIntegerv(GL_NUM_EXTENSIONS, &num_ext);
			for (int i = 0; i < num_ext; i++)
			{
				std::string_view ext = as<const char*>(gl::GetStringi(GL_EXTENSIONS, i));
				gl::extensions.insert(ext);
			}

			if (gl::has_wgl_extension("WGL_EXT_swap_control"))
			{
				gl::wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

				gl::wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

				if (gl::wglSwapIntervalEXT)
					gl::wglSwapIntervalEXT(1);
			}


			int flags{};
			glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
			if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
			{
				glEnable(GL_DEBUG_OUTPUT);

				gl::DebugMessageCallback(
				  [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
				  {
					  (length);
					  (userParam);
					  (id);

					  std::string_view source_sv{"Unknown"};
					  switch (source)
					  {
						  case GL_DEBUG_SOURCE_API: source_sv = "API"; break;
						  case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_sv = "Window system"; break;
						  case GL_DEBUG_SOURCE_SHADER_COMPILER: source_sv = "Shader compiler"; break;
						  case GL_DEBUG_SOURCE_THIRD_PARTY: source_sv = "Third party"; break;
						  case GL_DEBUG_SOURCE_APPLICATION: source_sv = "Application"; break;
						  case GL_DEBUG_SOURCE_OTHER: source_sv = "Application"; break;
						  default: break;
					  }

					  std::string_view type_sv{"Unknown"};
					  switch (type)
					  {
						  case GL_DEBUG_TYPE_ERROR: type_sv = "Error"; break;
						  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_sv = "Deprecated behavior"; break;
						  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_sv = "Undefined behavior"; break;
						  case GL_DEBUG_TYPE_PORTABILITY: type_sv = "Portability"; break;
						  case GL_DEBUG_TYPE_PERFORMANCE: type_sv = "Performance"; break;
						  case GL_DEBUG_TYPE_OTHER: type_sv = "Other"; break;

						  default: break;
					  }

					  std::string_view severity_sv{"Unknown"};
					  switch (severity)
					  {
						  case GL_DEBUG_SEVERITY_NOTIFICATION: severity_sv = "Notify"; break;
						  case GL_DEBUG_SEVERITY_LOW: severity_sv = "Low"; break;
						  case GL_DEBUG_SEVERITY_MEDIUM: severity_sv = "Medium"; break;
						  case GL_DEBUG_SEVERITY_HIGH: severity_sv = "High"; break;

						  default: break;
					  }
					  dbg::println("{}: {} - {} | {}", source_sv, type_sv, severity_sv, message);
				  },
				  0);
				gl::DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);

#if 1
				gl::DebugMessageControl(

				  GL_DEBUG_SOURCE_APPLICATION, // source
				  GL_DONT_CARE,                // type
				  GL_DONT_CARE,                // severity
				  0,
				  0,
				  GL_FALSE);
#endif
			}


			const char* vert_shader =
			  "#version 330\n"
			  "layout(location = 0) in vec2 point;\n"
			  "uniform float angle;\n"
			  "void main() {\n"
			  "    mat2 rotate = mat2(cos(angle), -sin(angle),\n"
			  "                       sin(angle), cos(angle));\n"
			  "    gl_Position = vec4(0.75 * rotate * point, 0.0, 1.0);\n"
			  "}\n";
			const char* frag_shader =
			  "#version 330\n"
			  "out vec4 color;\n"
			  "void main() {\n"
			  "    color = vec4(0.75, 0.15, 0.15, 0);\n"
			  "}\n";
			GLuint vert    = compile_shader(GL_VERTEX_SHADER, vert_shader);
			GLuint frag    = compile_shader(GL_FRAGMENT_SHADER, frag_shader);
			GLuint program = link_program(vert, frag);
			gl::UseProgram(program);

			//
			u_angle = gl::GetUniformLocation(program, "angle");

			float  SQUARE[] = {-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
			GLuint vao_point;
			gl::GenVertexArrays(1, &vao_point);
			gl::BindVertexArray(vao_point);

			GLuint vbo_point;
			gl::GenBuffers(1, &vbo_point);
			gl::BindBuffer(GL_ARRAY_BUFFER, vbo_point);
			gl::BufferData(GL_ARRAY_BUFFER, sizeof(SQUARE), SQUARE, GL_STATIC_DRAW);
			gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
			gl::EnableVertexAttribArray(0);
			gl::BindBuffer(GL_ARRAY_BUFFER, 0);

			glClearColor(0.0f, 0.5f, 0.75f, 1);

			renderer_initialized = true;
		}

		void close_renderer() noexcept
		{
			if (auto hglrc = wglGetCurrentContext())
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(hglrc);
			}
		}

		void render()
		{
#if 1
			if (not vulkan.draw())
				return;
#else

#ifdef _DEBUG
			if (not renderer_initialized)
				return;
#endif
			glClear(GL_COLOR_BUFFER_BIT);
			gl::Uniform1f(u_angle, angle += 0.01);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			// glMultiDrawArrays, SSBO

			SwapBuffers(dc);

#endif
		}

		// Only client size, only detect dpi change, calc new size on the client_size
		// WM_SIZE, save actual client size
		WindowSize scale_client_extent(f32 scale); // return new size
		WindowSize client_size_windowed;
		WindowSize client_size_fullscreen;

		f32 dpi_scale{1.0f};


		GLuint u_angle{};
		f32    angle{};

		HWND              handle{nullptr};
		HDC               dc{nullptr};
		DWORD             style{WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS};
		DWORD             ex_style{0};
		WINDOWPLACEMENT   wp = {sizeof(WINDOWPLACEMENT)};
		std::vector<char> rawinput_buffer;
		POINT             minsize{1, 1};

		vulkan::vulkan  vulkan;
		vulkan::vulkan2 vulkan2;

		bool renderer_initialized{false};
		bool is_running{false};
		bool windowed{true};
		int  being_dragged{false};
	};

} // namespace deckard::app
