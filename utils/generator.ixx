module;
#if __cpp_lib_generator && __has_include(<generator>)
#error "Generator is supported, delete this version"
#endif

export module deckard.generator;
import std;

export namespace deckard
{
	template<class T>
	class generator final
	{
		struct promise
		{
			using value_type     = std::remove_reference_t<T>;
			using reference_type = std::conditional_t<std::is_pointer_v<value_type>, value_type, value_type&>;
			using pointer_type   = std::conditional_t<std::is_pointer_v<value_type>, value_type, value_type*>;

			promise() = default;

			generator get_return_object() { return generator(std::coroutine_handle<promise>::from_promise(*this)); }

			std::suspend_always initial_suspend() const { return {}; }

			std::suspend_always final_suspend() const noexcept { return {}; }

			void return_void() const noexcept { return; }

			void unhandled_exception() noexcept { exception_ = std::current_exception(); }

			void rethrow_if_exception()
			{
				if (exception_)
				{
					std::rethrow_exception(exception_);
				}
			}

			std::suspend_always yield_value(reference_type v) noexcept
			{
				if constexpr (std::is_pointer_v<value_type>)
				{
					value_ = v;
				}
				else
				{
					value_ = std::addressof(v);
				}
				return {};
			}

			std::exception_ptr exception_;
			pointer_type       value_;
		};

	public:
		using promise_type = promise;

		class sentinel
		{
		};

		class iterator
		{
			using handle_type = std::coroutine_handle<promise_type>;

		public:
			using value_type      = typename promise_type::value_type;
			using reference_type  = typename promise_type::reference_type;
			using pointer_type    = typename promise_type::pointer_type;
			using difference_type = std::ptrdiff_t;

			iterator() = default;

			~iterator()
			{
				if (handle_)
					handle_.destroy();
			}

			// Non-copyable because coroutine handles point to a unique resource
			iterator(iterator const&) = delete;

			iterator(iterator&& rhs) noexcept
				: handle_(std::exchange(rhs.handle_, nullptr))
			{
			}

			iterator& operator=(iterator const&) = delete;

			iterator& operator=(iterator&& rhs) noexcept
			{
				handle_ = std::exchange(rhs.handle_, nullptr);
				return *this;
			}

			friend bool operator==(iterator const& it, sentinel) noexcept { return (!it.handle_ || it.handle_.done()); }

			iterator& operator++()
			{
				handle_.resume();
				if (handle_.done())
				{
					handle_.promise().rethrow_if_exception();
				}
				return *this;
			}

			void operator++(int) { (void)this->operator++(); }

			reference_type operator*() const noexcept(noexcept(std::is_nothrow_copy_constructible_v<reference_type>))
			{
				if constexpr (std::is_pointer_v<value_type>)
					return handle_.promise().value_;
				else
					return *handle_.promise().value_;
			}

		private:
			friend class generator;

			iterator(handle_type handle)
				: handle_(handle)
			{
			}

			handle_type handle_;
		};

		using handle_type = std::coroutine_handle<promise_type>;

		generator() noexcept = default;

		~generator()
		{
			if (handle_)
				handle_.destroy();
		}

		generator(generator const&) = delete;

		generator(generator&& rhs) noexcept
			: handle_(std::exchange(rhs.handle_, nullptr))
		{
		}

		generator& operator=(generator const&) = delete;

		generator& operator=(generator&& rhs) noexcept
		{
			swap(rhs);
			return *this;
		}

		iterator begin()
		{
			handle_.resume();
			if (handle_.done())
			{
				handle_.promise().rethrow_if_exception();
			}
			return {std::exchange(handle_, nullptr)};
		}

		sentinel end() const noexcept { return {}; }

		void swap(generator& other) noexcept { std::swap(handle_, other.handle_); }

	private:
		friend class iterator;

		explicit generator(handle_type handle) noexcept
			: handle_(handle)
		{
		}

		handle_type handle_ = nullptr;
	};
} // namespace deckard

export template<class T>
inline constexpr bool std::ranges::enable_view<deckard::generator<T>> = true;
