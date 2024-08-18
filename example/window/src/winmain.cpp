
import deckard;
using namespace deckard;
using namespace deckard::app;

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	dbg::println("key: {} - {}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	bool up = action == Action::Up;

	if (key == Key::Escape and up)
	{
		dbg::println("quit");
		app.quit();
	}

	if (key == Key::F1 and up)
	{
		app.set(Attribute::vsync);
	}

	if (key == Key::F11 and up)
	{
		app.set(Attribute::togglefullscreen);
	}

	if (key == Key::NUM1 and up)
	{
		app.set(Attribute::gameticks, 60u);
	}
	if (key == Key::NUM2 and up)
	{
		app.set(Attribute::gameticks, 30u);
	}

	if (key == Key::NUM3 and up)
	{
		app.set(Attribute::gameticks, 15u);
	}
}

int deckard_main()
{


	vulkanapp app01(
	  {.title  = "Example 01", //
	   .width  = 1280,
	   .height = 720,
	   .flags  = Attribute::windowed | Attribute::vsync | Attribute::resizable});

	app01.set_title(std::format("{}", sizeof(vulkanapp)));

	app01.set_keyboard_callback(keyboard_callback);
	return app01.run();
}
