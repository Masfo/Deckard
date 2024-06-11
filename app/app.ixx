export module deckard.app;
export import :window;

namespace deckard
{
	namespace app
	{
		class app
		{
		public:
			void run() noexcept;

		private:
			window m_window;
		};

		// impl
		void app::run() noexcept { }


	} // namespace app
} // namespace deckard
