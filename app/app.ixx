
module;
#include <Windows.h>
#include <hidusage.h>

export module deckard.app;
export import :window;

using namespace std::chrono_literals;
import deckard.as;
import deckard.system;
import std;

namespace deckard
{
	namespace app
	{
		export class app
		{
		public:
			void init_inputs();
			void destroy_inputs();
			void run();

		private:
			window                        wnd;
			std::array<RAWINPUTDEVICE, 3> raw_inputs;

			enum RawInputType : uint32_t
			{
				Keyboard,
				Mouse,
				Pad,

				InputCount
			};
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

		// impl
		void app::run()
		{
			std::jthread t(
			  [&]
			  {
				  system::set_thread_name("App::Run");

				  wnd.create();
				  if (!wnd.running())
					  return;

				  init_inputs();

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


				  while (wnd.loop())
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
						  // wnd.set_title(std::format("{}, {}, Delta: {}", currentframe, framerate, timeperframe));
						  currentframe = 0;
					  }
				  }


				  destroy_inputs();
				  wnd.destroy();
			  });
		}


	} // namespace app
} // namespace deckard
