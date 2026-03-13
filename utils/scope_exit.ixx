export module deckard.scope_exit;
import std;

export namespace deckard
{
	template<typename EF>
	class scope_exit
	{
	private:
		template<typename Fn>
		static inline constexpr bool noexcept_ctor = std::is_nothrow_constructible_v<EF, Fn> || std::is_nothrow_constructible_v<EF, Fn &>;

		static inline constexpr bool noexcept_move = std::is_nothrow_move_constructible_v<EF> || std::is_nothrow_copy_constructible_v<EF>;

	public:
		template<typename Fn>
		explicit scope_exit(Fn &&fn) noexcept(noexcept_ctor<Fn>)
			: exitfun(std::forward<Fn>(fn))
		{
		}

		scope_exit(scope_exit &&other) noexcept(noexcept_move)
			: active(std::exchange(other.active, false))
			, exitfun(std::move_if_noexcept(other.exitfun))
		{
		}

		scope_exit(const scope_exit &)            = delete;
		scope_exit &operator=(scope_exit &&)      = delete;
		scope_exit &operator=(const scope_exit &) = delete;

		~scope_exit() noexcept
		{
			if (active)
				exitfun();
		}

		void release() noexcept { active = false; }

	private:
		bool active = true;
		EF   exitfun;
	};

	template<class EF>
	scope_exit(EF) -> scope_exit<EF>;

} // namespace deckard
