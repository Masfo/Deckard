export module deckard.utf8:string;
import :codepoints;
import :decode;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.sbo;
import deckard.utils.hash;

namespace deckard::utf8
{


	export class string
	{
	private:
		sbo<32> buffer;

		using value_type                            = sbo<32>;
		using pointer                               = value_type*;
		using reference                             = value_type&;
		using const_pointer                         = const pointer;
		using const_reference                       = const reference;
		using difference_type                       = std::ptrdiff_t;
		using index_type                            = u32;
		using char_type                             = char32;
		static constexpr size_t BYTES_PER_CODEPOINT = sizeof(char_type);
		static constexpr size_t npos                = limits::max<size_t>;

		static_assert(BYTES_PER_CODEPOINT == 4, "sizeof(char32) is assumed to be 4 bytes");

		class const_iterator;

		class iterator
		{
		public:
			friend class const_iterator;

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

			iterator(pointer p, const difference_type v)
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
				return ptr == other.ptr and current_index == other.current_index;
			}
		};
#endif

	public:
		string() = default;

		string(std::span<u8> input) { buffer.assign(input); }

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

		// insert
		iterator insert(iterator pos, std::span<u8> input)
		{
			if (input.empty())
				return pos;


			auto insert_pos = pos.byteindex();

			buffer.insert(buffer.begin() + insert_pos, input);

			return iterator(&buffer, insert_pos);
		}

		iterator insert(iterator pos, char c) { return insert(pos, {as<u8*>(&c), 1}); }

		iterator insert(iterator pos, char32 c)
		{
			auto decoded = encode_codepoint(c);
			return insert(pos, {decoded.bytes.data(), decoded.count});
		}

		iterator insert(iterator pos, std::string_view input) { return insert(pos, {as<u8*>(input.data()), input.size()}); }

		iterator insert(iterator pos, const string& str) { return insert(pos, {str.data(), str.size_in_bytes()}); }

		//
		void assign(const char* str) { buffer.assign({as<u8*>(str), std::strlen(str)}); }

		void assign(const std::span<u8>& input) { buffer.assign(input); }

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

			if (not str.valid())
			{
				dbg::println("Trying to replace with invalid string");
				return first;
			}

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

		std::optional<index_type> find(std::span<u8> input, size_t offset = 0) const
		{
			if (input.empty() || input.size() > buffer.size() - offset)
				return {};

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

			return {};
		}

		auto find(std::string_view input, size_t offset = 0) const
		{
			return find(std::span<u8>{as<u8*>(input.data()), input.size()}, offset);
		}

		std::optional<index_type> find(string input, size_t offset = 0) const
		{
			if (input.empty() or input.size_in_bytes() > buffer.size() - offset or not input.valid())
				return {};

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

			return {};
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

		iterator begin() { return iterator{&buffer, 0}; }

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

		auto span() const { return buffer.data(); }

		auto c_str() const { return as<const char*>(buffer.data().data()); }

		bool empty() const { return buffer.empty(); }

		void clear() { buffer.clear(); }

		size_t size_in_bytes() const { return buffer.size(); }

		size_t size() { return length(); }

		bool valid() const { return length() != 0; }

		size_t length() const
		{
			if (empty())
				return 0;

			auto len = utf8::length(buffer.data());
			return len ? *len : 0;
		}

		void resize(size_t count, char32 c)
		{
			if (count == 0)
			{
				clear();
				return;
			}


			size_t current_length = length();

			if (count == current_length)
				return;

			if (count < current_length)
			{
				auto it = begin();
				for (size_t i = 0; i < count; ++i)
					++it;
				erase(it, end());
			}
			else if (count > current_length)
			{
				size_t to_add = count - current_length;
				for (size_t i = 0; i < to_add; ++i)
				{
					append(c);
				}
			}
		}

		void resize(size_t count, char c) { resize(count, (char32)c); }

		void resize(size_t count) { resize(count, (char32)0); }

		void reserve(size_t bytes) { buffer.reserve(bytes); }

		auto capacity() const { return buffer.capacity(); }

		// start index of character, count of characters
		// returns raw bytes of the string
		auto subspan(size_t start, size_t count = npos) -> std::span<u8>
		{
			assert::check(start < size(), "Index out of bounds");

			if (empty())
				return std::span<u8>{};

			// Find byte index of start position
			auto it = begin();
			for (size_t i = 0; i < start && it != end(); ++i)
				++it;

			// TODO: no iterators

			if (it == end())
				return std::span<u8>{};

			auto start_byte = it.byteindex();

			// Count N codepoints forward
			size_t remaining = std::min(count, size() - start);
			for (size_t i = 0; i < remaining && it != end(); ++i)
				++it;

			auto end_byte = it.byteindex();

			return buffer.subspan(start_byte, end_byte - start_byte);
		}

		string substr(size_t start, size_t count = npos) { return string(subspan(start, count)); }


#if 0
		iterator find_first_of(std::span<u8> input, size_t pos = 0)
		{
			assert::check(pos < size(), "Index out of bounds");

			auto it     = begin();
			auto end_it = end();
			for (; it != end_it; ++it)
			{
				for (auto input_it = input.begin(); input_it != input.end(); ++input_it)
				{
					if (*it == *input_it)
					{
						return it;
					}
				}
			}
			return end_it;
		}

		iterator find_first_of(string &str, size_t pos = 0) { return find_first_of({str.data(), str.size_in_bytes()}, pos); }

		iterator find_first_of(std::string_view str, size_t pos = 0) { return find_first_of({str.data(), str.size()}, pos); }


		iterator find_first_not_of(string &str, size_t pos = 0)
		{
			assert::check(pos < size(), "Index out of bounds");

			auto it     = begin() + pos;
			auto end_it = end();

			for (; it != end_it; ++it)
			{
				bool found = false;
				for (auto str_it = str.begin(); str_it != str.end(); ++str_it)
				{
					if (*it == *str_it)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					return it;
				}
			}
			return end_it;
		}
#endif
	};

	// TODO:
	// find_first_of / first_not_of
	// find_last_of  / last_no_of
	// subview -> utf8::view
	// self insert, deletes prev buffer


	export string operator"" _utf8(char const* s, size_t count) { return utf8::string({s, count}); }


} // namespace deckard::utf8

export namespace std

{
	using namespace deckard;

	template<>
	struct hash<utf8::string>
	{
		size_t operator()(const utf8::string& value) const
		{
			return utils::hash_values(std::span<u8>{value.data(), value.size_in_bytes()});
		}
	};

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
