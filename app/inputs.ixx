module;
#include <Windows.h>

export module deckard.app:inputs;

import std;
import deckard.types;
import deckard.debug;
import deckard.utf8;

namespace deckard::app
{
	enum class key_state : u8
	{
		up            = 0,
		just_pressed  = 1,
		held          = 2,
		just_released = 3,
	};

	constexpr u32 MAX_KEY_COUNT           = 128;
	constexpr u32 DEFAULT_TEXTBUFFER_SIZE = 64;

	export class inputs
	{
	private:
		f32 m_poll_timer{0.0f};
		std::array<key_state, MAX_KEY_COUNT> m_key_states{};
		utf8::string textbuffer;

		void poll_keyboard()
		{
			std::array<bool, MAX_KEY_COUNT> down_now{};
			for (usize vk = 0; vk < down_now.size(); ++vk)
				down_now[vk] = (GetAsyncKeyState(static_cast<int>(vk)) & 0x8000) != 0;

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
		}

	public:
		auto initialize() -> std::expected<void, std::string>
		{
			m_poll_timer = 0.0f;
			textbuffer.reserve(DEFAULT_TEXTBUFFER_SIZE);
			return {};
		}

		void deinitialize()
		{
			m_key_states.fill(key_state::up);
			dbg::println("inputs: deinitialized");
		}

		void poll(f32 delta)
		{
			m_poll_timer += delta;
			if (m_poll_timer >= 0.008f)
			{
				poll_keyboard();
				m_poll_timer = 0.0f;
			}
		}

		void character_input(char32 c)
		{
			textbuffer.append(c);
			dbg::println("Character input: U+{:04X} -> '{}' | Buffer: '{}'", static_cast<u32>(c), utf8::view{c}, textbuffer);
		}

		auto take_character_buffer() { return std::exchange(textbuffer, utf8::string(DEFAULT_TEXTBUFFER_SIZE, 0)); }

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
