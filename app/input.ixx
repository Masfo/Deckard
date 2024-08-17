module;
#include <windows.h>
#include <hidusage.h>


export module deckard.input;
import std;
import deckard.types;
import deckard.debug;
import deckard.as;

namespace deckard::input
{
	enum RawInputType : u32
	{
		Keyboard,
		Mouse,
		Pad,

		InputCount
	};

	export class rawinput
	{
	private:
		std::vector<char>             rawinput_buffer;
		std::array<RAWINPUTDEVICE, 3> raw_inputs;
		HWND                          handle{nullptr};

	public:
		void initialize(HWND hwnd)
		{
			handle = hwnd;
			// Keyboard
			raw_inputs[Keyboard].usUsagePage = HID_USAGE_PAGE_GENERIC;
			raw_inputs[Keyboard].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
			raw_inputs[Keyboard].dwFlags     = RIDEV_APPKEYS | RIDEV_NOLEGACY; // | RIDEV_NOHOTKEYS;
			raw_inputs[Keyboard].hwndTarget  = handle;


			// Mouse
			raw_inputs[Mouse].usUsagePage = HID_USAGE_PAGE_GENERIC;
			raw_inputs[Mouse].usUsage     = HID_USAGE_GENERIC_MOUSE;
			raw_inputs[Mouse].dwFlags     = 0;
			raw_inputs[Mouse].hwndTarget  = handle;

			// Pad
			raw_inputs[Pad].usUsagePage = HID_USAGE_PAGE_GENERIC;
			raw_inputs[Pad].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
			raw_inputs[Pad].dwFlags     = RIDEV_DEVNOTIFY;
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
				// char         deviceName[256]{};
				// GetRawInputDeviceInfoA(i.hDevice, RIDI_DEVICENAME, &deviceName, &size);
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

		void deinitialize()
		{
			//
			raw_inputs[Keyboard].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
			raw_inputs[Keyboard].dwFlags     = RIDEV_REMOVE;
			raw_inputs[Keyboard].usUsagePage = 0x01;
			raw_inputs[Keyboard].hwndTarget  = handle;


			// Mouse
			raw_inputs[Mouse].usUsage     = HID_USAGE_GENERIC_MOUSE;
			raw_inputs[Mouse].dwFlags     = RIDEV_REMOVE;
			raw_inputs[Mouse].usUsagePage = 0x01;
			raw_inputs[Mouse].hwndTarget  = handle;

			// Pad
			raw_inputs[Pad].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
			raw_inputs[Pad].dwFlags     = RIDEV_REMOVE;
			raw_inputs[Pad].usUsagePage = 0x01;
			raw_inputs[Pad].hwndTarget  = handle;

			if (RegisterRawInputDevices(raw_inputs.data(), as<u32>(raw_inputs.size()), sizeof(raw_inputs[0])) == 0)
			{
			}
		}

		void handle_input(const HRAWINPUT input) noexcept
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


				handle_keyboard(rawInput->data.keyboard);
				handle_mouse(rawInput->data.mouse);
			}
			DefRawInputProc(&rawInput, 2, sizeof(RAWINPUTHEADER));
		}


	private:
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
			(kb);
#if 1
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
						break;
					}

					case VK_F1:
					{


						break;
					}

					case VK_F2:
					{


						break;
					}


					case VK_F3:
					{


						break;
					};

					case VK_F4:
					{
						break;
					}
					case VK_F5:
					{
						break;
					}


					case VK_NUMPAD1:
					{
						break;
					}
					case VK_NUMPAD2:
					{
						break;
					}

					case VK_NUMPAD3:
					{

						break;
					}


					case VK_NUMPAD9:
					{
						break;
					}


					default:
					{
						break;
					}
				}
			}
#endif
		}
	};


#if 0
	};

	void app::init_inputs()
	{
		// Keyboard
		raw_inputs[Keyboard].usUsagePage = HID_USAGE_PAGE_GENERIC;
		raw_inputs[Keyboard].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
		raw_inputs[Keyboard].dwFlags     = RIDEV_APPKEYS | RIDEV_NOLEGACY; // | RIDEV_NOHOTKEYS;
		raw_inputs[Keyboard].hwndTarget  = wnd.get_handle();


		// Mouse
		raw_inputs[Mouse].usUsagePage = HID_USAGE_PAGE_GENERIC;
		raw_inputs[Mouse].usUsage     = HID_USAGE_GENERIC_MOUSE;
		raw_inputs[Mouse].dwFlags     = 0;
		raw_inputs[Mouse].hwndTarget  = wnd.get_handle();

		// Pad
		raw_inputs[Pad].usUsagePage = HID_USAGE_PAGE_GENERIC;
		raw_inputs[Pad].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
		raw_inputs[Pad].dwFlags     = RIDEV_DEVNOTIFY;
		raw_inputs[Pad].hwndTarget  = wnd.get_handle();

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
			// char         deviceName[256]{};
			// GetRawInputDeviceInfoA(i.hDevice, RIDI_DEVICENAME, &deviceName, &size);
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

	void app::destroy_inputs()
	{
		//
		raw_inputs[Keyboard].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
		raw_inputs[Keyboard].dwFlags     = RIDEV_REMOVE;
		raw_inputs[Keyboard].usUsagePage = 0x01;
		raw_inputs[Keyboard].hwndTarget  = wnd.get_handle();


		// Mouse
		raw_inputs[Mouse].usUsage     = HID_USAGE_GENERIC_MOUSE;
		raw_inputs[Mouse].dwFlags     = RIDEV_REMOVE;
		raw_inputs[Mouse].usUsagePage = 0x01;
		raw_inputs[Mouse].hwndTarget  = wnd.get_handle();

		// Pad
		raw_inputs[Pad].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
		raw_inputs[Pad].dwFlags     = RIDEV_REMOVE;
		raw_inputs[Pad].usUsagePage = 0x01;
		raw_inputs[Pad].hwndTarget  = wnd.get_handle();

		if (RegisterRawInputDevices(raw_inputs.data(), as<u32>(raw_inputs.size()), sizeof(raw_inputs[0])) == 0)
		{
		}
	}
#endif
} // namespace deckard::input
