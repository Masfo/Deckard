
import deckard;

using namespace deckard;

void keyboard_callback(app::vulkanapp& app, i32 key, i32 scancode, i32 action, i32 mods)
{
	dbg::println("key: {} - {}, {} - {}", key, scancode, action == 1 ? "UP" : "DOWN", mods);

	if (key == 27)
	{
		dbg::println("quit");
		app.quit();
	}
}

int deckard_main()
{

	app::vulkanapp app01({.title  = "Example 01", //
						  .width  = 1'280,
						  .height = 720});
	app01.set_keyboard_callback(keyboard_callback);
	return app01.run();
}
