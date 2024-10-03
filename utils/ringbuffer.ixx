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

	private:
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

		array_type      m_array{};
		size_type       m_head{1};
		size_type       m_tail{0};
		size_type       m_content_size{0};
		const size_type m_array_size{0};
	};


} // namespace deckard
