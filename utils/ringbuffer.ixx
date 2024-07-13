export module deckard.ringbuffer;


import std;
import deckard.types;
import deckard.assert;

namespace deckard
{

	export template<typename T>
	class ringbuffer
	{
		using value_type      = T;
		using size_type       = u32;
		using reference       = T&;
		using const_reference = const T&;
		using array_type      = std::vector<value_type>;

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

		[[nodiscard]] reference front() noexcept { return at(0); }

		[[nodiscard]] reference top() noexcept { return front(); }

		[[nodiscard]] reference back() noexcept { return at(size() - 1); }

		[[nodiscard]] const_reference front() const noexcept { return at(0); }

		[[nodiscard]] const_reference back() const noexcept { return at(size() - 1); }

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

		[[nodiscard]] reference pop_front() noexcept
		{
			increment_head();
			return m_array[m_tail];
		}

		[[nodiscard]] reference pop() noexcept { return pop_front(); }

		[[nodiscard]] const_reference pop_front() const noexcept
		{
			increment_head();
			return m_array[m_tail];
		}

		[[nodiscard]] const_reference pop() const noexcept { return pop_front(); }

		[[nodiscard]] size_type size() const noexcept { return m_content_size; }

		[[nodiscard]] size_type capacity() const noexcept { return m_array_size; }

		[[nodiscard]] bool empty() const noexcept { return m_content_size == 0; }

		[[nodiscard]] bool full() const noexcept { return m_content_size == m_array_size; }

		[[nodiscard]] reference operator[](size_type index) noexcept
		{
			index += m_head;
			index %= m_array_size;
			return m_array[index];
		}

		[[nodiscard]] const_reference operator[](size_type index) const noexcept
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
		[[nodiscard]] reference last() noexcept
		{
			size_type pos = m_head - 1;
			if (pos > m_array_size)
				pos = m_array_size - 1;

			return m_array[pos];
		}

		[[nodiscard]] const_reference last() const noexcept
		{
			size_type pos = m_head - 1;
			if (pos > m_array_size)
				pos = m_array_size - 1;

			return m_array[pos];
		}

		[[nodiscard]] std::vector<value_type> last(size_type count) const noexcept
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
