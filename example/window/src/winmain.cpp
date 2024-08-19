
import deckard;
using namespace deckard;
using namespace deckard::app;

bool alt   = false;
bool enter = false;

void keyboard_callback(vulkanapp& app, i32 key, i32 scancode, Action action, i32 mods)
{
	dbg::println("key: {} - {}, {} - {}", key, scancode, action == Action::Up ? "UP" : "DOWN", mods);

	bool up = action == Action::Up;

	alt   = key == Key::Alt;
	enter = key == Key::Enter;

	if (alt and enter)
		app.set(Attribute::togglefullscreen);


	if (key == Key::Escape and up)
	{
		dbg::println("quit");
		app.quit();
	}

	if (key == Key::F1 and up)
	{
		app.set(Attribute::vsync);
	}

	if (up and (key == Key::F11 or key == 'F' or key == 'f'))
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
		app.set(Attribute::gameticks, 1u);
	}
}

void fixed_update(vulkanapp& app, f32 fixed_delta)
{
	//
}

void update(vulkanapp& app, f32 delta)
{
	//
}

void render(vulkanapp& app)
{
	//
}

int deckard_main()
{


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
