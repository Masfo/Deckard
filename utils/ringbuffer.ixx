export module deckard.ringbuffer;


import std;
import deckard.types;
import deckard.assert;

namespace deckard
{

	struct alignas(std::hardware_destructive_interference_size) SlotFlag
	{
		enum class State : u8
		{
			Empty   = 0,
			Writing = 1,
			Ready   = 2
		};
		std::atomic<State> flag{State::Empty};
	};

	export template<typename T, std::size_t N>
	class atomic_ringbuffer
	{
	private:
		using index_type = u64;
		static_assert((N & (N - 1)) == 0, "Must be a power of two");
		static_assert(std::is_nothrow_move_constructible_v<T> || std::is_nothrow_copy_constructible_v<T>,
					  "T must be move‑ or copy‑constructible without throwing");

		alignas(std::hardware_destructive_interference_size) std::atomic<index_type> head{0};
		alignas(std::hardware_destructive_interference_size) std::atomic<index_type> tail{0};

		const index_type mask{N - 1};

		std::vector<std::aligned_storage_t<sizeof(T), alignof(T)>> slots;
		std::vector<SlotFlag>                                      flags;


	public:
		constexpr atomic_ringbuffer() noexcept
			: head{0}
			, tail{0}
			, mask{N - 1}
			, slots(N)
			, flags(N)
		{
		}
		bool try_push(const T& value) noexcept
		{
			index_type pos   = tail.fetch_add(1, std::memory_order_relaxed);
			index_type index = pos & mask;

			index_type current_head = head.load(std::memory_order_acquire);
			if (pos - current_head >= N)
			{
				tail.fetch_sub(1, std::memory_order_relaxed);
				return false;
			}

			flags[index].flag.store(SlotFlag::State::Writing, std::memory_order_relaxed);

			new (&slots[index]) T(value);

			flags[index].flag.store(SlotFlag::State::Ready, std::memory_order_release);
			return true;
		}

		bool try_push(T&& value) noexcept
		{
			index_type pos   = tail.fetch_add(1, std::memory_order_relaxed);
			index_type index = pos & mask;

			index_type current_head = head.load(std::memory_order_acquire);
			if (pos - current_head >= N)
			{
				tail.fetch_sub(1, std::memory_order_relaxed);
				return false;
			}
			flags[index].flag.store(SlotFlag::State::Writing, std::memory_order_relaxed);

			new (&slots[index]) T(std::move(value));

			flags[index].flag.store(SlotFlag::State::Ready, std::memory_order_release);
			return true;
		}

		std::optional<T> try_pop() noexcept
		{
			index_type pos   = head.fetch_add(1, std::memory_order_relaxed);
			index_type index = pos & mask;

			auto state = flags[index].flag.load(std::memory_order_acquire);
			if (state != SlotFlag::State::Ready)
				return std::nullopt;

			T* elem  = reinterpret_cast<T*>(&slots[index]);
			T  value = std::move(*elem);
			elem->~T();

			flags[index].flag.store(SlotFlag::State::Empty, std::memory_order_release);

			head.store(pos + 1, std::memory_order_relaxed);
			return value;
		}

		bool empty() const noexcept { return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire); }

		bool full() const noexcept { return (tail.load(std::memory_order_acquire) - head.load(std::memory_order_acquire)) >= N; }

		std::size_t size() const noexcept
		{
			return static_cast<std::size_t>(tail.load(std::memory_order_acquire) - head.load(std::memory_order_acquire));
		}

	};

	export template<typename T>
	class ringbuffer
	{
		using value_type      = T;
		using size_type       = u32;
		using reference       = T&;
		using const_reference = const T&;
		using array_type      = std::vector<value_type>;

	private:
		array_type      m_array{};
		size_type       m_head{1};
		size_type       m_tail{0};
		size_type       m_content_size{0};
		const size_type m_array_size{0};

		void increment_tail()
		{
			++m_tail;
			++m_content_size;

			if (m_tail == m_array_size)
				m_tail = 0;
		}

		void increment_head()
		{
			if (m_content_size == 0)
				return;
			++m_head;
			--m_content_size;

			if (m_head == m_array_size)
				m_head = 0;
		}


	public:
		ringbuffer(size_type size = 8)
			: m_array(size)
			, m_array_size(size)
			, m_head(1)
			, m_tail(0)
			, m_content_size(0)
		{
			assert::check(m_array_size > 1, "size must be greater than 1");
		}

		[[nodiscard]] reference front() { return at(0); }

		[[nodiscard]] reference top() { return front(); }

		[[nodiscard]] reference back() { return at(size() - 1); }

		[[nodiscard]] const_reference front() const { return at(0); }

		[[nodiscard]] const_reference back() const { return at(size() - 1); }

		void clear()
		{
			m_head = 1;
			m_tail = m_content_size = 0;
		}

		void push_back(const value_type& item)
		{
			increment_tail();
			if (m_content_size > m_array_size)
				increment_head();
			m_array[m_tail] = item;
		}

		void push(const value_type& item) { push_back(item); }

		[[nodiscard]] reference pop_front()
		{
			increment_head();
			return m_array[m_tail];
		}

		[[nodiscard]] reference pop() { return pop_front(); }

		[[nodiscard]] const_reference pop_front() const
		{
			increment_head();
			return m_array[m_tail];
		}

		[[nodiscard]] const_reference pop() const { return pop_front(); }

		[[nodiscard]] size_type size() const { return m_content_size; }

		[[nodiscard]] size_type capacity() const { return m_array_size; }

		[[nodiscard]] bool empty() const { return m_content_size == 0; }

		[[nodiscard]] bool full() const { return m_content_size == m_array_size; }

		[[nodiscard]] reference operator[](size_type index)
		{
			index += m_head;
			index %= m_array_size;
			return m_array[index];
		}

		[[nodiscard]] const_reference operator[](size_type index) const
		{
			const ringbuffer<value_type>& constMe = *this;
			return const_cast<reference>(constMe.operator[](index));
		}

		[[nodiscard]] reference at(size_type index)
		{
			assert::check(index < m_content_size, "Indexing out-of-bounds");
			return this->operator[](index);
		}

		[[nodiscard]] const_reference at(size_type index) const
		{
			assert::check(index < m_content_size, "Indexing out-of-bounds");
			return this->operator[](index);
		}

		//
		[[nodiscard]] reference last()
		{
			size_type pos = m_head - 1;
			if (pos > m_array_size)
				pos = m_array_size - 1;

			return m_array[pos];
		}

		[[nodiscard]] const_reference last() const
		{
			size_type pos = m_head - 1;
			if (pos > m_array_size)
				pos = m_array_size - 1;

			return m_array[pos];
		}

		// TODO: iterator instead of copying
		[[nodiscard]] std::vector<value_type> last(size_type count) const
		{
			assert::check(count - 1 < m_content_size, "Cant get more than max size");
			if (count - 1 > m_content_size)
				return {};

			std::vector<value_type> ret;
			ret.reserve(count);
			size_type pos = m_head - 1;
			while (count--)
			{
				if (pos > m_array_size)
					pos = m_array_size - 1;

				ret.push_back(m_array[pos]);
				pos--;
			}

			return ret;
		}
	};


} // namespace deckard
