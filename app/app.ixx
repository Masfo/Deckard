export module deckard.app;
export import :window;

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
			wnd.create();

			while (wnd.loop())
			{
			}

			wnd.destroy();
		}


	} // namespace app
} // namespace deckard
