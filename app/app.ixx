export module deckard.app;
export import :window;

using namespace std::chrono_literals;

namespace deckard
{
	namespace app
	{
		export class app
		{
		public:
			void run() noexcept;

		private:
			window wnd;
		};

		// impl
		void app::run() noexcept
		{
			std::jthread t(
			  [&]
			  {
				  wnd.create();

				  while (wnd.loop())
				  {
				  }

				  wnd.destroy();
			  });
			std::this_thread::sleep_for(500ms);
			t.detach();
		}


	} // namespace app
} // namespace deckard
