module;
#include <Windows.h>
#include <gameinput.h> // v3
#include <hidsdi.h>
#include <wrl/client.h>

export module deckard.app2:inputs;


import std;
import deckard.types;
import deckard.enums;
import deckard.as;
import deckard.debug;
import deckard.helpers;
import deckard.platform;
import deckard.utf8;

using namespace GameInput::v3;
using namespace std::string_view_literals;

export using Microsoft::WRL::ComPtr;

namespace deckard::app
{

	// TODO:
	//		- x sensitivity, y sensitivity
	//			* mouse and controller
	//		- deadzone	

	[[nodiscard]] std::optional<GameInputVersion> query_gameinput_dll_version(std::wstring_view dll_path) noexcept
	{
		DWORD handle{};
		DWORD size = GetFileVersionInfoSizeW(dll_path.data(), &handle);
		if (size == 0)
			return std::nullopt;

		std::vector<std::byte> buffer(size);
		if (!GetFileVersionInfoW(dll_path.data(), handle, size, buffer.data()))
			return std::nullopt;

		VS_FIXEDFILEINFO* info{};
		UINT              info_size{};
		if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<void**>(&info), &info_size))
			return std::nullopt;

		return GameInputVersion{
		  .major    = HIWORD(info->dwFileVersionMS),
		  .minor    = LOWORD(info->dwFileVersionMS),
		  .build    = HIWORD(info->dwFileVersionLS),
		  .revision = LOWORD(info->dwFileVersionLS),
		};
	}

	auto get_friendly_device_name(u16 vendor_id) -> std::optional<std::string>
	{
		constexpr u16 generic_desktop_page = 0x01;
		constexpr u16 joystick_usage       = 0x04;
		constexpr u16 gamepad_usage        = 0x05;

		UINT count{};
		if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0)
			return {};

		std::vector<RAWINPUTDEVICELIST> devices(count);
		if (GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) == static_cast<UINT>(-1))
			return {};

		std::vector<std::string> candidates;

		for (const auto& entry : devices)
		{
			if (entry.dwType != RIM_TYPEHID)
				continue; // skip mice/keyboards, we only want RIM_TYPEHID gamepad-style devices

			RID_DEVICE_INFO info{.cbSize = sizeof(info)};
			UINT            info_size = sizeof(info);
			if (GetRawInputDeviceInfoA(entry.hDevice, RIDI_DEVICEINFO, &info, &info_size) == static_cast<UINT>(-1))
				continue;

			if (info.hid.dwVendorId != vendor_id)
				continue;

			//  filter only gamepads and joysticks
			if (info.hid.usUsagePage != generic_desktop_page
				|| (info.hid.usUsage != joystick_usage && info.hid.usUsage != gamepad_usage))
				continue;

			UINT path_chars{};
			GetRawInputDeviceInfoA(entry.hDevice, RIDI_DEVICENAME, nullptr, &path_chars);
			if (path_chars == 0)
				continue;

			std::string path(path_chars, '\0');
			if (GetRawInputDeviceInfoA(entry.hDevice, RIDI_DEVICENAME, path.data(), &path_chars) == static_cast<UINT>(-1))
				continue;


			HANDLE handle = CreateFileA(
			  path.c_str(),
			  GENERIC_READ | GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  nullptr,
			  OPEN_EXISTING,
			  0,
			  nullptr);

			if (handle == INVALID_HANDLE_VALUE)
				continue;

			wchar_t product[126 + 1]{};
			if (HidD_GetProductString(handle, product, sizeof(product)))
			{
				int         mblen = WideCharToMultiByte(CP_UTF8, 0, product, -1, nullptr, 0, nullptr, nullptr);
				std::string name(mblen - 1, '\0');
				WideCharToMultiByte(CP_UTF8, 0, product, -1, name.data(), mblen, nullptr, nullptr);
				candidates.push_back(std::move(name));
			}

			CloseHandle(handle);
		}

		if (candidates.size() == 1)
		{
			std::string      name = candidates.front();
			std::string_view name_view(name);

			constexpr std::string_view prefix = "Controller ("sv;
			if (name_view.starts_with(prefix))
			{
				name_view.remove_prefix(prefix.size());

				if (name_view.ends_with(")"sv))
					name_view.remove_suffix(1);

				return std::string(name_view);
			}
			return name;
		}

		return {};
	}

	// #####################################################################################


	struct gamepad_slot
	{
		IGameInputDevice*     device       = nullptr;
		IGameInputReading*    last_reading = nullptr;
		GameInputGamepadState current{};
		GameInputGamepadState previous{};
		std::string           name;
		bool                  connected = false;
	};

	enum class key_state : u8
	{
		up            = 0,
		just_pressed  = 1,
		held          = 2,
		just_released = 3,
	};

	constexpr u32 MAX_GAMEPAD_COUNT       = 4;
	constexpr u32 MAX_KEY_COUNT           = 128;
	constexpr u32 DEFAULT_TEXTBUFFER_SIZE = 64;

	export class inputs
	{
	private:
		GameInputCallbackToken callback_token{};
		GameInputCallbackToken systembutton_token{};

		IGameInput* gameinput{nullptr};

		f32 m_poll_timer{0.0f};

		// gamepads
		u8                                                   gamepad_count{0};
		std::array<gamepad_slot, MAX_GAMEPAD_COUNT>          m_gamepads{};
		std::array<GameInputGamepadState, MAX_GAMEPAD_COUNT> m_gamepad_states{};


		// keyboard
		std::array<key_state, MAX_KEY_COUNT> m_key_states{};


		// text input
		utf8::string textbuffer;

		// mouse
		i32 m_mouse_x{0};
		i32 m_mouse_y{0};

		void poll_gamepads()
		{
			for (u32 i = 0; i < gamepad_count; ++i)
			{
				gamepad_slot& slot = m_gamepads[i];

				if (slot.device == nullptr)
				{
					slot.connected = false;
					continue;
				}

				IGameInputReading* reading{};
				const HRESULT      hr = gameinput->GetCurrentReading(GameInputKindGamepad, slot.device, &reading);

				if (FAILED(hr))
				{
					slot.connected = false;
					continue;
				}

				slot.connected = true;

				IGameInputReading* current = reading;

				while (current != nullptr)
				{
					GameInputGamepadState state{};
					if (current->GetGamepadState(&state))
					{
						slot.previous = slot.current;
						slot.current  = state;

						dbg::println(
						  "[{}] Gamepad state: buttons={:014b} | LS=({:+1.5f},{:+1.5f}) RS=({:+1.5f},{:+1.5f}) | "
						  "LT={:+1.5f} RT={:+1.5f}",
						  current->GetTimestamp(),
						  as<u32>(state.buttons),
						  state.leftThumbstickX,
						  state.leftThumbstickY,
						  state.rightThumbstickX,
						  state.rightThumbstickY,
						  state.leftTrigger,
						  state.rightTrigger);
					}

					IGameInputReading* prev{};
					const HRESULT prev_hr = gameinput->GetPreviousReading(current, GameInputKindGamepad, slot.device, &prev);

					if (current != reading)
						current->Release();

					if (FAILED(prev_hr) or prev == slot.last_reading)
					{
						if (prev)
							prev->Release();
						current = nullptr;
					}
					else
					{
						current = prev;
					}
				}

				if (slot.last_reading)
					slot.last_reading->Release();

				slot.last_reading   = reading;
				m_gamepad_states[i] = slot.current;
			}
		}

		void poll_keyboard()
		{
			constexpr u32                 max_key_count = 128;
			std::array<u8, max_key_count> down_now{};
			IGameInputReading*            reading{};

			if (SUCCEEDED(gameinput->GetCurrentReading(GameInputKindKeyboard, nullptr, &reading)))
			{
				std::array<GameInputKeyState, max_key_count> keys{};
				const u32 count = reading->GetKeyState(static_cast<u32>(keys.size()), keys.data());

				for (u32 i = 0; i < count and i < keys.size(); ++i)
				{
					const u32 vk = keys[i].virtualKey;
					if (vk < down_now.size())
						down_now[vk] = true;
				}

				reading->Release();
			}

			for (usize vk = 0; vk < m_key_states.size(); ++vk)
			{
				const bool was_down = (m_key_states[vk] == key_state::just_pressed or m_key_states[vk] == key_state::held);
				const bool is_down  = down_now[vk];

				if (is_down and not was_down)
					m_key_states[vk] = key_state::just_pressed;
				else if (is_down and was_down)
					m_key_states[vk] = key_state::held;
				else if (not is_down and was_down)
					m_key_states[vk] = key_state::just_released;
				else
					m_key_states[vk] = key_state::up;
			}
		};

	public:
		auto initialize() -> std::expected<void, std::string>
		{


			if (not SUCCEEDED(GameInputCreate(&gameinput)))
				return std::unexpected("Failed to initialize GameInput.");

			Microsoft::WRL::ComPtr<GameInput::v3::IGameInput> v3;
			if (not SUCCEEDED(gameinput->QueryInterface(IID_PPV_ARGS(v3.GetAddressOf()))))
				return std::unexpected("Failed to query IGameInput v3 interface.");

#ifdef _DEBUG
			gameinput->SetFocusPolicy(GameInputEnableBackgroundInput);
#else
			gameinput->SetFocusPolicy(GameInputDefaultFocusPolicy);
#endif

			// callback for system button presses
			gameinput->RegisterSystemButtonCallback(
			  nullptr,
			  GameInputSystemButtonGuide | GameInputSystemButtonShare,
			  this,
			  [](GameInputCallbackToken,
				 void*                  context,
				 IGameInputDevice*      device,
				 u64                    timestamp,
				 GameInputSystemButtons current_buttons,
				 GameInputSystemButtons prev_buttons)
			  {
				  inputs* self = static_cast<inputs*>(context);
				  (self);

				  const GameInputDeviceInfo* info{};
				  device->GetDeviceInfo(&info);
				  dbg::println(
					"[{}] System button pressed: current_buttons={:#x}, prev_buttons={:#x}, Device ID: {}, Family: {}, "
					"Display Name: {}, PnP Path: {}",
					timestamp,
					static_cast<unsigned>(current_buttons),
					static_cast<unsigned>(prev_buttons),
					info->deviceId.value,
					std::to_underlying(info->deviceFamily),
					info->displayName,
					info->pnpPath);
			  },
			  &systembutton_token);


			// Callback for connection/disconnection of gamepads
			gameinput->RegisterDeviceCallback(
			  nullptr,
			  GameInputKindGamepad | GameInputKindKeyboard | GameInputKindMouse,
			  GameInputDeviceConnected,
			  GameInputBlockingEnumeration,
			  this,
			  [](GameInputCallbackToken,
				 void*                 context,
				 IGameInputDevice*     device,
				 u64                   timestamp,
				 GameInputDeviceStatus status,
				 GameInputDeviceStatus prev_status)

			  {
				  _ = prev_status;

				  inputs* const self = static_cast<inputs*>(context);

				  const GameInputDeviceInfo* info{};
				  device->GetDeviceInfo(&info);

				  const bool is_gamepad = (info->supportedInput & GameInputKindGamepad) != 0;
				  if (not is_gamepad)
					  return;

				  if (status & GameInputDeviceConnected)
				  {

					  auto it = std::ranges::find(self->m_gamepads, device, &gamepad_slot::device);
					  if (it == self->m_gamepads.end())
						  it = std::ranges::find(self->m_gamepads, nullptr, &gamepad_slot::device);

					  if (it != self->m_gamepads.end())
					  {
						  device->AddRef();
						  it->device    = device;
						  it->connected = true;
						  it->name      = get_friendly_device_name(info->vendorId).value_or(info->displayName);

						  const auto slot_index = static_cast<u8>(std::distance(self->m_gamepads.begin(), it));
						  self->gamepad_count   = std::max(self->gamepad_count, static_cast<u8>(slot_index + 1));

						  dbg::println(
							"[{}] Gamepad connected: Device ID: {}, Family: {}, Display Name: {}, PnP Path: {}",
							timestamp,
							info->deviceId.value,
							std::to_underlying(info->deviceFamily),
							it->name,
							info->pnpPath);
					  }
				  }
				  else
				  {

					  auto it = std::ranges::find(self->m_gamepads, device, &gamepad_slot::device);
					  if (it != self->m_gamepads.end())
					  {
						  if (it->last_reading)
							  it->last_reading->Release();

						  *it = gamepad_slot{};
					  }
				  }
			  },
			  &callback_token);


			// text buffer
			textbuffer.reserve(DEFAULT_TEXTBUFFER_SIZE);


			return {};
		}

		void deinitialize()
		{
			for (auto& slot : m_gamepads)
			{
				if (slot.last_reading)
					slot.last_reading->Release();
				if (slot.device)
					slot.device->Release();

				slot.device = nullptr;
			}
			m_gamepads.fill({});

			if (gameinput)
			{
				gamepad_count = 0;
				gameinput->UnregisterCallback(systembutton_token);
				gameinput->UnregisterCallback(callback_token);
				gameinput->Release();
				gameinput = nullptr;
			}
			dbg::println("inputs: deinitialized");
		}

		void poll(f32 delta)
		{
			m_poll_timer += delta;
			if (m_poll_timer >= 0.008f) // 125 Hz
			{
				poll_keyboard();
				poll_gamepads();
				m_poll_timer = 0.0f;
			}
		}

		void character_input(char32 c)
		{

			textbuffer.append(c);
			dbg::println("Character input: U+{:04X} -> '{}' | Buffer: '{}'", static_cast<u32>(c), utf8::view{c}, textbuffer);
		}

		auto take_character_buffer() { return std::exchange(textbuffer, utf8::string(DEFAULT_TEXTBUFFER_SIZE, 0)); }

		//

		[[nodiscard]] bool is_key_down(u32 vk) const
		{
			if (vk >= m_key_states.size())
				return false;

			return m_key_states[vk] == key_state::just_pressed or m_key_states[vk] == key_state::held;
		}

		[[nodiscard]] bool is_key_up(u32 vk) const
		{
			if (vk >= m_key_states.size())
				return false;

			return m_key_states[vk] == key_state::just_released or m_key_states[vk] == key_state::up;
		}

		[[nodiscard]] bool is_key_just_pressed(u32 vk) const
		{
			if (vk >= m_key_states.size())
				return false;

			return m_key_states[vk] == key_state::just_pressed;
		}

		[[nodiscard]] bool is_key_just_released(u32 vk) const
		{
			if (vk >= m_key_states.size())
				return false;

			return m_key_states[vk] == key_state::just_released;
		}
	};


} // namespace deckard::app
