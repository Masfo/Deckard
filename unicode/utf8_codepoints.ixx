export module deckard.utf8:codepoints;
import :xid;

import std;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.debug;

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

	export constexpr char32_t REPLACEMENT_CHARACTER{0xFFFD};
	export constexpr char32_t EOF_CHARACTER{0xFFFF};

	export class codepoints
	{
	private:
		struct iterator
		{
			using iterator_category = std::forward_iterator_tag;
			using value_type        = char32_t;

			iterator(codepoints* ptr, i32 i)
				: p(ptr)
				, index(i)
			{
				if (i == 0 and p and p->has_data())
					current = p->next();
			}

			const value_type operator*() const { return current; }

			const iterator& operator++()
			{
				if (index >= 0 and p and p->has_next())
				{
					current = p->next();
					index += 1;
					return *this;
				}
				index = -1;
				return *this;
			}

			friend bool operator==(const iterator& a, const iterator& b) { return a.index == b.index; };

			codepoints* p{nullptr};
			value_type  current{REPLACEMENT_CHARACTER};
			i32         index{0};
		};

	public:
		using type = char32_t;

		iterator begin() { return iterator(this, idx); }

		iterator end() { return iterator(this, -1); }

		codepoints() = default;

		codepoints(std::string_view input)
			: codepoints(std::span{as<u8*>(input.data()), input.size()})
		{
		}

		codepoints(std::optional<std::span<u8>> input)
		{
			if (input.has_value())
				buffer = *input;
		}

		codepoints(std::span<u8> input)
			: buffer(input)
		{
		}

		// TODO: Begin end iterator, for-range loop

		void reset() noexcept
		{
			idx             = 0;
			state           = UTF8_ACCEPT;
			codepoint_count = 0;
		}

		void reload(std::span<u8> input) noexcept
		{
			reset();
			buffer = input;
		}

		void reload(std::string_view input) noexcept
		{
			reset();
			buffer = std::span{as<u8*>(input.data()), input.size()};
		}

		// return buffer size in bytes
		u64 size_in_bytes() const noexcept { return as<u64>(buffer.size()); }

		// count - returns the number of codepoints in the buffer
		u32 count() noexcept
		{
			if (codepoint_count != 0)
				return codepoint_count;

			u32 old_index = idx;
			u32 old_state = state;

			reset();

			while (has_data())
			{
				if (next())
					codepoint_count += 1;
			}

			idx   = old_index;
			state = old_state;

			return codepoint_count;
		}

		bool has_next() const noexcept { return idx < buffer.size(); }

		// next() - returns the next codepoint
		type next() noexcept
		{

			u8 byte = 0;

			for (state = 0; idx < buffer.size(); idx++)
			{
				byte = buffer[idx];

				if (!read_byte(byte))
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

		bool has_data() const noexcept { return idx < buffer.size(); }

		// data() - collects all codepoints to a vector
		std::vector<type> data(bool eof) noexcept
		{
			std::vector<type> points;
			points.reserve(buffer.size());

			while (has_data())
			{
				points.push_back(next());
			}

			reset();

			if (eof)
				points.push_back(utf8::EOF_CHARACTER);
			return points;
		}

		u32 index() const { return idx; };

		std::span<u8> buffer;

	private:
		type read_byte(u8 byte) noexcept
		{
			assert::check(byte < utf8_table.size(), "Out-of-bound indexing on utf8 table");

			u32 type = utf8_table[byte];

			decoded_point = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (decoded_point << 6) : (0xff >> type) & (byte);
			state         = utf8_table[256 + state + type];
			return state;
		}

		type decoded_point{0};
		type state{UTF8_ACCEPT};
		u32  idx{0};
		u32  codepoint_count{};
	};

	export constexpr u32 codepoint_width(char32_t codepoint) noexcept
	{

		if (codepoint < 0x80)
			return 1;
		if (codepoint < 0x800)
			return 2;
		if (codepoint < 0x10000)
			return 3;
		if (codepoint < 0x20000)
			return 4;
		if (codepoint < 0x400'0000)
			return 5;
		if (codepoint < 0x8000'0000)
			return 6;

		return 7;
	}

	export constexpr bool is_bom(char32_t codepoint) noexcept { return codepoint == 0xFEFF or codepoint == 0xFFFE; }

	export constexpr bool is_whitespace(char32_t codepoint) noexcept
	{
		// PropList-15.1.0.txt
		return (
		  (codepoint == 0x0020) or                             // space
		  ((codepoint >= 0x0009) and (codepoint <= 0x000D)) or // control
		  (codepoint == 0x0085) or                             // control
		  (codepoint == 0x00A0) or                             // no-break space
		  (codepoint == 0x1680) or                             // Ogham space mark
		  ((codepoint >= 0x2000) and (codepoint <= 0x200A)) or // EN QUAD..HAIR SPACE
		  (codepoint == 0x2028) or                             // LINE SEPARATOR
		  (codepoint == 0x2029) or                             // PARAGRAPH SEPARATOR
		  (codepoint == 0x202F) or                             // NARROW NO-BREAK SPACE
		  (codepoint == 0x205F) or                             // MEDIUM MATHEMATICAL SPACE
		  (codepoint == 0x2060) or                             // WORD JOINER (like U+00A0)
		  (codepoint == 0x3000));                              // IDEOGRAPHIC SPACE
	}

	export constexpr bool is_identifier_start(char32_t codepoint) noexcept
	{
		return is_ascii_identifier_start(codepoint) or is_xid_start(codepoint);
	}

	export constexpr bool is_identifier_continue(char32_t codepoint) noexcept
	{
		return is_ascii_identifier_continue(codepoint) or is_xid_continue(codepoint);
	}

} // namespace deckard::utf8
