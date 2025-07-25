﻿export module deckard.utf8:string;
import :codepoints;
import :decode;
import :view;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;
import deckard.sbo;
import deckard.utils.hash;

namespace deckard::utf8
{

	template<typename T>
	concept CharacterType = std::same_as<T, char> or std::same_as<T, char32>;


	using value_type      = sbo<32>;
	using pointer         = value_type*;
	using reference       = value_type&;
	using const_pointer   = const value_type*;
	using const_reference = const value_type&;

	using difference_type = std::ptrdiff_t;
	using codepoint_type  = u32;
	using unit            = char32;

	static constexpr size_t BYTES_PER_CODEPOINT = sizeof(codepoint_type);
	static_assert(BYTES_PER_CODEPOINT == 4, "sizeof(char32) is assumed to be 4 bytes");

	export class iterator final
	{
	public:
		friend class string;

		using value_type      = value_type;
		using pointer         = pointer;
		using reference       = reference;
		using const_pointer   = const_pointer;
		using const_reference = const_reference;

		using difference_type = difference_type;

		using iterator_category   = std::random_access_iterator_tag;
		constexpr static u64 npos = limits::max<u64>;

	private:
		pointer ptr{nullptr};
		i64     current_index{0};

		void next_codepoint()
		{
			assert::check(ptr != nullptr, "Null pointer dereference");

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

		void previous_codepoint()
		{
			assert::check(ptr != nullptr, "Null pointer dereference");


			if (current_index > 0)
			{
				current_index -= 1;

				while (current_index > 0 and utf8::is_continuation_byte(ptr->at(current_index)))
					current_index -= 1;

				if (current_index < 0)
					current_index = 0;
			}
		}

		unit decode_current_codepoint() const
		{

			if (current_index >= as<difference_type>(ptr->size()))
				return REPLACEMENT_CHARACTER;

			auto index     = current_index;
			u32  state     = 0;
			unit codepoint = 0;

			for (; index < as<difference_type>(ptr->size()); index++)
			{
				u8 byte = ptr->at(index);

				const auto type = utf8_table[byte];
				codepoint       = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
				state           = utf8_table[256 + state + type];
				if (state == 0)
					return codepoint;
				else if (state == UTF8_REJECT)
					return REPLACEMENT_CHARACTER;
			}
			return REPLACEMENT_CHARACTER;
		}

	public:
		iterator()                           = default;
		iterator(const iterator&)            = default;
		iterator(iterator&&)                 = default;
		iterator& operator=(const iterator&) = default;
		iterator& operator=(iterator&&)      = default;
		~iterator()                          = default;

		iterator(value_type& p)
			: ptr(&p)
			, current_index(0)
		{
			assert::check(ptr != nullptr, "Null pointer dereference");
			assert::check(current_index <= as<i64>(ptr->size()), "Dereferencing out-of-bounds iterator");
		}

		iterator(pointer p)
			: iterator(p, 0)
		{
			assert::check(ptr != nullptr, "Null pointer dereference");
		}

		iterator(pointer p, const difference_type v)
			: ptr(p)
			, current_index(v)
		{
			assert::check(ptr != nullptr, "Null pointer dereference");
			assert::check(current_index <= as<i64>(ptr->size()), "Dereferencing out-of-bounds iterator");
		}

		bool operator==(const iterator& other) const
		{
			assert::check(ptr == other.ptr, "Not pointing to same data");
			return current_index == other.current_index;
		}

		bool operator<(const iterator& other) const
		{
			assert::check(ptr == other.ptr, "Not pointing to same data");
			return current_index < other.current_index;
		}

		bool operator>(const iterator& other) const
		{
			assert::check(ptr == other.ptr, "Not pointing to same data");
			return current_index > other.current_index;
		}

		bool operator<=(const iterator& other) const
		{
			assert::check(ptr == other.ptr, "Not pointing to same data");
			return current_index <= other.current_index;
		}

		bool operator>=(const iterator& other) const
		{
			assert::check(ptr == other.ptr, "Not pointing to same data");
			return current_index >= other.current_index;
		}

		unit operator*() const
		{
			assert::check(ptr != nullptr, "Null pointer dereference");
			assert::check(current_index < as<i64>(ptr->size()), "Dereferencing out-of-bounds iterator");
			return static_cast<unit>(decode_current_codepoint());
		}

		iterator operator++()
		{
			next_codepoint();
			return *this;
		}

		iterator operator++(int)
		{
			iterator tmp = *this;
			next_codepoint();
			return tmp;
		}

		iterator operator--()
		{
			previous_codepoint();
			return *this;
		}

		iterator operator--(int)
		{
			iterator tmp = *this;
			previous_codepoint();
			return tmp;
		}

		iterator operator+=(int v)
		{
			while (v)
			{
				next_codepoint();
				v--;
			}
			return *this;
		}

		iterator operator-=(int v)
		{
			while (v)
			{
				previous_codepoint();
				v--;
			}
			return *this;
		}

		iterator operator+(difference_type n) const
		{
			iterator tmp = *this;
			while (n-- > 0)
				tmp.next_codepoint();
			return tmp;
		}

		iterator operator+(const std::optional<codepoint_type> n) const
		{
			if (n.has_value())
				return operator+(n.value());
			else
				return *this + npos;
		}

		iterator operator-(difference_type n) const
		{
			iterator tmp = *this;
			while (n-- > 0)
				tmp.previous_codepoint();
			return tmp;
		}

		iterator operator-(const std::optional<codepoint_type> n) const
		{
			if (n.has_value())
				return operator-(n.value());
			else
				return *this + npos;
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

		unit operator[](difference_type n) const
		{
			assert::check(n >= 0, "Index is negative");
			assert::check(n < as<difference_type>(ptr->size()), "Index out-of-bounds");
			return ptr->at(n);
		}

		auto byteindex() const { return current_index; }

		auto width() const { return utf8::codepoint_width(ptr->at(current_index)); }

		auto byte_at(size_t index) const
		{
			assert::check(index < ptr->size(), "Index out-of-bounds");
			return ptr->at(index);
		}

		auto byte() const { return byte_at(current_index); }

		// pointer operator->() const { return ptr; }
		// pointer operator->() const = delete;

		codepoint_type codepoint() const { return decode_current_codepoint(); }

		explicit operator bool() const { return current_index < as<difference_type>(ptr->size()) and ptr != nullptr; }
	};

	// #########################


	export class string
	{
	private:
		static constexpr size_t npos = limits::max<size_t>;

		value_type buffer;
		friend class iterator;

	public:
		string() = default;

		string(iterator start, iterator end)
		{
			assert::check(start.ptr == end.ptr, "Not pointing to same data");

			auto start_index = start.byteindex();
			auto end_index   = end.byteindex();
			assert::check(start_index <= end_index, "Invalid range");
			buffer.assign(start.ptr->data().subspan(start_index, end_index - start_index));
		}

		string(std::span<u8> input) { buffer.assign(input); }

		string(std::string_view input) { buffer.assign({as<u8*>(input.data()), input.size()}); }

		string& operator=(std::string_view input)
		{
			buffer.assign({as<u8*>(input.data()), input.size()});
			return *this;
		}

		string& operator=(const view input)
		{
			if (input.empty())
			{
				clear();
				return *this;
			}
			buffer.assign(input.data());
			return *this;
		}

		[[nodiscard("use result of at method")]]
		unit at(u64 index) const
		{
			assert::check(index < size(), "Index out of bounds");

			auto it = begin();
			while (index--)
				it++;

			return *it;
		}

		[[nodiscard("use result of index operator")]] unit operator[](u64 index) const { return at(index); }

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

		iterator insert(iterator pos, const string& str) { return insert(pos, str.data()); }

		//
		void assign(const char* str) { buffer.assign({as<u8*>(str), std::strlen(str)}); }

		void assign(const std::span<u8>& input) { buffer.assign(input); }

		// append
		void append(const std::string_view str) { insert(end(), str); }

		void append(const string& other) { insert(end(), other); }

		void append(const u8 b) { buffer.append(b); }

		void append(const char b) { buffer.append(b); }

		void append(const char32 c)
		{
			auto encoded = encode_codepoint(c);
			buffer.append({encoded.bytes.data(), encoded.count});
		}

		void append(const view v) { append(utf8::string(v)); };

		// TODO: prepend
		void prepend(const std::string_view str) { insert(begin(), str); }

		void prepend(const string& other) { insert(begin(), other); }

		void prepend(const u8 b) { buffer.prepend(b); }

		void prepend(const char b) { buffer.prepend(b); }

		void prepend(const char32 c)
		{
			auto encoded = encode_codepoint(c);
			buffer.prepend({encoded.bytes.data(), encoded.count});
		}

		void prepend(const view v) { prepend(utf8::string(v)); }

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
				return first;

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

		std::optional<codepoint_type> find(std::span<u8> input, size_t offset = 0) const
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
					codepoint_type codepoint_index = 0;
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

		std::optional<codepoint_type> find(string input, size_t offset = 0) const
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
					codepoint_type codepoint_index = 0;
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

		bool contains(string str) const { return contains(str.data()); }

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

		bool starts_with(const string& str) const
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

		bool starts_with(char32 c) const
		{
			if (empty())
				return false;

			return front() == c;
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

		bool ends_with(string str) const { return ends_with(str.data()); }

		codepoint_type front() const
		{
			assert::check(not empty(), "String is empty");

			return *begin();
		}

		codepoint_type back() const
		{
			assert::check(not empty(), "String is empty");

			const auto it = end() - 1;
			return *it;
		}

		iterator begin() const { return iterator(as<pointer>(&buffer)); }

		iterator end() const
		{
			iterator it(as<pointer>(&buffer), buffer.size());
			return it;
		}

		auto rbegin() const { return std::reverse_iterator(end()); }

		auto rend() const { return std::reverse_iterator(begin()); }

		auto span() const { return buffer.data(); }

		bool empty() const { return buffer.empty(); }

		void clear() { buffer.clear(); }

		size_t size_in_bytes() const { return buffer.size(); }

		size_t size() const { return length(); }

		std::expected<bool, std::string> valid() const { return utf8::valid(buffer.data()); }

		size_t length() const
		{
			if (empty() or valid().has_value() == false)
				return 0;

			auto len = utf8::length(buffer.data());
			return len ? *len : 0;
		}

		size_t graphemes() const
		{


			if (empty() or valid().has_value() == false)
				return 0;

			return utf8::graphemes(buffer.data());
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
		auto subspan(size_t start = 0, size_t count = npos) const -> std::span<u8>
		{
			if (start >= size())
			{
			}
			assert::check(start < size(), "Indexing out-of-bounds");

			if (empty())
				return std::span<u8>{};

			// Find byte index of start position
			auto it = begin();
			for (size_t i = 0; i < start && it != end(); ++i)
				++it;

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

		string substr(size_t start = 0, size_t count = npos) const { return string(subspan(start, count)); }

		string substr(iterator start, size_t count = npos) const
		{
			assert::check(start.ptr == &buffer, "Iterator not from this string");

			auto start_byte = start.byteindex();

			if (count == npos)
				return string(buffer.subspan(start_byte, count));

			auto end_it = start;
			for (size_t i = 0; i < count && end_it != end(); ++i)
				++end_it;

			auto end_byte = end_it.byteindex();

			return string(buffer.subspan(start_byte, end_byte - start_byte));
		}

		auto subview(size_t start = 0, size_t count = npos) const { return view(subspan(start, count)); }

		auto subview(iterator start, size_t count = npos) const
		{
			auto start_index = start.byteindex();

			auto end_index = (start + count).byteindex();
			return subview(start_index, end_index - start_index);
		}

		auto data() const { return subspan(); }

		auto c_str() const { return as<const char*>(buffer.data().data()); }

		size_t find_first_of(const string& str, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");


			if (str.empty() or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				for (auto byte : str)
				{
					if (*it == byte)
					{
						return std::distance(begin(), it);
					}
				}
			}


			return npos;
		}

		size_t find_first_of(utf8::view view, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");

			if (view.empty() or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				auto view_it  = view.begin();
				auto view_end = view.end();

				for (; view_it != view_end; ++view_it)
				{
					if (*it == *view_it)
					{
						return std::distance(begin(), it);
					}
				}
			}

			return npos;
		}

		size_t find_first_of(std::string_view str, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");


			if (str.empty() or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				for (char32 byte : str)
				{
					if (*it == byte)
					{
						return std::distance(begin(), it);
					}
				}
			}

			return npos;
		}

		size_t find_first_of(CharacterType auto c, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");

			if (c == 0 or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				if (*it == (char32)c)
					return std::distance(begin(), it);
			}

			return npos;
		}

		size_t find_first_not_of(const string& str, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");

			if (str.empty() or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				bool found = false;
				for (auto byte : str)
				{
					if (*it == byte)
					{
						found = true;
						break;
					}
				}
				if (not found)
					return std::distance(begin(), it);
			}
			return npos;
		}

		size_t find_first_not_of(utf8::view view, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");

			if (view.empty() or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				bool found    = false;
				auto view_it  = view.begin();
				auto view_end = view.end();

				for (; view_it != view_end; ++view_it)
				{
					if (*it == *view_it)
					{
						found = true;
						break;
					}
				}
				if (not found)
					return std::distance(begin(), it);
			}
			return npos;
		}

		size_t find_first_not_of(CharacterType auto c, size_t pos = 0) const
		{
			assert::check(pos < size(), "Index out of bounds");

			if (c == 0 or pos >= size())
				return npos;

			auto it    = begin() + pos;
			auto itend = end();

			for (; it != itend; ++it)
			{
				bool found = false;
				if (*it == (char32)c)
				{
					found = true;
					break;
				}

				if (not found)
					return std::distance(begin(), it);
			}
			return npos;
		}

		size_t find_last_of(const string& view, size_t pos = npos) const
		{
			if (empty() or view.empty())
				return npos;

			if (pos >= size())
				pos = size() - 1;

			auto it = begin() + pos;

			while (it >= begin())
			{
				auto view_it  = view.begin();
				auto view_end = view.end();

				for (; view_it != view_end; ++view_it)
				{
					if (*it == *view_it)
						return std::distance(begin(), it);
				}

				--it;
			}

			return npos;
		}

		size_t find_last_of(std::string_view view, size_t pos = npos) const
		{
			if (empty() or view.empty())
				return npos;
			if (pos >= size())
				pos = size() - 1;
			auto it = begin() + pos;
			while (it >= begin())
			{
				for (char32 byte : view)
				{
					if (*it == byte)
						return std::distance(begin(), it);
				}
				--it;
			}
			return npos;
		}

		size_t find_last_of(CharacterType auto c, size_t pos = npos) const
		{
			if (empty() or c == 0)
				return npos;

			if (pos >= size())
				pos = size() - 1;

			auto it = begin() + pos;

			while (it >= begin())
			{
				if (*it == (char32)c)
						return std::distance(begin(), it);
				--it;
			}
			return npos;
		}

		size_t find_last_not_of(const string& view, size_t pos = npos) const
		{
			if (empty() or view.empty())
				return npos;
			if (pos >= size())
				pos = size() - 1;
			auto it = begin() + pos;
			while (it >= begin())
			{
				bool found = false;
				auto view_it  = view.begin();
				auto view_end = view.end();
				for (; view_it != view_end; ++view_it)
				{
					if (*it == *view_it)
					{
						found = true;
						break;
					}
				}
				if (not found)
					return std::distance(begin(), it);
				--it;
			}
			return npos;
		}
		size_t find_last_not_of(std::string_view view, size_t pos = npos) const
		{
			if (empty() or view.empty())
				return npos;
			if (pos >= size())
				pos = size() - 1;
			auto it = begin() + pos;
			while (it >= begin())
			{
				bool found = false;
				for (char32 byte : view)
				{
					if (*it == byte)
					{
						found = true;
						break;
					}
				}
				if (not found)
					return std::distance(begin(), it);
				--it;
			}
			return npos;
		}
		size_t find_last_not_of(CharacterType auto c, size_t pos = npos) const
		{
			if (empty() or c == 0)
				return npos;
			if (pos >= size())
				pos = size() - 1;
			auto it = begin() + pos;
			while (it >= begin())
			{
				if (*it != (char32)c)
					return std::distance(begin(), it);
				--it;
			}
			return npos;
		}

		void trim_right()
		{
			if (empty())
				return;

			auto it       = end();
			auto begin_it = begin();

			while (it != begin_it)
			{
				auto prev = it - 1;
				if (not utf8::is_whitespace(*prev))
					break;

				it = prev;
			}

			erase(it, end());
		}

		void trim_left()
		{
			if (empty())
				return;
			auto it     = begin();
			auto end_it = end();

			while (it != end_it)
			{
				char32 c = *it;
				if (not utf8::is_whitespace(*it))
					break;
				++it;
			}
			erase(begin(), it);
		}

		void trim()
		{
			trim_left();
			trim_right();
		}


		
	};

	static_assert(sizeof(string) == 32, "string size mismatch");

	// TODO:
	// rfind
	// trim copy, trim modify inplace
	// self insert, deletes prev buffer
	// standalone valid check

	export string operator"" _utf8(char const* s, size_t count) { return utf8::string({s, count}); }


} // namespace deckard::utf8

export namespace std
{
	using namespace deckard;

	template<>
	struct hash<utf8::string>
	{
		size_t operator()(const utf8::string& value) const { return utils::hash_values(value.data()); }
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
			std::string_view view{v.c_str()};
			return std::format_to(ctx.out(), "{}", view);
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};


} // namespace std
