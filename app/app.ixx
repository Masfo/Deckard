
module;
#include <Windows.h>

export module deckard.app;
export import :window;

using namespace std::chrono_literals;
import deckard.as;
import std;

namespace deckard
{
	namespace app
	{
		export class app
		{
		public:
			void init_inputs() noexcept;
			void destroy_inputs() noexcept;
			void run() noexcept;

		private:
			window                        wnd;
			std::array<RAWINPUTDEVICE, 3> raw_inputs;
		};

		void app::init_inputs() noexcept
		{
			// Keyboard
			raw_inputs[0].usUsage = 0x06; // Keyboard
			raw_inputs[0].dwFlags = RIDEV_APPKEYS | RIDEV_DEVNOTIFY | RIDEV_NOLEGACY;
			//| RIDEV_NOHOTKEYS;
			raw_inputs[0].usUsagePage = 0x01;
			raw_inputs[0].hwndTarget  = wnd.get_handle();


			// Mouse
			raw_inputs[1].usUsage     = 0x02; // Mouse
			raw_inputs[1].dwFlags     = 0;
			raw_inputs[1].usUsagePage = 0x01;
			raw_inputs[1].hwndTarget  = wnd.get_handle();

			// Pad
			raw_inputs[2].usUsage     = 0x05; // Pad
			raw_inputs[2].dwFlags     = 0;    // RIDEV_DEVNOTIFY;
			raw_inputs[2].usUsagePage = 0x01;
			raw_inputs[2].hwndTarget  = wnd.get_handle();

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

		void app::destroy_inputs() noexcept
		{
			//
			raw_inputs[0].usUsage     = 0x06; // Keyboard
			raw_inputs[0].dwFlags     = RIDEV_REMOVE;
			raw_inputs[0].usUsagePage = 0x01;
			raw_inputs[0].hwndTarget  = wnd.get_handle();


			// Mouse
			raw_inputs[1].usUsage     = 0x02; // Mouse
			raw_inputs[1].dwFlags     = RIDEV_REMOVE;
			raw_inputs[1].usUsagePage = 0x01;
			raw_inputs[1].hwndTarget  = wnd.get_handle();

			// Pad
			raw_inputs[2].usUsage     = 0x05; // Pad
			raw_inputs[2].dwFlags     = RIDEV_REMOVE;
			raw_inputs[2].usUsagePage = 0x01;
			raw_inputs[2].hwndTarget  = wnd.get_handle();

			if (RegisterRawInputDevices(raw_inputs.data(), as<u32>(raw_inputs.size()), sizeof(raw_inputs[0])) == 0)
			{
			}
		}

		// impl
		void app::run() noexcept
		{
			std::jthread t(
			  [&]
			  {
				  wnd.create();
				  if (!wnd.running())
					  return;

				  init_inputs();

				  while (wnd.loop())
				  {
				  }

				  destroy_inputs();
				  wnd.destroy();
			  });
		}


	} // namespace app
} // namespace deckard
