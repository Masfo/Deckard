export module deckard.ringbuffer;


import std;
import deckard.types;
import deckard.assert;

namespace deckard
{
	template<typename T>
	class alignas(32) ringbuffer
	{
		using value_type      = T;
		using size_type       = u32;
		using reference       = T&;
		using array_type      = std::vector<value_type>;
		using const_reference = const T&;


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

		reference front() const noexcept { return m_array[m_head]; }

		reference top() const noexcept { return front(); }

		reference back() const noexcept { return m_array[m_tail]; }

		const_reference front() const noexcept { return m_array[m_head]; }

		const_reference back() const noexcept { return m_array[m_tail]; }

		void clear() noexcept
		{
			m_head = 1;
			m_tail = m_content_size = 0;
		}

		void push_back(const value_type& item) noexcept
		{
			increment_tail();
			if (m_content_size > m_array_size)
				increment_head();
			m_array[m_tail] = item;
		}

		void push(const value_type& item) noexcept { push_back(item); }

		void pop_front() noexcept { increment_head(); }

		void pop() noexcept { pop_front(); }

		[[nodiscard]] size_type size() const noexcept { return m_content_size; }

		[[nodiscard]] size_type capacity() const noexcept { return m_array_size; }

		[[nodiscard]] bool empty() const noexcept { return m_content_size == 0; }

		[[nodiscard]] bool full() const noexcept { return m_content_size == m_array_size; }

		reference operator[](size_type index) noexcept { }

		const_reference operator[](size_type index) const noexcept { }

		reference at(size_type index) { }

		const_reference at(size_type index) const { }


	private:
		void increment_tail() noexcept
		{
			++m_tail;
			++m_content_size;

			if (m_tail == m_array_size)
				m_tail = 0;
		}

		void increment_head() noexcept
		{
			if (m_content_size == 0)
				return;
			++m_head;
			--m_content_size;

			if (m_head == m_array_size)
				m_head = 0;
		}

		array_type      m_array{};
		size_type       m_head{1};
		size_type       m_tail{0};
		size_type       m_content_size{0};
		const size_type m_array_size{0};
	};

	static_assert(sizeof(ringbuffer<u32>) == 64);

	//
	template<typename T, size_t Size>
	class static_ringbuffer
	{
	public:
		using value_type = T;

	private:
		std::array<value_type, Size> m_data{};
	};

} // namespace deckard
