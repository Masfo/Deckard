
import deckard;
using namespace deckard;
using namespace deckard::app;

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	dbg::println("key: {} - {}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	if (key == Key::Escape and action == Action::Up)
	{
		dbg::println("quit");
		app.quit();
	}

	if (key == Key::F11 and action == Action::Up)
	{
		app.set(Flag::ToggleFullscreen);
	}
}

int deckard_main()
{


	vulkanapp app01({.title  = "Example 01", //
					 .width  = 1'280,
					 .height = 720});

	app01.set_title(std::format("{}", sizeof(vulkanapp)));
	app01.set_keyboard_callback(keyboard_callback);
	return app01.run();
}
