module;

#include <version> 

#ifdef __cpp_lib_function_ref
#error "use std::function_ref"
#endif

export module deckard.callbacks;

import deckard.types;
import std;

namespace deckard
{


	export template<u32 N, typename... Args>
	class fixed_callbacks
	{
		using callback_t = std::move_only_function<void(Args...) noexcept>;

		std::array<callback_t, N> callbacks{};
		u32                       count{0};

	public:
		fixed_callbacks()                                  = default;
		fixed_callbacks(const fixed_callbacks&)            = delete;
		fixed_callbacks& operator=(const fixed_callbacks&) = delete;
		fixed_callbacks(fixed_callbacks&&)                 = delete;
		fixed_callbacks& operator=(fixed_callbacks&&)      = delete;

		[[nodiscard]] bool add(callback_t callback) noexcept
		{
			if (count >= N or not callback)
				return false;

			callbacks[count++] = std::move(callback);
			return true;
		}

		template<typename F>
		requires std::is_nothrow_invocable_r_v<void, F&, Args...>
		[[nodiscard]] bool add(F&& f) noexcept
		{
			return add(callback_t{std::forward<F>(f)});
		}

		void invoke(Args... args) noexcept
		{
			for (auto& callback : std::span{callbacks}.first(count))
				callback(args...);
		}

		[[nodiscard]] constexpr u32 size() const noexcept { return count; }

		[[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }

		[[nodiscard]] constexpr bool full() const noexcept { return count >= N; }

		void clear() noexcept
		{
			for (auto& callback : std::span{callbacks}.first(count))
				callback = nullptr;

			count = 0;
		}
	};

} // namespace deckard
