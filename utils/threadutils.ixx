export module deckard.threadutil;

import std;
import deckard.function_ref;

namespace deckard::thread
{
	export template<typename T>
	class threadsafe
	{
	private:
		T          value;
		std::mutex mutex;

	public:
		template<typename... Args>
		threadsafe(Args&&... args)
			: value(std::forward<Args>(args)...)
		{
		}

		template<typename F>
		auto with_lock(F&& f) -> decltype(f(value))
		{
			std::lock_guard lock(mutex);
			return f(value);
		}
	};


} // namespace deckard::thread
