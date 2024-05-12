export module deckard.utf8;


import std;
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

	export constexpr u32 REPLACEMENT_CHARACTER{0xFFFD};

#define UTF8_STAT 0

	export class codepoints
	{
	public:
		codepoints() = default;

		codepoints(std::string_view input)
			: codepoints(std::span{(u8*)input.data(), input.size()})
		{
		}

		codepoints(std::span<u8> input)
			: buffer(input)
		{
		}

		void reset() noexcept
		{
			index = 0;
			state = UTF8_ACCEPT;
		}

		void reload(std::string_view input) noexcept
		{
			reset();
			buffer = std::span{(u8*)input.data(), input.size()};
		}

		u32 count() noexcept
		{
			u32 old_index = index;
			u32 old_state = state;

			reset();

			u32 count{0};
			while (has_data())
			{
				if (next())
					count += 1;
			}

			index = old_index;
			state = old_state;

			return count;
		}

		u32 next() noexcept
		{

			u8 byte = 0;

			for (state = 0; index < buffer.size(); index++)
			{
				byte = buffer[index];

#if UTF8_STAT
				auto leading_ones = std::countl_one(byte);
				switch (leading_ones)
				{
					case 0: ascii_bytes++; break;
					case 1: continuation_bytes++; break;
					case 2: two_bytes++; break;
					case 3: three_bytes++; break;
					case 4: four_bytes++; break;
					default: dbg::panic("utf8 not handled");
				}
#endif

				if (!read(byte))
				{
					index += 1;
					return decoded_point;
				}
				else if (state == UTF8_REJECT)
				{
					index += 1;
#if UTF8_STAT
					invalid_bytes += 1;
#endif
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

		bool has_data() const noexcept { return index < buffer.size(); }

		std::vector<u32> data() noexcept
		{
			std::vector<u32> points;
			points.reserve(buffer.size());

			while (has_data())
			{
				points.push_back(next());
			}

			return points;
		}

	private:
		u32 read(u8 byte) noexcept
		{
			assert::if_true(byte < utf8_table.size(), "Out-of-bound indexing on utf8 table");

			uint32_t type = utf8_table[byte];

			decoded_point = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (decoded_point << 6) : (0xff >> type) & (byte);
			state         = utf8_table[256 + state + type];
			return state;
		}

#if UTF8_STAT
	public:
		u32 ascii_bytes{0};
		u32 continuation_bytes{0};
		u32 two_bytes{0};
		u32 three_bytes{0};
		u32 four_bytes{0};
		u32 invalid_bytes{0};
#endif
	private:
		std::span<u8> buffer;
		u32           index{0};
		u32           decoded_point{0};
		u32           state{UTF8_ACCEPT};
		bool          buffer_empty{false};
	};

	export constexpr bool is_bom(u32 codepoint) noexcept { return codepoint == 0xFEFF or codepoint == 0xFFFE; }

	export u32 count_codepoints(const std::span<u8> input) noexcept
	{
		codepoints decoder(input);
		return decoder.count();
	}

	export auto count_codepoints(const std::string_view input) noexcept
	{
		codepoints decoder(input);
		return decoder.count();
	}

	export std::vector<u32> codepoints_to_vector(const std::span<u8> input) noexcept
	{
		codepoints decoder(input);
		return decoder.data();
	}

	export auto codepoints_to_vector(const std::string_view input) noexcept
	{
		codepoints decoder(input);
		return decoder.data();
	}

} // namespace deckard::utf8
