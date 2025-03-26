export module deckard.utf8:string;
import :codepoints;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.sbo;

namespace deckard::utf8
{
	/*
	 Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
	 https://bjoern.hoehrmann.de/utf-8/decoder/dfa/

	 Permission is hereby granted, free of charge, to any person obtaining a copy of this
	 software and associated documentation files (the "Software"), to deal in the Software
	 without restriction, including without limitation the rights to use, copy, modify,
	 merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
	 permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or
	substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
	BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
	DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	*/
	constexpr std::array<u8, 364> utf8_table{
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	  9,  9,  9,  9,  9,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	  2,  2,  2,  2,  2,  2,  2,  10, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,
	  8,  8,  8,  8,  8,  8,  8,  8,  0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 12, 0,  12, 12, 12, 12, 12, 0,  12, 0,  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12,
	  36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	};
	constexpr u32 UTF8_ACCEPT{0};
	constexpr u32 UTF8_REJECT{12};

	export constexpr char32 REPLACEMENT_CHARACTER{0xFFFD}; // U+FFFD 0xEF 0xBF 0xBD(239, 191, 189) REPLACEMENT CHARACTER

	struct utf8_decode_t
	{
		u32 state{};
		u32 codepoint{};
	};

	export u32 decode(utf8_decode_t& state, const u32 byte)
	{
		const u32 type     = utf8_table[byte];
		state.codepoint    = state.state ? (byte & 0x3fu) | (state.codepoint << 6) : (0xffu >> type) & byte;
		return state.state = utf8_table[256 + state.state + type];
	}

	export std::optional<i32> length(const std::span<u8> buffer)
	{
		if (buffer.empty())
			return {};

		const u8* ptr    = buffer.data();
		const u8* endptr = buffer.data() + buffer.size_bytes();
		i32       len{};
		bool      valid = true;

		for (; (ptr < endptr) && valid; len++)
		{
			valid = (*ptr & 0x80) == 0 or ((*ptr & 0xE0) == 0xC0 && (*(ptr + 1) & 0xC0) == 0x80) or
					((*ptr & 0xF0) == 0xE0 && (*(ptr + 1) & 0xC0) == 0x80 && (*(ptr + 2) & 0xC0) == 0x80) or
					((*ptr & 0xF8) == 0xF0 && (*(ptr + 1) & 0xC0) == 0x80 && (*(ptr + 2) & 0xC0) == 0x80 && (*(ptr + 3) & 0xC0) == 0x80);

			i32 v1 = ((*ptr & 0x80) >> 7) & ((*ptr & 0x40) >> 6);
			i32 v2 = (*ptr & 0x20) >> 5;
			i32 v3 = (*ptr & 0x10) >> 4;
			ptr += 1 + ((v1 << v2) | (v1 & v3));
		}
		if (valid)
			return len;

		return {};
	}

	export auto length(std::string_view buffer) { return length(std::span<u8>{as<u8*>(buffer.data()), as<u32>(buffer.length())}); }

	export auto length(const char* str, u32 len) { return length(std::span<u8>{as<u8*>(str), len}); }

	export bool is_valid(std::string_view buffer)
	{
		auto ret = length(std::span<u8>{as<u8*>(buffer.data()), as<u32>(buffer.length())});

		return ret ? true : false;
	}

	export bool is_valid(const char* str, u32 len)
	{
		auto ret = length(std::span<u8>{as<u8*>(str), len});

		return ret ? true : false;
	}

	export class string
	{
	private:
		sbo<32> buffer;

		using value_type      = sbo<32>;
		using pointer         = value_type*;
		using reference       = value_type&;
		using const_pointer   = const pointer;
		using const_reference = const reference;
		using difference_type = std::ptrdiff_t;
		using index_type      = u32;
		using char_type       = char32;

		class const_iterator;

		class iterator
		{
		public:
			friend class const_iterator;

			// using iterator_category = std::bidirectional_iterator_tag;
			using iterator_category = std::random_access_iterator_tag;

			using difference_type = std::ptrdiff_t;
			using value_type      = value_type;
			// TODO: random access iterator

		private:
			pointer         ptr;
			difference_type current_index;

			void advance_to_next_codepoint()
			{
				if (current_index >= as<difference_type>(ptr->size()))
					return;

				i64 next = current_index;
				next++;

				while (next < as<i64>(ptr->size()) and utf8::is_continuation_byte(ptr->at(next)))
					next++;

				while (next < as<i64>(ptr->size()))
				{
					u8 byte = ptr->at(next);

					if (not utf8::is_single_byte(byte))
						break;

					if (not utf8::is_start_of_codepoint(byte))
						break;

					next++;
				}
				current_index = next;
			}

			void reverse_to_last_codepoint()
			{
				if (current_index > 0)
				{
					current_index -= 1;

					while (current_index > 0 and utf8::is_continuation_byte(ptr->at(current_index)))
						current_index -= 1;

					if (current_index < 0)
						current_index = 0;
				}
			}

			index_type decode_current_codepoint() const
			{

				if (current_index >= as<difference_type>(ptr->size()))
					return REPLACEMENT_CHARACTER;

				auto       index     = current_index;
				u32        state     = 0;
				index_type codepoint = 0;

				for (; index < as<difference_type>(ptr->size()); index++)
				{
					u8 byte = ptr->at(index);

					const index_type type = utf8_table[byte];
					codepoint             = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
					state                 = utf8_table[256 + state + type];
					if (state == 0)
						return codepoint;
					else if (state == UTF8_REJECT)
						return REPLACEMENT_CHARACTER;
				}
				return REPLACEMENT_CHARACTER;
			}

			void set_codepoint(char_type new_codepoint)
			{
				if (current_index >= as<u32>(ptr->size()))
					return;

				std::vector<u8> encoded_bytes;
				if (new_codepoint <= 0x7F)
				{
					encoded_bytes.push_back(static_cast<u8>(new_codepoint));
				}
				else if (new_codepoint <= 0x7FF)
				{
					encoded_bytes.push_back(static_cast<u8>((new_codepoint >> 6) | 0xC0));
					encoded_bytes.push_back(static_cast<u8>((new_codepoint & 0x3F) | 0x80));
				}
				else if (new_codepoint <= 0xFFFF)
				{
					encoded_bytes.push_back(static_cast<u8>((new_codepoint >> 12) | 0xE0));
					encoded_bytes.push_back(static_cast<u8>(((new_codepoint >> 6) & 0x3F) | 0x80));
					encoded_bytes.push_back(static_cast<u8>((new_codepoint & 0x3F) | 0x80));
				}
				else if (new_codepoint <= 0x10'FFFF)
				{
					encoded_bytes.push_back(static_cast<u8>((new_codepoint >> 18) | 0xF0));
					encoded_bytes.push_back(static_cast<u8>(((new_codepoint >> 12) & 0x3F) | 0x80));
					encoded_bytes.push_back(static_cast<u8>(((new_codepoint >> 6) & 0x3F) | 0x80));
					encoded_bytes.push_back(static_cast<u8>((new_codepoint & 0x3F) | 0x80));
				}
				else
				{
					encoded_bytes.push_back(0xEF);
					encoded_bytes.push_back(0xBF);
					encoded_bytes.push_back(0xBD);
				}

				auto it = ptr->begin() + current_index;
				ptr->erase(it, it + width());
				ptr->insert(it, encoded_bytes);
			}

		public:
			iterator(const_iterator& ci)
				: ptr(ci.ptr)
			{
			}

			// iterator() = default;


			iterator(const pointer p)
				: iterator(p, 0)
			{
			}

			iterator(const pointer p, const difference_type v)
				: ptr(p)
				, current_index(v)
			{
			}

			index_type operator*() const { return codepoint(); }

			// pointer operator->() const { return ptr; }

			index_type codepoint() const { return decode_current_codepoint(); }

			u32 width() const { return utf8::codepoint_width(ptr->at(current_index)); }

			auto byteindex() const { return current_index; }

			iterator& operator++()
			{
				advance_to_next_codepoint();
				return *this;
			}

			iterator operator++(int)
			{
				iterator tmp = *this;
				advance_to_next_codepoint();
				return tmp;
			}

			iterator& operator--()
			{
				reverse_to_last_codepoint();
				return *this;
			}

			iterator operator--(int)
			{
				iterator tmp = *this;
				reverse_to_last_codepoint();
				return tmp;
			}

			iterator operator+=(int v)
			{
				while (--v)
					advance_to_next_codepoint();

				return *this;
			}

			iterator operator-=(int v)
			{
				while (--v)
					reverse_to_last_codepoint();

				return *this;
			}

			iterator operator+(difference_type n) const
			{
				iterator tmp = *this;
				while (n-- > 0)
					tmp.advance_to_next_codepoint();
				return tmp;
			}

			iterator operator-(difference_type n) const
			{
				iterator tmp = *this;
				while (n-- > 0)
					tmp.reverse_to_last_codepoint();
				return tmp;
			}

			difference_type operator-(const iterator& other) const
			{
				assert::check(ptr == other.ptr, "Not pointing to same data");


				difference_type count{0};
				auto            copy = other;
				while (copy != *this)
				{
					copy++;
					count++;
				}

				return count;
			}

			bool empty() const { return ptr->empty(); }

			reference operator[](difference_type n) const
			{
				assert::check(n >= 0, "Index is negative");
				assert::check(n < as<difference_type>(ptr->size()), "Index out-of-bounds");
				return ptr[n];
			}

			// auto operator<=>(const iterator&) const = default;
			bool operator<(const iterator& other) const { return current_index < other.current_index; }

			bool operator>(const iterator& other) const { return current_index > other.current_index; }

			bool operator==(const iterator& other) const
			{
				assert::check(ptr == other.ptr, "Not pointing to same data");

				return current_index == other.current_index;
			}
		};
#if 1
		class const_iterator
		{
		public:
			friend class iterator;

			// using iterator_category = std::bidirectional_iterator_tag;
			using iterator_category = std::random_access_iterator_tag;

			using difference_type = std::ptrdiff_t;
			using value_type      = value_type;

		private:
			pointer         ptr;
			difference_type current_index;

			void advance_to_next_codepoint()
			{
				if (current_index >= as<difference_type>(ptr->size()))
					return;

				i64 next = current_index;
				next++;

				while (next < as<i64>(ptr->size()) and utf8::is_continuation_byte(ptr->at(next)))
					next++;

				while (next < as<i64>(ptr->size()))
				{
					u8 byte = ptr->at(next);

					if (not utf8::is_single_byte(byte))
						break;

					if (not utf8::is_start_of_codepoint(byte))
						break;

					next++;
				}
				current_index = next;
			}

			void reverse_to_last_codepoint()
			{
				if (current_index > 0)
				{
					current_index -= 1;

					while (current_index > 0 and utf8::is_continuation_byte(ptr->at(current_index)))
						current_index -= 1;

					if (current_index < 0)
						current_index = 0;
				}
			}

			index_type decode_current_codepoint() const
			{

				if (current_index >= as<difference_type>(ptr->size()))
					return REPLACEMENT_CHARACTER;

				auto       index     = current_index;
				u32        state     = 0;
				index_type codepoint = 0;
				for (; index < as<difference_type>(ptr->size()); index++)
				{
					u8 byte = ptr->at(index);

					const index_type type = utf8_table[byte];
					codepoint             = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
					state                 = utf8_table[256 + state + type];
					if (state == 0)
						return codepoint;
					else if (state == UTF8_REJECT)
						return REPLACEMENT_CHARACTER;
				}
				return REPLACEMENT_CHARACTER;
			}

		public:
			friend class iterator;

			const_iterator() = default;

			const_iterator(iterator ci)
				: ptr(ci.ptr)
			{
			}

			const_iterator(const_pointer p)
				: ptr(p)
				, current_index(0)
			{
			}

			const_iterator(const_pointer p, difference_type v)
				: ptr(p)
				, current_index(v)
			{
			}

			u32 operator*() const { return codepoint(); }

			u32 codepoint() const { return decode_current_codepoint(); }

			u32 width() const { return utf8::codepoint_width(ptr->at(current_index)); }

			auto byteindex() const { return current_index; }

			// pointer operator->() const { return ptr; }

			const_iterator& operator++()
			{
				advance_to_next_codepoint();
				return *this;
			}

			const_iterator operator++(int)
			{
				const_iterator tmp = *this;
				advance_to_next_codepoint();
				return tmp;
			}

			const_iterator& operator--()
			{
				reverse_to_last_codepoint();
				return *this;
			}

			const_iterator operator--(int)
			{
				const_iterator tmp = *this;
				reverse_to_last_codepoint();
				return tmp;
			}

			const_iterator operator+=(int v)
			{
				while (--v)
					advance_to_next_codepoint();

				return *this;
			}

			const_iterator operator-=(int v)
			{
				while (--v)
					reverse_to_last_codepoint();

				return *this;
			}

			const_iterator operator+(difference_type n) const { return const_iterator(ptr + n); }

			const_iterator operator-(difference_type n) const { return const_iterator(ptr - n); }

			difference_type operator-(const const_iterator& other) const
			{
				assert::check(ptr == other.ptr, "Not pointing to same data");


				difference_type count{0};
				auto            copy = other;
				while (copy != *this)
				{
					copy++;
					count++;
				}

				return ptr->empty() ? 0 : count + 1;
			}

			reference operator[](difference_type n) const { return ptr[n]; }

			// auto operator<=>(const const_iterator&) const = default;

			bool operator<(const const_iterator& other) const { return current_index < other.current_index; }

			bool operator>(const const_iterator& other) const { return current_index > other.current_index; }

			bool operator==(const const_iterator& other) const
			{
				assert::check(ptr == other.ptr, "Pointers invalidated");
				// TODO: assert on pointer diff
				return ptr == other.ptr and current_index == other.current_index;
			}
		};
#endif

	public:
		string() = default;

		string(std::string_view input) { buffer.assign({as<u8*>(input.data()), input.size()}); }

		string& operator=(std::string_view input)
		{
			buffer.assign({as<u8*>(input.data()), input.size()});
			return *this;
		}

		[[nodiscard("use result of at method")]]
		char_type at(u32 index)
		{
			assert::check(index < size(), "Index out of bounds");

			const_iterator it = cbegin();
			while (index--)
				it++;

			return *it;
		}

		[[nodiscard("use result of index operator")]] char_type operator[](u32 index) { return at(index); }

		bool operator==(const string& other) const
		{
			if (buffer.size() != other.buffer.size())
				return false;


			for (u32 i = 0; i < buffer.size(); i++)
			{
				if (buffer.at(i) != other.buffer.at(i))
					return false;
			}
			return true;
		}

		bool operator==(std::span<u8> other) const
		{
			if (buffer.size() != other.size())
				return false;


			for (u32 i = 0; i < other.size(); i++)
			{
				if (buffer[i] != other[i])
					return false;
			}
			return true;
		}

		bool operator==(std::string_view str) const { return operator==({as<u8*>(str.data()), str.length()}); }

		iterator insert(iterator pos, std::string_view input)
		{
			if (input.empty())
				return pos;

			if (not pos.empty() and pos == end())
				pos--;

			auto  insert_pos          = pos.byteindex();
			insert_pos += pos.width() > 1 ? pos.width() : 0;

			buffer.insert(buffer.begin() + insert_pos, {as<u8*>(input.data()), input.size()});

			return iterator(&buffer, insert_pos);
		}

		iterator insert(iterator pos, const string& str)
		{
			if (str.empty())
				return pos;

			//if (not pos.empty() and pos == end())
			//	pos--;

			auto insert_pos = pos.byteindex() +pos.width();

			buffer.insert(buffer.begin() + insert_pos, {str.data(), str.size_in_bytes()});

			return iterator(&buffer, insert_pos);
		}

		void assign(const char* str) { buffer.assign({as<u8*>(str), std::strlen(str)}); }

		void append(const std::string_view str) { insert(end(), str); }

		void append(const string& other) { insert(end(), other); }

		void append(const u8 b) { buffer.append(b); }

		void append(const char b) { buffer.append(b); }

		void append(const char32 c)
		{
			auto encoded = encode_codepoint(c);
			buffer.append({encoded.bytes.data(), encoded.count});
		}

		auto operator+(const std::string_view other) const
		{
			auto ret(*this);
			ret.append(other);
			return ret;
		}

		auto operator+(const string& other)
		{
			auto ret(*this);
			ret.append(other);
			return ret;
		}

		auto operator+=(const std::string_view other)
		{
			append(other);
			return *this;
		}

		auto operator+=(const string& other)
		{
			append(other);
			return *this;
		}

		iterator erase(iterator pos)
		{
			if (pos == end())
				return end();

			auto   erase_start = pos.byteindex();
			size_t erase_width = pos.width();

			buffer.erase(erase_start, erase_width);

			return iterator(&buffer, erase_start);
		}

		iterator erase(iterator first, iterator last)
		{
			if (first == last)
				return first;

			size_t erase_start = first.byteindex();
			size_t erase_count = last.byteindex() - erase_start;


			buffer.erase(erase_start, erase_count);


			return iterator(&buffer, erase_start);
		}

		string& erase(size_t pos, size_t count = std::string::npos)
		{
			if (empty() || pos + count - 1 >= size())
				return *this;

			auto first = begin();
			for (size_t i = 0; i < pos && first != end(); ++i)
				++first;

			if (pos + count >= size())
			{
				erase(first, end() + 1);
			}
			else
			{
				auto last = first;
				for (size_t i = 0; i < count && last != end(); ++i)
					++last;


				erase(first, last);
			}

			return *this;
		}

		iterator replace(iterator first, std::string_view str) { return replace(first, first, str); }

		iterator replace(iterator first, string str) { return replace(first, first, str); }

		iterator replace(iterator first, iterator end, std::string_view str)
		{
			if (first == end)
			{
				auto pos = erase(first);
				return insert(pos, str);
			}

			auto pos = erase(first, end);
			return insert(pos, str);
		}

		iterator replace(iterator first, iterator end, string str)
		{
			if (first == end)
			{
				auto pos = erase(first);
				return insert(pos, str);
			}
			auto pos = erase(first, end);
			return insert(pos, str);
		}

		iterator replace(size_t pos, size_t count, std::string_view str)
		{
			assert::check(pos < size(), "Index out of bounds");
			assert::check(pos + count <= size(), "Index out of bounds");

			auto first = begin() + pos;
			auto end   = first + count;
			return replace(first, end, str);
		}

		iterator replace(size_t pos, size_t count, string str)
		{
			assert::check(pos < size(), "Index out of bounds");
			assert::check(pos + count <= size(), "Index out of bounds");

			auto first = begin() + pos;
			auto end   = first + count;
			return replace(first, end, str);
		}

		index_type find(std::span<u8> input, size_t offset = 0) const
		{
			if (input.empty() || input.size() > buffer.size() - offset)
				return -1;

			for (size_t i = offset; i <= buffer.size() - input.size(); ++i)
			{
				if (i > 0 and utf8::is_continuation_byte(buffer[i]))
					continue;

				bool found = true;
				for (size_t j = 0; j < input.size(); ++j)
				{
					if (buffer[i + j] != input[j])
					{
						found = false;
						break;
					}
				}

				if (found)
				{
					index_type codepoint_index = 0;
					for (size_t pos = 0; pos < i; ++pos)
					{
						if (not utf8::is_continuation_byte(buffer[pos]))
							++codepoint_index;
					}
					return codepoint_index;
				}
			}

			return -1;
		}

		index_type find(std::string_view input, size_t offset = 0) const
		{
			return find(std::span<u8>{as<u8*>(input.data()), input.size()}, offset);
		}

		index_type find(string input, size_t offset = 0) const
		{
			if (input.empty() || input.size_in_bytes() > buffer.size() - offset)
				return -1;

			for (size_t i = offset; i <= buffer.size() - input.size_in_bytes(); ++i)
			{
				if (i > 0 and utf8::is_continuation_byte(buffer[i]))
					continue;

				bool found = true;
				for (size_t j = 0; j < input.size_in_bytes(); ++j)
				{
					if (buffer[i + j] != input.buffer[j])
					{
						found = false;
						break;
					}
				}

				if (found)
				{
					index_type codepoint_index = 0;
					for (size_t pos = 0; pos < i; ++pos)
					{
						if (not utf8::is_continuation_byte(buffer[pos]))
							++codepoint_index;
					}
					return codepoint_index;
				}
			}

			return -1;
		}

		bool contains(std::span<u8> input) const
		{
			if (input.empty() || input.size() > buffer.size())
				return false;

			for (size_t i = 0; i <= buffer.size() - input.size(); ++i)
			{
				if (std::equal(input.begin(), input.end(), buffer.begin() + i))
					return true;
			}

			return false;
		}

		bool contains(std::string_view str) const { return contains({as<u8*>(str.data()), str.size()}); }

		bool contains(string str) const { return contains(std::span<u8>{str.data(), str.size_in_bytes()}); }

		// starts with
		bool starts_with(std::span<u8> input) const
		{
			if (input.empty())
				return true;

			if (input.size() > buffer.size())
				return false;

			for (size_t i = 0; i < input.size(); i++)
			{
				if (buffer[i] != buffer[i])
					return false;
			}

			return true;
		}

		bool starts_with(std::string_view str) const { return starts_with({as<u8*>(str.data()), str.size()}); }

		bool starts_with(string str) const
		{
			if (str.empty())
				return true;

			if (str.size_in_bytes() > buffer.size())
				return false;

			for (size_t i = 0; i < str.size_in_bytes(); i++)
			{
				if (buffer[i] != str.buffer[i])
					return false;
			}

			return true;
		}

		// ends with
		bool ends_with(std::span<u8> input) const
		{
			if (input.empty())
				return true;

			if (input.size() > buffer.size())
				return false;

			auto buffer_end = buffer.size() - input.size();
			for (size_t i = 0; i < input.size(); i++)
			{
				if (buffer[buffer_end + i] != input[i])
					return false;
			}

			return true;
		}

		bool ends_with(std::string_view str) const { return ends_with({as<u8*>(str.data()), str.size()}); }

		bool ends_with(string str) const { return ends_with(std::span<u8>{str.data(), str.size_in_bytes()}); }

		string substr(size_t start, size_t end)
		{
			if (empty() or start >= size() or start >= end)
				return string{};


			size_t     size  = end - start + 1;
			auto       it    = begin() + start;
			const auto itend = it + size;

			string result;
			for (; it != itend; ++it)
				result.append((char32)*it);

			return result;
		}

		index_type front()
		{
			if (empty())
				return REPLACEMENT_CHARACTER;

			return *begin();
		}

		index_type back()
		{
			if (empty())
				return REPLACEMENT_CHARACTER;

			auto it = end();
			it--;
			return *it;
		}

		iterator begin() { return iterator(&buffer, 0); }

		iterator end()
		{
			iterator it(&buffer, buffer.size());
			return it;
		}

		const_iterator cbegin() { return const_iterator(&buffer, 0); }

		const_iterator cend()
		{
			const_iterator it(&buffer, buffer.size());
			return it;
		}

		auto rbegin() { return std::reverse_iterator(end()); }

		auto rend() { return std::reverse_iterator(begin()); }

		auto data() const { return buffer.data().data(); }

		auto c_str() const { return as<const char*>(buffer.data().data()); }

		bool empty() const { return buffer.empty(); }

		void clear() { buffer.clear(); }

		size_t size_in_bytes() const { return buffer.size(); }

		size_t size() const { return length(); }

		bool is_valid() const { return length() != 0; }

		size_t count()
		{
			if (empty())
				return 0;

			size_t count = 0;
			auto   it    = cbegin();
			auto   end   = cend();

			while (it != end)
			{
				count++;

				char32 current = *it;
				it++;

				if (it == end)
					break;

				while (it != end)
				{
					char32 next = *it;

					bool is_combining =
					  (next >= 0x0300 && next <= 0x036F)    // Combining Diacritical Marks
					  || (next >= 0x1AB0 && next <= 0x1AFF) // Combining Diacritical Marks Extended
					  || (next >= 0x1DC0 && next <= 0x1DFF) // Combining Diacritical Marks Supplement
					  || (next >= 0x20D0 && next <= 0x20FF) // Combining Diacritical Marks for Symbols
					  || (next >= 0xFE20 && next <= 0xFE2F) // Combining Half Marks
					  // Add these new ranges:
					  || (next >= 0x0483 && next <= 0x0489)  // Cyrillic Combining Marks
					  || (next >= 0x0591 && next <= 0x05BD)  // Hebrew Points
					  || (next >= 0x064B && next <= 0x065F)  // Arabic Combining Marks
					  || (next >= 0x0900 && next <= 0x0903)  // Devanagari Signs
					  || (next >= 0x093A && next <= 0x094F)  // Additional Devanagari
					  || (next >= 0x0981 && next <= 0x0983)  // Bengali Signs
					  || (next >= 0x0E31 && next <= 0x0E3A)  // Thai Combining Marks
					  || (next >= 0x0F18 && next <= 0x0F19)  // Tibetan Subjoined Letters
					  || (next >= 0x0F35 && next <= 0x0F37)  // Tibetan Signs
					  || (next >= 0x0F71 && next <= 0x0F84); // Tibetan Vowel Signs

					// Enhance emoji handling:
					bool is_emoji =
					  (next >= 0x1'F3FB && next <= 0x1'F3FF)    // Emoji skin tone modifiers
					  || (next >= 0x1'F1E6 && next <= 0x1'F1FF) // Regional indicator symbols
					  || (next >= 0xFE00 && next <= 0xFE0F)     // Variation Selectors
					  || (next >= 0xE'0020 && next <= 0xE'007F) // Tags
					  // Add these new ranges:
					  || (next >= 0x1'F300 && next <= 0x1'F9FF)  // Miscellaneous Symbols and Pictographs
					  || (next >= 0x1'F600 && next <= 0x1'F64F)  // Emoticons
					  || (next >= 0x1'F680 && next <= 0x1'F6FF)  // Transport and Map Symbols
					  || (next >= 0x1'F900 && next <= 0x1'F9FF); // Supplemental Symbols and Pictographs


					// Add special character handling:
					bool is_special =
					  (next == 0xFE0F)                        // Variation Selector-16 (0xFE0F)
					  || (next == 0x20E3)                     // Combining Enclosing Keycap (0x20E3)
															  // Add these new special characters:
					  || (next == 0x200D) || (next == 0x200C) // Zero Width Non-Joiner
					  || (next == 0x200E)                     // Left-to-Right Mark
					  || (next == 0x200F)                     // Right-to-Left Mark
					  || (next == 0x2060)                     // Word Joiner
					  || (next == 0x2061)                     // Function Application
					  || (next >= 0x2062 && next <= 0x2064);  // Invisible Operators


					// Handle Indic scripts
					bool is_indic_vowel = (next >= 0x0900 && next <= 0x0903) // Devanagari vowel marks
										  || (next >= 0x093A && next <= 0x094F) || (next >= 0x0955 && next <= 0x0957);

					if (is_combining || is_emoji || is_special || is_indic_vowel)
						++it;
					else
						break;
				}
			}
			return count;
		}

		size_t length() const
		{
			if (empty())
				return 0;

			const u8* ptr    = buffer.data().data();
			const u8* endptr = ptr + buffer.size();

			size_t len{};
			bool   valid = true;

			for (; (ptr < endptr) && valid; len++)
			{
				valid =
				  (*ptr & 0x80) == 0 or ((*ptr & 0xE0) == 0xC0 && (*(ptr + 1) & 0xC0) == 0x80) or
				  ((*ptr & 0xF0) == 0xE0 && (*(ptr + 1) & 0xC0) == 0x80 && (*(ptr + 2) & 0xC0) == 0x80) or
				  ((*ptr & 0xF8) == 0xF0 && (*(ptr + 1) & 0xC0) == 0x80 && (*(ptr + 2) & 0xC0) == 0x80 && (*(ptr + 3) & 0xC0) == 0x80);

				i32 v1 = ((*ptr & 0x80) >> 7) & ((*ptr & 0x40) >> 6);
				i32 v2 = (*ptr & 0x20) >> 5;
				i32 v3 = (*ptr & 0x10) >> 4;
				ptr += 1 + ((v1 << v2) | (v1 & v3));
			}

			return valid ? len : 0ull;
		}
	};

	// TODO:
	// erase
	// c_str() -> string_view
	// data() -> span

	export string operator"" _utf8(char const* s, size_t count) { return utf8::string({s, count}); }


} // namespace deckard::utf8

export namespace std

{
	using namespace deckard;

	// template<>
	// struct hash<utf8::string2>
	//{
	//	size_t operator()(const utf8::string2& value) const { return deckard::utils::hash_values(value.to_string()); }
	// };

	template<>
	struct formatter<utf8::string>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			// TODO: width
			return ctx.begin();
		}

		auto format(const utf8::string& v, std::format_context& ctx) const
		{
			std::string_view view{as<char*>(v.data()), v.size_in_bytes()};
			return std::format_to(ctx.out(), "{}", view);
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};
} // namespace std
