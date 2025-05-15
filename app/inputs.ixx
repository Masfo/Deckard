module;
#include <Windows.h>
export module deckard.app:inputs;

import :pad;

import std;
import deckard.types;
import deckard.enums;
import deckard.as;
import deckard.debug;
import deckard.helpers;

namespace deckard::app
{

	export enum class Action { Up, Down };

	export enum Key : u32 {


		Escape     = VK_ESCAPE,
		Shift      = VK_SHIFT,
		Ctrl       = VK_CONTROL,
		Alt        = VK_MENU,
		LeftShift  = VK_LSHIFT,
		RightShift = VK_RSHIFT,
		LeftCtrl   = VK_LCONTROL,
		RightCtrl  = VK_RCONTROL,
		LeftAlt    = VK_LMENU,
		RightAlt   = VK_RMENU,
		Pause      = VK_PAUSE,
		Caps       = VK_CAPITAL,

		Enter     = VK_RETURN,
		Space     = VK_SPACE,
		Backspace = VK_BACK,
		Tab       = VK_TAB,
		PageUp    = VK_PRIOR,
		PageDown  = VK_NEXT,
		End       = VK_END,
		Home      = VK_HOME,

		Left  = VK_LEFT,
		Up    = VK_UP,
		Right = VK_RIGHT,
		Down  = VK_DOWN,

		Select      = VK_SELECT,
		Print       = VK_PRINT,
		Execute     = VK_EXECUTE,
		PrintScreen = VK_SNAPSHOT,
		Insert      = VK_INSERT,
		Del         = VK_DELETE,
		Help        = VK_HELP,

		F1  = VK_F1,
		F2  = VK_F2,
		F3  = VK_F3,
		F4  = VK_F4,
		F5  = VK_F5,
		F6  = VK_F6,
		F7  = VK_F7,
		F8  = VK_F8,
		F9  = VK_F9,
		F10 = VK_F10,
		F11 = VK_F11,
		F12 = VK_F12,

		LeftWin      = VK_LWIN,
		RightWin     = VK_RWIN,
		Applications = VK_APPS,

		Numpad0 = VK_NUMPAD0,
		Numpad1 = VK_NUMPAD1,
		Numpad2 = VK_NUMPAD2,
		Numpad3 = VK_NUMPAD3,
		Numpad4 = VK_NUMPAD4,
		Numpad5 = VK_NUMPAD5,
		Numpad6 = VK_NUMPAD6,
		Numpad7 = VK_NUMPAD7,
		Numpad8 = VK_NUMPAD8,
		Numpad9 = VK_NUMPAD9,

		Separator = VK_SEPARATOR,
		Multiply  = VK_MULTIPLY,
		Divide    = VK_DIVIDE,
		Add       = VK_ADD,
		Subtract  = VK_SUBTRACT,
		Decimal   = VK_DECIMAL,

		Num0 = '0',
		Num1 = '1',
		Num2 = '2',
		Num3 = '3',
		Num4 = '4',
		Num5 = '5',
		Num6 = '6',
		Num7 = '7',
		Num8 = '8',
		Num9 = '9',

		A = 'A',
		B = 'B',
		C = 'C',
		D = 'D',
		E = 'E',
		F = 'F',
		G = 'G',
		H = 'H',
		I = 'I',
		J = 'J',
		K = 'K',
		L = 'L',
		M = 'M',
		N = 'N',
		O = 'O',
		P = 'P',
		Q = 'Q',
		R = 'R',
		S = 'S',
		T = 'T',
		U = 'U',
		V = 'V',
		W = 'W',
		X = 'X',
		Y = 'Y',
		Z = 'Z',
	};

	export enum class Mouse : u8 {
		Left   = BIT(0),
		Middle = BIT(1),
		Right  = BIT(2),
		X1     = BIT(3),
		X2     = BIT(4),
	};

	export enum class Controller : u8 {
		//
	};


	export consteval void enable_bitmask_operations(Mouse);

	class inputs
	{
	private:
		std::array<unsigned char, 256> m_keyboard_state_previous{};
		std::array<unsigned char, 256> m_keyboard_state_current{};
		Mouse                          m_mouse{0};
		pad                            m_pad;

		void poll_mouse()
		{
			m_mouse = {};
			//
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
				m_mouse += Mouse::Left;

			if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
				m_mouse += Mouse::Right;

			if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
				m_mouse += Mouse::Middle;

			if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)
				m_mouse += Mouse::X1;

			if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)
				m_mouse += Mouse::X2;
		}

		void poll_keyboard()
		{
			m_keyboard_state_current.swap(m_keyboard_state_previous);
			GetKeyboardState(m_keyboard_state_current.data());
		}

		void poll_controller() { m_pad.poll(); }

		void fire_events()
		{
			//
		}


	public:
		void poll()
		{
			poll_keyboard();
			poll_mouse();
			poll_controller();
		}

		const pad& controller() const { return m_pad; }

		// bind key to event enum
		void bind(Key key, u32 event_id)
		{
			(key);
			(event_id);
		}

		void bind(Mouse button, u32 event_id)
		{
			(button);
			(event_id);
		}

		void bind(Controller button, u32 event_id)
		{
			(button);
			(event_id);
		}
	};

} // namespace deckard::app
