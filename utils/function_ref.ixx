export module deckard.function_ref;

import std;

namespace deckard
{
	// https://github.com/TartanLlama/function_ref/
	namespace detail
	{
		namespace fnref
		{
			template<class T>
			using remove_const_t = typename std::remove_const<T>::type;
			template<class T>
			using remove_reference_t = typename std::remove_reference<T>::type;
			template<class T>
			using decay_t = typename std::decay<T>::type;
			template<bool E, class T = void>
			using enable_if_t = typename std::enable_if<E, T>::type;
			template<bool B, class T, class F>
			using conditional_t = typename std::conditional<B, T, F>::type;

			template<typename Fn, typename... Args, typename = enable_if_t<std::is_member_pointer<decay_t<Fn>>::value>, int = 0>
			constexpr auto invoke(Fn&& f, Args&&... args) noexcept(noexcept(std::mem_fn(f)(std::forward<Args>(args)...)))
			  -> decltype(std::mem_fn(f)(std::forward<Args>(args)...))
			{
				return std::mem_fn(f)(std::forward<Args>(args)...);
			}

			template<typename Fn, typename... Args, typename = enable_if_t<!std::is_member_pointer<decay_t<Fn>>{}>>
			constexpr auto invoke(Fn&& f, Args&&... args) noexcept(noexcept(std::forward<Fn>(f)(std::forward<Args>(args)...)))
			  -> decltype(std::forward<Fn>(f)(std::forward<Args>(args)...))
			{
				return std::forward<Fn>(f)(std::forward<Args>(args)...);
			}

			template<class F, class, class... Us>
			struct invoke_result_impl;

			template<class F, class... Us>
			struct invoke_result_impl<F, decltype(detail::fnref::invoke(std::declval<F>(), std::declval<Us>()...), void()), Us...>
			{
				using type = decltype(detail::fnref::invoke(std::declval<F>(), std::declval<Us>()...));
			};

			template<class F, class... Us>
			using invoke_result = invoke_result_impl<F, void, Us...>;

			template<class F, class... Us>
			using invoke_result_t = typename invoke_result<F, Us...>::type;

			template<class, class R, class F, class... Args>
			struct is_invocable_r_impl : std::false_type
			{
			};

			template<class R, class F, class... Args>
			struct is_invocable_r_impl<typename std::is_convertible<invoke_result_t<F, Args...>, R>::type, R, F, Args...> : std::true_type
			{
			};

			template<class R, class F, class... Args>
			using is_invocable_r = is_invocable_r_impl<std::true_type, R, F, Args...>;

		} // namespace fnref
	} // namespace detail

	export template<class F>
	class function_ref;

	export template<class R, class... Args>
	class function_ref<R(Args...)>
	{
	public:
		constexpr function_ref() noexcept = delete;

		constexpr function_ref(const function_ref<R(Args...)>& rhs) noexcept = default;

		template<typename F, detail::fnref::enable_if_t<!std::is_same<detail::fnref::decay_t<F>, function_ref>::value &&
														detail::fnref::is_invocable_r<R, F&&, Args...>::value>* = nullptr>
		constexpr function_ref(F&& f) noexcept
			: obj_(const_cast<void*>(reinterpret_cast<const void*>(std::addressof(f))))
		{
			callback_ = [](void* obj, Args... args) -> R
			{ return detail::fnref::invoke(*reinterpret_cast<typename std::add_pointer<F>::type>(obj), std::forward<Args>(args)...); };
		}

		constexpr function_ref<R(Args...)>& operator=(const function_ref<R(Args...)>& rhs) noexcept = default;

		template<typename F, detail::fnref::enable_if_t<detail::fnref::is_invocable_r<R, F&&, Args...>::value>* = nullptr>
		constexpr function_ref<R(Args...)>& operator=(F&& f) noexcept
		{
			obj_      = reinterpret_cast<void*>(std::addressof(f));
			callback_ = [](void* obj, Args... args)
			{ return detail::fnref::invoke(*reinterpret_cast<typename std::add_pointer<F>::type>(obj), std::forward<Args>(args)...); };

			return *this;
		}

		constexpr void swap(function_ref<R(Args...)>& rhs) noexcept
		{
			std::swap(obj_, rhs.obj_);
			std::swap(callback_, rhs.callback_);
		}

		R operator()(Args... args) const { return callback_(obj_, std::forward<Args>(args)...); }

	private:
		void* obj_                     = nullptr;
		R (*callback_)(void*, Args...) = nullptr;
	};

	export template<typename R, typename... Args>
	constexpr void swap(function_ref<R(Args...)>& lhs, function_ref<R(Args...)>& rhs) noexcept
	{
		lhs.swap(rhs);
	}

	export template<typename R, typename... Args>
	function_ref(R (*)(Args...)) -> function_ref<R(Args...)>;


	static_assert(sizeof(function_ref<double(double)>) == 16);


	// ################################


} // namespace deckard
