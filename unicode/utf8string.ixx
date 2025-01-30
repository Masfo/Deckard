export module deckard.utf8:string;
import :codepoints;

import std;
import deckard.types;
import deckard.assert;
import deckard.as;

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
	};


} // namespace deckard::utf8
