
import deckard;
using namespace deckard;

int deckard_main()
{
	app::vulkanapp app01({.title  = "Example 02", //
						  .width  = 1'280,
						  .height = 720});

	return app01.run();
}
