export module deckard.utf8:decode;

import :codepoints;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;
import :utf8_span;

export namespace deckard::utf8
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
	// constexpr u32 UTF8_ACCEPT{0};
	constexpr u32 UTF8_REJECT{12};

	constexpr char32 REPLACEMENT_CHARACTER{0xFFFD}; // U+FFFD,  0xEF 0xBF 0xBD,  (239, 191, 189) REPLACEMENT CHARACTER

	struct decode_result
	{
		char32 codepoint{};
		u32    bytes_consumed{};
	};

    auto decode(std::span<const std::byte> buffer, size_t index) -> std::optional<decode_result>
	{
      assert::check(index < buffer.size(), "Index out-of-bounds");
		if (index >= buffer.size())
			return std::nullopt;

       u32    bytes_consumed = 0;
		u32    state     = 0;
		char32 codepoint = 0;
        for (; index < buffer.size(); index++)
		{
          u8 byte = utf8::u8_at(buffer, index);
			bytes_consumed += 1;
			const u32 type = utf8_table[byte];
			codepoint      = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
			state          = utf8_table[256 + state + type];
			if (state == 0)
             return decode_result{codepoint, bytes_consumed};
			else if (state == UTF8_REJECT)
             return decode_result{REPLACEMENT_CHARACTER, 1};
		}
     // Truncated sequence: consume one byte so callers always make forward progress.
		return decode_result{REPLACEMENT_CHARACTER, 1};
	}

    std::generator<char32> yield_codepoints(std::span<const std::byte> buffer)
	{
		size_t i = 0;
		while (i < buffer.size())
		{
			auto result = decode(buffer, i);
			if (result)
			{
				co_yield result.value().codepoint;
				i += result.value().bytes_consumed;
				continue;
			}
          // Should be unreachable unless assertions are disabled.
			i += 1;
		}
	}

    size_t graphemes(std::span<const std::byte> buffer)
	{
		// TODO: note that does not do full breaking rules
		//		 Unicode Standard Annex #29 (UAX #29)
		// https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries

		size_t count = 0;
		size_t i     = 0;
		while (i < buffer.size())
		{
			auto result = decode(buffer, i);
			if (result and not utf8::is_combining_codepoint(result.value().codepoint))
				count += 1;
			i += result ? result->bytes_consumed : 1;
		}
		return count;
	}

    std::optional<size_t> length(std::span<const std::byte> buffer)
	{
      if (buffer.empty())
          return 0uz;

     const u8* ptr    = utf8::u8_data(buffer);
		const u8* endptr = ptr + buffer.size();
		size_t    len{};
		bool      valid = true;

      for (; (ptr < endptr) and valid; len++)
		{
         const auto remaining = static_cast<size_t>(endptr - ptr);
			valid = (*ptr & 0x80) == 0 or
					(remaining >= 2 and (*ptr & 0xE0) == 0xC0 and (*(ptr + 1) & 0xC0) == 0x80) or
					(remaining >= 3 and (*ptr & 0xF0) == 0xE0 and (*(ptr + 1) & 0xC0) == 0x80 and (*(ptr + 2) & 0xC0) == 0x80) or
					(remaining >= 4 and (*ptr & 0xF8) == 0xF0 and (*(ptr + 1) & 0xC0) == 0x80 and (*(ptr + 2) & 0xC0) == 0x80 and
					 (*(ptr + 3) & 0xC0) == 0x80);

			i32 v1 = ((*ptr & 0x80) >> 7) & ((*ptr & 0x40) >> 6);
			i32 v2 = (*ptr & 0x20) >> 5;
			i32 v3 = (*ptr & 0x10) >> 4;
			ptr += 1 + ((v1 << v2) | (v1 & v3));
		}
		if (valid)
			return len;

		return {};
	}

   auto length(std::string_view buffer) { return length(utf8::as_ro_bytes(buffer)); }

  auto length(const char* str, u32 len) { return length(utf8::as_ro_bytes(str, len)); }

  std::expected<bool, std::string> valid(std::span<const std::byte> buffer)
	{
		size_t i = 0;
     while (i < buffer.size())
		{
         auto c         = utf8::u8_at(buffer, i);
			u32  codepoint = 0;

          if (utf8::is_single_byte(c))
			{
				i += 1;
				codepoint = c;
			}
			else if (utf8::is_two_byte_codepoint(c))
			{
              if (i + 1 >= buffer.size() or not utf8::is_continuation_byte(utf8::u8_at(buffer, i + 1)))
					return std::unexpected(std::format("Invalid or missing continuation byte at index {}", i));

             codepoint = (((c & 0x1F) << 6) | (utf8::u8_at(buffer, i + 1) & 0x3F));
				if (codepoint < 0x80)
					return std::unexpected(std::format("Overlong 2-byte encoding at index {}", i));

				i += 2;
			}
			else if (utf8::is_three_byte_codepoint(c))
			{
                if (i + 2 >= buffer.size() or not utf8::is_continuation_byte(utf8::u8_at(buffer, i + 1)) or
					not utf8::is_continuation_byte(utf8::u8_at(buffer, i + 2)))
					return std::unexpected(std::format("Invalid or missing continuation byte at index {}", i));

              codepoint = (((c & 0x0F) << 12) | ((utf8::u8_at(buffer, i + 1) & 0x3F) << 6) | (utf8::u8_at(buffer, i + 2) & 0x3F));
				if (codepoint < 0x800)
					return std::unexpected(std::format("Overlong 3-byte encoding at index {}", i));

				if (codepoint >= 0xD800 and codepoint <= 0xDFFF)
					return std::unexpected(std::format("UTF-8 encoded surrogate pair at index {}", i));

				i += 3;
			}
			else if (utf8::is_four_byte_codepoint(c))
			{
              if (i + 3 >= buffer.size() or not utf8::is_continuation_byte(utf8::u8_at(buffer, i + 1)) or
					not utf8::is_continuation_byte(utf8::u8_at(buffer, i + 2)) or not utf8::is_continuation_byte(utf8::u8_at(buffer, i + 3)))
					return std::unexpected(std::format("Invalid or missing continuation byte at index {}", i));

               codepoint = (((c & 0x07) << 18) | ((utf8::u8_at(buffer, i + 1) & 0x3F) << 12) | ((utf8::u8_at(buffer, i + 2) & 0x3F) << 6) |
							(utf8::u8_at(buffer, i + 3) & 0x3F));

				if (codepoint < 0x1'0000)
					return std::unexpected(std::format("Overlong 4-byte encoding at index {}", i));

				if (codepoint > 0x10'FFFF)
					return std::unexpected(std::format("Codepoint beyond U+10FFFF at index {}", i));

				i += 4;
			}
			else
			{
				return std::unexpected(std::format("Invalid leading byte at index {}", i));
			}
		}
		return true;
	}

	bool is_valid(std::string_view buffer)
	{
       auto ret = valid(utf8::as_ro_bytes(buffer));

		return ret.has_value() == true;
	}

	bool is_valid(const char* str, u32 len)
	{
       auto ret = valid(utf8::as_ro_bytes(str, len));

		return ret ? true : false;
	}

    char32 decode_codepoint(std::span<const std::byte> buffer, u32 index = 0)
	{
       assert::check(index < buffer.size(), "Index out-of-bounds");

     if (index >= buffer.size())
			return REPLACEMENT_CHARACTER;

		u32 state     = 0;
		u32 codepoint = 0;
     for (; index < buffer.size(); index++)
		{
         u8        byte = utf8::u8_at(buffer, index);
			const u32 type = utf8_table[byte];
			codepoint      = state ? (byte & 0x3fu) | (codepoint << 6) : (0xffu >> type) & byte;
			state          = utf8_table[256 + state + type];
			if (state == 0)
				return codepoint;
			else if (state == UTF8_REJECT)
				return REPLACEMENT_CHARACTER;
		}
		return REPLACEMENT_CHARACTER;
	}


} // namespace deckard::utf8
