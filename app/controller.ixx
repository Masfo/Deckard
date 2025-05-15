module;
#include <Windows.h>
#include <Xinput.h>

export module deckard.app:pad;

import std;
import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.assert;

namespace deckard::app
{

	export class pad
	{
	private:
		bool         m_connected{false};
		XINPUT_STATE state{};

		// TODO: variable deadzone, option to resize
		// stick magnitude, from deadzone 0.0f - 1.0f


	public:
		pad()
		{
			//XINPUT_CAPABILITIES cap{};
			//DWORD               err = XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &cap);

		}

		void poll()
		{
			// Assume only one controller
			m_connected = XInputGetState(0, &state) == ERROR_SUCCESS;
		}

		// 0.0f - 1.0f
		void vibrate(f32 left, f32 right) const
		{

			left  = std::clamp(left, 0.0f, 1.0f);
			right = std::clamp(right, 0.0f, 1.0f);

			XINPUT_VIBRATION vibration{};

			vibration.wLeftMotorSpeed  = as<u16>(left * 65535);
			vibration.wRightMotorSpeed = as<u16>(right * 65535);

			// Assume only one controller
			XInputSetState(0, &vibration);
		}

		f32 left_thumb_x() const { return as<f32>((state.Gamepad.sThumbLX + 32768.0f) / 65535.0f) - 1.0f; }

		f32 left_thumb_y() const { return as<f32>((state.Gamepad.sThumbLY + 32768.0f) / 65535.0f) - 1.0f; }

		f32 right_thumb_x() const { return as<f32>((state.Gamepad.sThumbRX + 32768.0f) / 65535.0f) - 1.0f; }

		f32 right_thumb_y() const { return as<f32>((state.Gamepad.sThumbRY + 32768.0f) / 65535.0f) - 1.0f; }

		std::pair<f32, f32> thumb_direction(f32 x, f32 y) const
		{
			f32 LX = x;
			f32 LY = y;

			float magnitude = std::sqrt(LX * LX + LY * LY);

			constexpr f32 PI2 = 2.0f * std::numbers::pi_v<f32>;
			float         fx  = x;
			float         fy  = y;

			// Handle division by zero
			if (fx == 0.0f)
			{
				if (fy > 0.0f)
				{
					return {PI2, magnitude}; // 90 degrees
				}
				else if (fy < 0.0f)
				{
					return {-PI2, magnitude}; // -90 degrees
				}
				else
				{
					return {0.0f, magnitude}; // undefined angle
				}
			}

			// Calculate angle using atan2f
			float angle = std::atan2f(fy, fx);
			angle       = angle < 0 ? angle += PI2 : angle;

			f32 normalizedLX = LX / magnitude;
			f32 normalizedLY = LY / magnitude;

			if (normalizedLX < -1.0f)
				normalizedLX = -1.0f;

			if (normalizedLY < -1.0f)
				normalizedLY = -1.0f;

			f32 normalizedMagnitude{0};
			if (magnitude > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
			{
				if (magnitude > 32767)
					magnitude = 32767;

				magnitude -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

				normalizedMagnitude = magnitude / (32767 - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			}
			else
			{
				magnitude           = 0.0f;
				normalizedMagnitude = 0.0f;
			}

			return {angle, normalizedMagnitude};

			/*
			// #####

			//constexpr f32 tau = 2 * std::numbers::pi_v<f32>;

			// f32 rad = std::atan2f(normalizedLY, normalizedLX);
			f32 rad = std::atan2f(y, x);

			f32 deg = rad * (180.0f / std::numbers::pi_v<f32>);

			deg = (rad > 0 ? rad : (tau + rad)) * 360.0f / (tau);


			return {std::atan2f(y, x), normalizedMagnitude};
			return {x, y};
			return {rad, normalizedMagnitude};
			*/
		}

		std::pair<f32, f32> left_thumb_direction() const
		{
			f32 LX = state.Gamepad.sThumbLX;
			f32 LY = state.Gamepad.sThumbLY;
			return thumb_direction(LX, LY);
		}

		std::pair<f32, f32> right_thumb_direction() const
		{
			f32 LX = state.Gamepad.sThumbRX;
			f32 LY = state.Gamepad.sThumbRY;
			return thumb_direction(LX, LY);
		}

		f32 left_trigger() const { return as<f32>(state.Gamepad.bLeftTrigger / 255); }

		f32 right_trigger() const { return as<f32>(state.Gamepad.bRightTrigger / 255); }

		bool connected() const { return m_connected; }
	};

} // namespace deckard::app
