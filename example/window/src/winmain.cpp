#include <windows.h>
#include <Commctrl.h>
import deckard;
using namespace deckard;
using namespace deckard::app;
import std;

std::array<unsigned char, 256> previous{0};
std::array<unsigned char, 256> current{0};

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	// dbg::println("key: {:#x} - {:#x}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	bool up = action == Action::Up;

	// num 6 -> 0x66, 0x4d
	// f     -> 0x46, 0x21

	if (up and key == Key::B)
	{
		current.swap(previous);
		{
			ScopeTimer<std::micro> _("state");
			GetKeyboardState(&current[0]);
		}

		auto shift = current[Key::Shift];

		int k = 0;
	}

	if (key == Key::Escape and up)
	{
		dbg::println("quit");
		app.quit();
	}

	if (key == Key::F1 and up)
	{
		app.set(Attribute::vsync);
	}

	if (up and (key == Key::F11 or key == Key::F))
	{
		app.set(Attribute::togglefullscreen);
	}

	if (key == Key::Numpad1 and up)
	{
		app.set(Attribute::gameticks, 60u);
	}
	if (key == Key::Numpad2 and up)
	{
		app.set(Attribute::gameticks, 30u);
	}

	if (key == Key::Numpad3 and up)
	{
		app.set(Attribute::gameticks, 1u);
	}
}

void fixed_update(vulkanapp&, f32 /*fixed_delta*/)
{
	//
}

void update(vulkanapp&, f32 /*delta*/)
{
	//
}

void render(vulkanapp&)
{
	//
}

int deckard_main()
{
#if 1
	{
		int                     nButtonPressed = 0;
		TASKDIALOGCONFIG        config         = {0};
		const TASKDIALOG_BUTTON buttons[]      = {
          {100, L"Update"},
          {101, L"Play now"},
        };

		const TASKDIALOG_BUTTON radiobuttons[] = {
		  {100, L"Update 2 "},
		  {101, L"Play now 2"},
		};
		config.cbSize             = sizeof(config);
		config.pszWindowTitle     = L"Deckard";
		config.hInstance          = 0;
		config.dwFlags            = TDF_USE_COMMAND_LINKS | TDF_ENABLE_HYPERLINKS;
		config.dwCommonButtons    = TDCBF_CLOSE_BUTTON;
		config.pszMainIcon        = TD_INFORMATION_ICON;
		config.pszMainInstruction = L"Taboo Builder";
		config.pszContent =
		  L"New update available, v1.0.1234.DEADBEEF\n\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings\n"
		  L"* Fixed server pings Fixed server pings Fixed server pings Fixed server pings\n"
		  L"* Update function no longer hangs\n"
		  "\n\n<a href=\"https://www.taboobuilder.com/patchnotes/\">Read more patch notes</a>\n";
		config.pButtons = buttons;
		config.cButtons = ARRAYSIZE(buttons);


		config.pfCallback = [](HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) -> HRESULT
		{
			switch (uNotification)
			{
				case TDN_HYPERLINK_CLICKED: dbg::println("url"); break;
				default: break;
			}
			return S_OK;
		};

		using TaskDialogIndirectFunc =
		  HRESULT(const TASKDIALOGCONFIG* pTaskConfig, int* pnButton, int* pnRadioButton, BOOL* pfVerificationFlagChecked);
		TaskDialogIndirectFunc* TaskDialogIndirect = nullptr;

		HMODULE mod = LoadLibraryA("Comctl32.dll");
		if (mod)
		{
			TaskDialogIndirect = reinterpret_cast<TaskDialogIndirectFunc*>(GetProcAddress(mod, "TaskDialogIndirect"));
		}

		if (TaskDialogIndirect)
		{
			auto err = TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
			if (SUCCEEDED(err))
			{
				switch (nButtonPressed)
				{
					case 0: dbg::println("Dialog failed"); break;
					case IDCANCEL: dbg::println("Dialog cancel"); break;
					case IDNO: dbg::println("Dialog no"); break;
					case IDRETRY: dbg::println("Dialog retry"); break;
					case IDYES: dbg::println("Dialog yes"); break;
					case 100: dbg::println("Update"); break;
					case 101: dbg::println("Play"); break;


					default: break;
				}
			}
		}

		FreeLibrary(mod);
	}
#endif
	// TODO: register key bindings to apps own enum
	//
	// enum player_movement
	// up,down,left,right, fire
	// bind(KEY_LEFT, left)
	// bind(KEY_SPACE, fire)  // both space and pad_a fires
	// bind(PAD_A, fire)

	// special enter textmode for input
	// keys.enter_text_mode()
	// end_text_mode(), // inputs keys as text?

	vulkanapp app01(
	  {.title  = "Example 01", //
	   .width  = 1280,
	   .height = 720,
	   .flags  = Attribute::vsync | Attribute::resizable});

	app01.set_title(std::format("{}", sizeof(vulkanapp)));

	app01.set_keyboard_callback(keyboard_callback);
	app01.set_fixed_update_callback(fixed_update);
	app01.set_update_callback(update);
	app01.set_render_callback(render);


	return app01.run();
}
