export module deckard.utf8:string;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;

namespace deckard::utf8
{
	namespace nt
	{
		using u8char_t                     = unsigned char;
		using u32char_t                    = char32_t;
		constexpr u32char_t CODE_POINT_MAX = 0x0010'ffffu;
		constexpr u32       SIZE_MAX       = std::numeric_limits<u32>::max();


		export constexpr char32_t REPLACEMENT_CHARACTER{0xFFFD};

		template<size_t SSO_SIZE>
		class basic_u8string
		{
		private:
			u8char_t* m_data{nullptr};
			u8char_t  m_ssodata[SSO_SIZE];
			u32       m_capacity{SSO_SIZE};
			u32       m_length{1};

			u8char_t* get_storage() const { return const_cast<u8char_t*>(m_capacity <= SSO_SIZE ? m_ssodata : m_data); }

			void grow_buffer(u32 amount)
			{
				u32 total_length = m_length + amount - 1;
				//
				if (m_capacity < total_length)
					grow(total_length - m_capacity);
			}

			void ensure_capacity(u32 capacity)
			{
				assert::check(capacity != SIZE_MAX);
				if (capacity > m_capacity && capacity > sso_capacity)
				{
					grow(capacity - m_capacity);
				}
			}

			void resize(u32 new_capacity)
			{
				if (new_capacity == m_capacity)
					return;

				if (not m_data)
				{
					m_data = reinterpret_cast<u8char_t*>(::malloc(new_capacity * sizeof(u8char_t)));
					if (m_data)
						m_capacity = new_capacity;

					return;
				}
				assert::check(new_capacity >= m_length);

				m_data = reinterpret_cast<u8char_t*>(::realloc(m_data, new_capacity * sizeof(u8char_t)));
				if (m_data)
				{
					m_capacity = new_capacity;
				}
			}

			struct iterator
			{
				using iterator_category = std::forward_iterator_tag;
				using value_type        = u32char_t;

				iterator(u8char_t* s, i32 idx)
					: ptr(s)
					, index(idx)
				{
					if (idx == 0 and s and s->has_data())
						current = s->next();
				}

				const value_type operator*() const { return current; }

				const iterator& operator++()
				{
					//	if (index >= 0 and str and str->has_next())
					//	{
					//		current = str->next();
					//		index += 1;
					//		return *this;
					//	}
					index = -1;
					return *this;
				}

				friend bool operator==(const iterator& a, const iterator& b) { return a.index == b.index; }

				u8char_t*  ptr{nullptr};
				u32        size{0};
				value_type current{REPLACEMENT_CHARACTER};
				i32        index{0};
			};

			void init_buffer(u32 capacity)
			{
				m_data     = reinterpret_cast<u8char_t*>(std::malloc(capacity * sizeof(u8char_t)));
				m_capacity = capacity;
			}

			void copy_other(const void* other, u32 otherLength)
			{
				assert::check(m_capacity >= otherLength);
				::memcpy_s(m_data, m_capacity * sizeof(u8char_t), other, otherLength);
				m_length = otherLength;
			}

			void copy_other_sso(const void* other, u32 otherLength)
			{
				assert::check(m_capacity >= otherLength);
				assert::check(sso_capacity >= otherLength);
				::memcpy_s(m_ssodata, m_capacity * sizeof(u8char_t), other, otherLength);
				m_length = otherLength;
			}

			void release()
			{
				//
				if (m_data)
				{
					::free(m_data);
					m_data = nullptr;
				}

				m_length   = 1;
				m_capacity = 1;
			}


		public:
			static constexpr u32 sso_capacity = SSO_SIZE;

			const u8char_t* get_raw() { return get_storage(); }

			void grow(u32 amount)
			{
				assert::check(amount != SIZE_MAX);

				u32 new_capacity = m_capacity;

				while (new_capacity < m_capacity + amount)
					new_capacity = new_capacity * 5 / 2 + 8;

				resize(new_capacity);
			}

			void append_other(const void* other, u32 otherLength)
			{
				assert::check(m_capacity >= m_length + otherLength);
				::memcpy_s(&m_data[--m_length], m_capacity * sizeof(u8char_t), other, otherLength);
				m_length += otherLength;
			}

			void append_other_sso(const void* other, u32 otherLength)
			{
				assert::check(m_capacity >= m_length + otherLength);
				assert::check(sso_capacity >= m_length + otherLength);

				::memcpy_s(&m_ssodata[--m_length], m_capacity * sizeof(u8char_t), other, otherLength);

				m_length += otherLength;
			}

			void append_sso_to_buffer()
			{
				if (m_capacity == sso_capacity && m_length > 1)
				{
					ensure_capacity(m_length + sso_capacity);
					u32 oldLength = m_length;
					m_length      = 1;
					append_other(m_ssodata, oldLength);
				}
			}

			void reset_to_sso()
			{
				//
				if (m_capacity > sso_capacity)
				{
					release();
					m_capacity = sso_capacity;
				}
			}

			void append(const char* other)
			{
				u32 length = as<u32>(::strlen(other));
				if (m_length + length < sso_capacity)
				{
					append_other_sso(other, length + 1);
				}
				else
				{
					append_sso_to_buffer();
					ensure_capacity(m_length + length);
					append_other(other, length + 1);
				}
			}

			void append(const basic_u8string& other)
			{
				if (m_length + other.m_length - 1 < sso_capacity)
				{
					append_other_sso(other.get_storage(), other.m_length);
				}
				else
				{
					append_sso_to_buffer();
					ensure_capacity(m_length + other.m_length);
					append_other(other.m_data, other.m_length);
				}
			}

			iterator begin() { return iterator(this, 0); }

			iterator end() { return iterator(this, -1); }

			basic_u8string& operator=(std::string_view input)
			{
				u32 length = as<u32>(input.size() + 1);
				if (length > sso_capacity)
				{
					ensure_capacity(length);
					copy_other(input.data(), length);
				}
				else
				{
					reset_to_sso();
					copy_other_sso(input.data(), length);
				}
				return *this;
			}

			basic_u8string& operator=(const basic_u8string& other)
			{
				if (other.m_length > sso_capacity)
				{
					ensure_capacity(other.m_length);
					copy_other(other.m_data, other.m_length);
				}
				else
				{
					reset_to_sso();
					copy_other_sso(other.get_storage());
				}
				return *this;
			}

			basic_u8string& operator=(basic_u8string&& other)
			{
				release();
				move_other(std::move(other));
				return *this;
			}

			basic_u8string() = default;

			basic_u8string(std::string_view input)
			{
				u32 length = as<u32>(input.size() + 1);
				if (length > sso_capacity)
				{
					init_buffer(length);
					copy_other(input.data(), length);
				}
				else
				{
					copy_other_sso(input.data(), length);
				}
			}

			basic_u8string(const basic_u8string& other)
			{
				if (other.m_length > sso_capacity)
				{
					init_buffer(other.m_length);
					copy_other(other.m_data, other.m_length);
				}
				else
				{
					copy_other_sso(other.get_storage(), other.m_length);
				}
			}

			basic_u8string(basic_u8string&& other) { move_other(std::move(other)); }

			~basic_u8string() { release(); }
		};

		export using u8string = basic_u8string<16>;

		static_assert(32 == sizeof(u8string), "u8string should be 32-bytes");

	} // namespace nt

	using u8char_t                     = unsigned char;
	using u32char_t                    = char32_t;
	constexpr u32char_t CODE_POINT_MAX = 0x0010'ffffu;

	constexpr u32 SIZE_MAX = std::numeric_limits<u32>::max();

	enum UTF8Error
	{
		UTF8Error_None,
		UTF8Error_NotEnoughRoom,
		UTF8Error_InvalidLead,
		UTF8Error_IncompleteSequence,
		UTF8Error_OverlongSequence,
		UTF8Error_InvalidCodePoint,
	};

	u8char_t mask(u8char_t c) { return (0xFF & c); }

	bool is_trail(u8char_t c) { return ((mask(c) >> 6) == 0x2); }

	bool is_code_point_valid(u32char_t c) { return (c <= CODE_POINT_MAX); }

	u8 sequence_length(u8char_t c)
	{
		u8char_t lead = mask(c);

		if (lead < 0x80)
		{
			return 1;
		}
		else if ((lead >> 5) == 0x6)
		{
			return 2;
		}
		else if ((lead >> 4) == 0xE)
		{
			return 3;
		}
		else if ((lead >> 3) == 0x1E)
		{
			return 4;
		}

		return 0;
	}

	bool is_overlong_sequence(u32char_t c, u8 length)
	{
		if (c < 0x80)
		{
			if (length != 1)
			{
				return true;
			}
		}
		else if (c < 0x800)
		{
			if (length != 2)
			{
				return true;
			}
		}
		else if (c < 0x10000)
		{
			if (length != 3)
			{
				return true;
			}
		}

		return false;
	}

	UTF8Error get_sequence1(u8char_t*& itr, u8char_t* end, u32char_t& c)
	{
		if (itr == end)
		{
			return UTF8Error_NotEnoughRoom;
		}

		c = mask(*itr);

		return UTF8Error_None;
	}

	UTF8Error increase_safely(u8char_t*& itr, u8char_t* end)
	{
		if (++itr == end)
		{
			return UTF8Error_NotEnoughRoom;
		}

		if (!is_trail(*itr))
		{
			return UTF8Error_IncompleteSequence;
		}

		return UTF8Error_None;
	}

#define increase_safely_and_return_on_error(itr, end)                                                                                      \
	{                                                                                                                                      \
		UTF8Error result = increase_safely(itr, end);                                                                                      \
		if (result != UTF8Error_None)                                                                                                      \
		{                                                                                                                                  \
			return result;                                                                                                                 \
		}                                                                                                                                  \
	}

	UTF8Error get_sequence2(u8char_t*& itr, u8char_t* end, u32char_t& c)
	{
		if (itr == end)
		{
			return UTF8Error_NotEnoughRoom;
		}

		c = mask(*itr);

		increase_safely_and_return_on_error(itr, end);

		c = ((c << 6) & 0x7FF) + ((*itr) & 0x3F);

		return UTF8Error_None;
	}

	UTF8Error get_sequence3(u8char_t*& itr, u8char_t* end, u32char_t& c)
	{
		if (itr == end)
		{
			return UTF8Error_NotEnoughRoom;
		}

		c = mask(*itr);

		increase_safely_and_return_on_error(itr, end);

		c = ((c << 12) & 0xFFFF) + ((mask(*itr) << 6) & 0xFFF);

		increase_safely_and_return_on_error(itr, end);

		c += (*itr) & 0x3F;

		return UTF8Error_None;
	}

	UTF8Error get_sequence4(u8char_t*& itr, u8char_t* end, u32char_t& c)
	{
		if (itr == end)
		{
			return UTF8Error_NotEnoughRoom;
		}

		c = mask(*itr);

		increase_safely_and_return_on_error(itr, end);

		c = ((c << 18) & 0x1F'FFFF) + ((mask(*itr) << 12) & 0x3FFFF);

		increase_safely_and_return_on_error(itr, end);

		c += (mask(*itr) << 6) & 0xFFF;

		increase_safely_and_return_on_error(itr, end);

		c += (*itr) & 0x3F;

		return UTF8Error_None;
	}

	UTF8Error validate_next(u8char_t*& itr, u8char_t* end, u32char_t& c)
	{
		if (itr == end)
		{
			return UTF8Error_NotEnoughRoom;
		}

		u8char_t* original = itr;
		u32char_t cp       = 0;

		const u8 length = sequence_length(*itr);

		UTF8Error result = UTF8Error_None;

		switch (length)
		{
			case 0: return UTF8Error_InvalidLead;
			case 1:
			{
				result = get_sequence1(itr, end, cp);
			}
			break;
			case 2:
			{
				result = get_sequence2(itr, end, cp);
			}
			break;
			case 3:
			{
				result = get_sequence3(itr, end, cp);
			}
			break;
			case 4:
			{
				result = get_sequence4(itr, end, cp);
			}
			break;
		}

		if (result == UTF8Error_None)
		{
			if (is_code_point_valid(cp))
			{
				if (!is_overlong_sequence(cp, length))
				{
					c = cp;
					++itr;
					return UTF8Error_None;
				}

				result = UTF8Error_OverlongSequence;
			}
			else
			{
				result = UTF8Error_InvalidCodePoint;
			}
		}

		itr = original;
		return result;
	}

	UTF8Error validate_next(u8char_t*& itr, u8char_t* end)
	{
		u32char_t ignored;
		return validate_next(itr, end, ignored);
	}

	u32char_t next(u8char_t*& itr, u8char_t* end)
	{
		u32char_t result = 0;
		validate_next(itr, end, result);
		return result;
	}

	u32char_t peek_next(u8char_t* itr, u8char_t* end) { return next(itr, end); }

	u32char_t previous(u8char_t*& itr, u8char_t* start)
	{
		if (itr == start)
		{
			return 0;
		}

		u8char_t* end = itr;

		while (is_trail(*(--itr)))
		{
			if (itr == start)
			{
				return 0;
			}
		}

		return peek_next(itr, end);
	}

	u32 distance(u8char_t* first, u8char_t* last)
	{
		u32 result{0};

		for (result = 0; first < last; ++result)
		{
			if (next(first, last) == 0)
			{
				return 0;
			}
		}

		return result;
	}

	// #################################
	class basic_utf8string_iterator
	{
	private:
		u8char_t* _begin;
		u8char_t* _end;
		u8char_t* _current;

	public:
		basic_utf8string_iterator(u8char_t* begin, u8char_t* end)
			: _begin(begin)
			, _end(end)
		{
			assert::check(begin);
			assert::check(end);
			_current = begin;
		}

		~basic_utf8string_iterator()                                           = default;
		basic_utf8string_iterator(const basic_utf8string_iterator&)            = default;
		basic_utf8string_iterator(basic_utf8string_iterator&&)                 = default;
		basic_utf8string_iterator& operator=(const basic_utf8string_iterator&) = default;
		basic_utf8string_iterator& operator=(basic_utf8string_iterator&&)      = default;

		bool operator==(const basic_utf8string_iterator& other) { return (_current == other._current); }

		bool operator!=(const basic_utf8string_iterator& other) { return !(operator==(other)); }

		basic_utf8string_iterator& operator++()
		{
			next(_current, _end);
			return *this;
		}

		basic_utf8string_iterator operator++(int)
		{
			basic_utf8string_iterator temp = *this;
			next(_current, _end);
			return temp;
		}

		basic_utf8string_iterator& operator--()
		{
			previous(_current, _begin);
			return *this;
		}

		basic_utf8string_iterator operator--(int)
		{
			basic_utf8string_iterator temp = *this;
			previous(_current, _begin);
			return temp;
		}

		u32char_t operator*() { return peek_next(_current, _end); }

		bool operator<(const basic_utf8string_iterator& other) { return (_current < other._current); }

		bool operator>(const basic_utf8string_iterator& other) { return (_current > other._current); }

		bool operator<=(const basic_utf8string_iterator& other) { return (_current <= other._current); }

		bool operator>=(const basic_utf8string_iterator& other) { return (_current >= other._current); }

		u8char_t* begin() { return _begin; }

		u8char_t* end() { return _end; }
	};

	// #################################

	template<size_t SSO_SIZE>
	class basic_u8string
	{
	private:
		u8char_t* m_data{nullptr};
		u8char_t  m_ssodata[SSO_SIZE];
		u32       m_capacity{SSO_SIZE};
		u32       m_length{1};

		u8char_t* get_storage() const { return const_cast<u8char_t*>(m_capacity == SSO_SIZE ? m_ssodata : m_data); }

		void grow_buffer(u32 amount)
		{
			u32 total_length = m_length + amount - 1;
			//
			if (m_capacity < total_length)
				grow(total_length - m_capacity);
		}

		void ensure_capacity(u32 capacity)
		{
			assert::check(capacity != SIZE_MAX);
			if (capacity > m_capacity && capacity > sso_capacity)
			{
				grow(capacity - m_capacity);
			}
		}

		void resize(u32 new_capacity)
		{
			if (new_capacity == m_capacity)
				return;

			if (not m_data)
			{
				m_data = reinterpret_cast<u8char_t*>(::malloc(new_capacity * sizeof(u8char_t)));
				if (m_data)
					m_capacity = new_capacity;

				return;
			}
			assert::check(new_capacity >= m_length);

			m_data = reinterpret_cast<u8char_t*>(::realloc(m_data, new_capacity * sizeof(u8char_t)));
			if (m_data)
			{
				m_capacity = new_capacity;
			}
		}

		void release()
		{
			//
			if (m_data)
			{
				::free(m_data);
				m_data = nullptr;
			}

			m_length   = 1;
			m_capacity = 1;
		}

		void init_buffer(u32 capacity)
		{
			m_data = reinterpret_cast<u8char_t*>(std::malloc(capacity * sizeof(u8char_t)));
			if (m_data)
				m_capacity = capacity;
		}

		void copy_other(const void* other, u32 otherLength)
		{
			assert::check(m_capacity >= otherLength);
			::memcpy_s(m_data, m_capacity * sizeof(u8char_t), other, otherLength);
			m_length = otherLength;
		}

		void copy_other_sso(const void* other, u32 otherLength)
		{
			assert::check(m_capacity >= otherLength);
			assert::check(sso_capacity >= otherLength);
			::memcpy_s(m_ssodata, m_capacity * sizeof(u8char_t), other, otherLength);
			m_length = otherLength;
		}

		void move_other(basic_u8string&& other)
		{
			m_capacity       = other.m_capacity;
			m_length         = other.m_length;
			m_data           = other.m_data;
			m_ssodata        = other.m_ssodata;
			other.m_capacity = 0;
			other.m_length   = 0;
			other.m_data     = nullptr;
		}

		void append_other(const void* other, u32 otherLength)
		{
			assert::check(m_capacity >= m_length + otherLength);
			::memcpy_s(&m_data[--m_length], m_capacity * sizeof(u8char_t), other, otherLength);
			m_length += otherLength;
		}

		void append_other_sso(const void* other, u32 otherLength)
		{
			assert::check(m_capacity >= m_length + otherLength);
			assert::check(sso_capacity >= m_length + otherLength);

			::memcpy_s(&m_ssodata[--m_length], m_capacity * sizeof(u8char_t), other, otherLength);

			m_length += otherLength;
		}

		void append_sso_to_buffer()
		{
			if (m_capacity == sso_capacity && m_length > 1)
			{
				ensure_capacity(m_length + sso_capacity);
				u32 oldLength = m_length;
				m_length      = 1;
				append_other(m_ssodata, oldLength);
			}
		}

		void reset_to_sso()
		{
			//
			if (m_capacity > sso_capacity)
			{
				release();
				m_capacity = sso_capacity;
			}
		}

	public:
		static constexpr u32 sso_capacity = SSO_SIZE;

		const u8char_t* get_raw() { return get_storage(); }

		void grow(u32 amount)
		{
			assert::check(amount != SIZE_MAX);

			u32 new_capacity = m_capacity;

			while (new_capacity < m_capacity + amount)
				new_capacity = new_capacity * 5 / 2 + 8;

			resize(new_capacity);
		}

		basic_utf8string_iterator begin() const
		{
			u8char_t* data = get_storage();
			return basic_utf8string_iterator(data, data + size_in_bytes());
		}

		basic_utf8string_iterator end() const
		{
			u8char_t* end = get_storage() + size_in_bytes();
			return basic_utf8string_iterator(end, end);
		}

		u32 size_in_bytes() const { return m_length - 1; }

		u32 capacity() const { return m_capacity - 1; }

		u32 count() const
		{
			u8char_t* data = get_storage();
			return distance(data, &data[m_length - 1]);
		}

		u32 length() const { return count(); }

		u32 size() const { return count(); }

		void clear() { m_length = 1; }

		void push(u32char_t c)
		{
			assert::check(c);

			--m_length;

			u32       seqlen = sequence_length(c);
			u8char_t* data   = get_storage();
			ensure_capacity(m_length + seqlen + 1);

			append(c, &data[m_length]);
			m_length += seqlen;
			data[m_length] = '\0';
			++m_length;
		}

		void push(u8char_t c) { push(static_cast<u32char_t>(c)); }

		void push(char c) { push(static_cast<u32char_t>(c)); }

		u32char_t pop()
		{
			// remove the null terminator
			if (m_length == 1)
			{
				return 0;
			}

			--m_length;

			u32char_t result = 0;
			u8char_t* data   = get_storage();
			u8char_t* pos    = &data[m_length];
			result           = previous(pos, data);
			if (result != 0)
			{
				data[m_length - 1] = '\0';
			}
			return result;
		}

		void append(const char* other)
		{
			u32 length = as<u32>(::strlen(other));
			if (m_length + length < sso_capacity)
			{
				append_other_sso(other, length + 1);
			}
			else
			{
				append_sso_to_buffer();
				ensure_capacity(m_length + length);
				append_other(other, length + 1);
			}
		}

		void append(const basic_u8string& other)
		{
			if (m_length + other.m_length - 1 < sso_capacity)
			{
				append_other_sso(other.get_storage(), other.m_length);
			}
			else
			{
				append_sso_to_buffer();
				ensure_capacity(m_length + other.m_length);
				append_other(other.m_data, other.m_length);
			}
		}

		u8char_t octet_at(u32 index) const
		{
			assert::check(index < m_length);
			return m_data[index];
		}

		u32char_t at(size_t index) const
		{
			assert::check(index < m_length);
			basic_utf8string_iterator itr = begin();
			basic_utf8string_iterator end = this->end();

			for (u32 i = 0; i < index; ++i)
			{
				if (itr == end)
				{
					return 0;
				}

				++itr;
			}

			return *itr;
		}

		basic_utf8string_iterator find(const char* substring) const
		{
			assert::check(substring);
			if (m_length <= 1)
				return end();

			u32 substringLength = ::strlen(substring);
			if (substringLength == 0)
				return end();
			if (substringLength >= m_length)
				return end();

			u32      currentSubstringIndex = 0;
			char     currentSubstringChar  = substring[currentSubstringIndex];
			u32      currentCharIndex      = 0;
			u8char_t currentChar           = get_storage()[currentCharIndex];
			u32      foundIndex            = 0;
			while (currentChar != '\0')
			{
				if (currentSubstringChar == currentChar)
				{
					if (foundIndex == 0 && currentSubstringIndex == 0)
					{
						foundIndex = currentCharIndex;
					}
					currentSubstringChar = substring[++currentSubstringIndex];
					if (currentSubstringIndex == substringLength)
					{
						break;
					}
				}
				else
				{
					currentSubstringIndex = foundIndex = 0;
					currentSubstringChar               = substring[currentSubstringIndex];
				}
				++currentCharIndex;
				currentChar = get_storage()[currentCharIndex];
			}

			if (currentSubstringIndex == substringLength)
			{
				u8char_t* begin = get_storage() + foundIndex;
				u8char_t* end   = begin + currentSubstringIndex;
				return basic_utf8string_iterator(begin, end);
			}

			return end();
		}

		basic_utf8string_iterator find(const basic_u8string& other) const
		{
			if (m_length <= 1 || other.m_length <= 1)
				return end();
			return find(reinterpret_cast<const char*>(other.get_storage()));
		}

		u32char_t operator[](u32 index) const { return at(index); }

		bool operator==(const char* other) const
		{
			u32 length = std::strlen(other) + 1;
			if (length != m_length)
			{
				return false;
			}

			int result = 0;
			result     = ::memcmp(get_storage(), other, m_length * sizeof(u8char_t));

			// a result of 0 means identical
			return result == 0;
		}

		bool operator!=(const char* other) const { return !operator==(other); }

		bool operator==(const basic_u8string& other) const
		{
			if (m_length != other.m_length)
			{
				return false;
			}

			int result = ::memcmp(get_storage(), other.get_storage(), m_length * sizeof(u8char_t));

			// a result of 0 means identical
			return result == 0;
		}

		bool operator!=(const basic_u8string& other) const { return !operator==(other); }

		basic_u8string& operator+=(u32char_t c)
		{
			push(c);
			return *this;
		}

		basic_u8string& operator+=(u8char_t c)
		{
			push(static_cast<u32char_t>(c));
			return *this;
		}

		basic_u8string& operator+=(char c)
		{
			push(static_cast<u32char_t>(c));
			return *this;
		}

		basic_u8string& operator+=(const char* other)
		{
			assert::check(other);
			append(other);
			return *this;
		}

		basic_u8string& operator+=(const basic_u8string& other)
		{
			assert::check(other.m_data);
			append(other);
			return *this;
		}

		basic_u8string& operator=(const char* other)
		{
			u32 length = ::strlen(other) + 1;
			if (length > sso_capacity)
			{
				ensure_capacity(length);
				copy_other(other, length);
			}
			else
			{
				reset_to_sso();
				copy_other_sso(other, length);
			}
			return *this;
		}

		basic_u8string& operator=(const basic_u8string& other)
		{
			if (other._length > sso_capacity)
			{
				ensure_capacity(other._length);
				copy_other(other._data, other._length);
			}
			else
			{
				reset_to_sso();
				copy_other_sso(other.get_storage());
			}
			return *this;
		}

		basic_u8string& operator=(basic_u8string&& other)
		{
			release();
			move_other(std::move(other));
			return *this;
		}

		basic_u8string() = default;

		basic_u8string(const char* other)
		{
			u32 length = as<u32>(std::strlen(other) + 1);
			if (length > sso_capacity)
			{
				init_buffer(length);
				copy_other(other, length);
			}
			else
			{
				copy_other_sso(other, length);
			}
		}

		basic_u8string(const basic_u8string& other)
		{
			if (other._length > sso_capacity)
			{
				init_buffer(other._length);
				copy_other(other._data, other._length);
			}
			else
			{
				copy_other_sso(other.get_storage(), other._length);
			}
		}

		basic_u8string(basic_u8string&& other) { move_other(std::move(other)); }

		~basic_u8string() { release(); }
	};

	export using u8string = basic_u8string<16>;
	static_assert(32 == sizeof(u8string), "u8string should be 32-bytes");


} // namespace deckard::utf8
