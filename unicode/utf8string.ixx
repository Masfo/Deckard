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
	public:
		using type = char32;

	private:
		struct iterator
		{
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type        = type;

			iterator(string* ptr, i32 i)
				: p(ptr)
				, index(i)
			{
				if (i == 0 and p and not p->empty())
					current = p->next();
			}

			const value_type operator*() const { return current; }

			const iterator& operator++()
			{
				if (index >= 0 and p and not p->empty())
				{
					current = p->next();
					index += 1;
					return *this;
				}
				index = -1;
				return *this;
			}

			friend bool operator==(const iterator& a, const iterator& b) { return a.index == b.index; };

			string* p{nullptr};

			value_type current{REPLACEMENT_CHARACTER};
			i32        index{0};
		};

		type read(u8 byte)
		{
			assert::check(byte < utf8_table.size(), "Out-of-bound indexing on utf8 table");

			const u32 type = utf8_table[byte];

			decoded_point = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (decoded_point << 6) : (0xff >> type) & (byte);
			state         = utf8_table[256 + state + type];
			return state;
		}

		void reset()
		{
			idx             = 0;
			state           = UTF8_ACCEPT;
			codepoint_count = 0;
		}

		std::vector<u8> buffer;
		type            decoded_point{0};
		type            state{UTF8_ACCEPT};
		u32             idx{0};
		u32             codepoint_count{};

		void update_cache()
		{
			reset();
			(void)size();
		}

	public:
		string() = default;

		string(std::string_view input)
		{
			std::ranges::copy_n(input.data(), input.size(), std::back_inserter(buffer));
			update_cache();
		}

		string(std::span<u8> input)
		{
			std::ranges::copy_n(input.data(), input.size(), std::back_inserter(buffer));
			update_cache();
		}

		string(std::optional<std::vector<u8>> input)
		{
			if (input.has_value())
			{
				auto i = *input;
				std::ranges::copy_n(i.data(), i.size(), std::back_inserter(buffer));
				update_cache();
			}
		}

		string(std::vector<u8> input)
			: string(std::optional<std::vector<u8>>(input))
		{
		}

		string(const char* input)
			: string(std::string_view{input})
		{
		}

		bool empty() const { return idx >= buffer.size(); }

		u64 size_in_bytes() const { return buffer.size(); }

		u64 size()
		{
			if (codepoint_count != 0)
				return codepoint_count;

			u32 old_index = idx;
			u32 old_state = state;

			while (not empty())
			{
				if (next())
					codepoint_count += 1;
			}

			idx   = old_index;
			state = old_state;

			return codepoint_count;
		}

		u64 count() { return size(); }

		iterator begin() { return iterator(this, idx); }

		iterator end() { return iterator(this, -1); }

		type next()
		{
			for (state = 0; idx < buffer.size(); idx++)
			{
				u8 byte = buffer[idx];

				if (!read(byte))
				{
					idx += 1;
					return decoded_point;
				}
				else if (state == UTF8_REJECT)
				{
					idx += 1;
					return REPLACEMENT_CHARACTER;
				}
			}

			if (state != UTF8_ACCEPT)
			{
				state = UTF8_ACCEPT;
				return REPLACEMENT_CHARACTER;
			}

			return decoded_point;
		}

		auto data() const { return buffer.data(); }

		auto codepoints() -> std::vector<type>
		{
			std::vector<type> ret;
			ret.reserve(count());

			for (const auto& cp : *this)
				ret.emplace_back(cp);

			return ret;
		}

		bool is_valid() const
		{
			utf8_decode_t state{};
			for (const auto& byte : buffer)
			{
				if (decode(state, byte) == UTF8_REJECT)
					return false;
			}
			return state.state == UTF8_ACCEPT;
		}
	};

	export class string2
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

			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = value_type;

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
			// iterator(const_iterator& ci)
			//	: ptr(ci.ptr)
			//{
			// }

			// iterator() = default;


			iterator(pointer p)
				: iterator(p, 0)
			{
			}

			iterator(pointer p, difference_type v)
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

			difference_type operator-(const iterator& other) const { return ptr - other.ptr; }

			reference& operator[](difference_type n) const { return ptr[n]; }

			// auto operator<=>(const iterator&) const = default;

			bool operator==(const iterator& other) const
			{
				// TODO: assert on pointer diff
				return ptr == other.ptr and current_index == other.current_index;
			}

			
		};
#if 1
		class const_iterator
		{
		public:
			friend class iterator;

			using iterator_category = std::bidirectional_iterator_tag;
			using difference_type   = std::ptrdiff_t;
			using value_type        = value_type;

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

			// const_iterator(iterator ci)
			//	: ptr(ci.ptr)
			//{
			// }

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

			difference_type operator-(const const_iterator& other) const { return ptr - other.ptr; }

			// reference& operator[](difference_type n) const { return ptr[n]; }

			// auto operator<=>(const const_iterator&) const = default;

			bool operator==(const const_iterator& other) const
			{
				// TODO: assert on pointer diff
				return ptr == other.ptr and current_index == other.current_index;
			}
		};
#endif

	public:
		string2() = default;

		string2(std::string_view input) { buffer.assign({as<u8*>(input.data()), input.size()}); }

		string2 operator=(std::string_view input)
		{
			buffer.assign({as<u8*>(input.data()), input.size()});
			return *this;
		}

		[[nodiscard("use result of at method")]]
		char_type at(u32 index) const
		{
			static char_type invalid_codepoint = REPLACEMENT_CHARACTER;
			if (index >= size())
				return invalid_codepoint;

			i64 current_index = 0;
			i64 next          = 0;


			return invalid_codepoint;
		}

		[[nodiscard("use result of index operator")]] char_type operator[](u32 index)
		{
			static char_type invalid_codepoint = REPLACEMENT_CHARACTER;
			if (index >= buffer.size())
				return invalid_codepoint;

			u32 current_index = 0;
			for (auto it = cbegin(); it != cend(); ++it, ++current_index)
			{
				if (current_index == index)
					return *it;
			}
			return invalid_codepoint;
		}

#if 1
		[[nodiscard("use result of index operator")]] const char_type& operator[](u32 index) const
		{
			static char_type invalid_codepoint = REPLACEMENT_CHARACTER;
			if (index >= buffer.size())
				return invalid_codepoint;

			u32 current_index = 0;
			// for (auto  it = cbegin(); it != cend(); ++it, ++current_index)
			//{
			//	if (current_index == index)
			//		return *it;
			// }
			return invalid_codepoint;
		}
#endif

		bool operator==(const string2& other) const
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

		iterator begin() { return iterator(&buffer); }

		iterator end() { return iterator(&buffer, buffer.size()); }

		const_iterator cbegin() { return const_iterator(&buffer); }

		const_iterator cend() { return const_iterator(&buffer, buffer.size()); }

		auto rbegin() { return std::reverse_iterator(end()); }

		auto rend() { return std::reverse_iterator(begin()); }

		//		 auto cbegin() { return buffer.cbegin(); }
		//
		//		 auto cend() { return buffer.cend(); }

		auto data() const { return buffer.data().data(); }

		bool empty() const { return buffer.empty(); }

		size_t size_in_bytes() const { return buffer.size(); }

		size_t size() const { return length(); }

		size_t length() const
		{
			if (empty())
				return {};

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

		// substr
	};


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
	struct formatter<utf8::string2>
	{
		constexpr auto parse(std::format_parse_context& ctx)
		{
			// TODO: width
			return ctx.begin();
		}

		auto format(const utf8::string2& v, std::format_context& ctx) const
		{
			std::string_view view{as<char*>(v.data()), v.size_in_bytes()};
			return std::format_to(ctx.out(), "{}", view);
		}

		int  parsed_base = 10;
		bool uppercase   = false;
	};
} // namespace std
