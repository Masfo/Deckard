export module deckard.utf8:decode;

import :codepoints;

import std;
import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.helpers;

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
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
	  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  7,  7,  7,  7,  7,  7,  7,  7,
	  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,
	  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	  10, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,
	  8,  8,  8,  8,  0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	  12, 0,  12, 12, 12, 12, 12, 0,  12, 0,  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12,
	  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36,
	  12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	};
	constexpr u32 UTF8_ACCEPT{0};
	constexpr u32 UTF8_REJECT{12};

	constexpr char32 REPLACEMENT_CHARACTER{0xFFFD}; // U+FFFD,  0xEF 0xBF 0xBD,  (239, 191, 189) REPLACEMENT CHARACTER

	struct decode_result
	{
		char32 codepoint{};
		u32    bytes_consumed{};
	};

	[[nodiscard]] constexpr auto u8_at(std::span<const u8> s, size_t i) -> u8 { return static_cast<u8>(s[i]); }


} // namespace deckard::utf8

export namespace deckard::utf8
{


	decode_result decode_unchecked(std::span<const u8> buffer, size_t index) noexcept
	{
		u8 byte = u8_at(buffer, index);
		if (byte < 0x80)
			return {byte, 1};

		u32 state     = 0;
		u32 codepoint = 0;

		u32 type  = utf8_table[byte];
		codepoint = (0xffu >> type) & byte;
		state     = utf8_table[256 + state + type];

		if (state == UTF8_REJECT)
			return {REPLACEMENT_CHARACTER, 1}; // invalid lead byte

		u32 bytes_consumed = 1;
		for (index += 1; index < buffer.size(); ++index)
		{
			byte = u8_at(buffer, index);
			++bytes_consumed;

			type      = utf8_table[byte];
			codepoint = (byte & 0x3fu) | (codepoint << 6);
			state     = utf8_table[256 + state + type];

			if (state == UTF8_ACCEPT)
				return {codepoint, bytes_consumed};
			if (state == UTF8_REJECT)
				return {REPLACEMENT_CHARACTER, 1};
		}

		// Incomplete sequence
		return {REPLACEMENT_CHARACTER, bytes_consumed};
	}

	auto decode(std::span<const u8> buffer, size_t index) noexcept -> std::optional<decode_result>
	{
		if (index >= buffer.size())
			return std::nullopt;
		return decode_unchecked(buffer, index);
	}

	std::generator<char32> yield_codepoints(std::span<const u8> buffer)
	{
		size_t i = 0;
		while (i < buffer.size())
		{
			auto [codepoint, bytes_consumed] = decode_unchecked(buffer, i);
			co_yield codepoint;
			i += bytes_consumed;
		}
	}

	export size_t grapheme_count(std::span<const u8> buffer)
	{

		std::size_t count = 0;

		bool prev_was_ZWJ      = false;
		bool prev_was_regional = false;
		bool in_regional_pair  = false;

		size_t i = 0;
		while (i < buffer.size())
		{
			auto [cp, consumed] = utf8::decode_unchecked(buffer, i);
			i += consumed;

			const bool is_extend =
			  is_variation_selector(cp) or is_skintone_modifier(cp) or is_zero_width_non_joiner(cp) or
			  is_combining_mark(cp) or is_spacing_mark(cp) or is_tag_character(cp);


			if (count == 0)
			{
				++count;
			}
			else if (prev_was_ZWJ and is_extended_pictographic(cp))
			{
				// continue cluster
			}
			else if (is_zero_width_joiner(cp) or is_extend) // GB9: Extend or ZWJ never breaks (continues current cluster)
			{
				// continue cluster
			}
			else if (is_regional_indicator(cp)) // GB12/GB13: Regional Indicators pair up
			{
				if (prev_was_regional and not in_regional_pair)
				{
					// second RI completes the flag — continue cluster
					in_regional_pair = true;
				}
				else
				{
					// either first RI, or third RI (starts fresh flag cluster)
					++count;
					in_regional_pair = false;
				}
			}
			// Everything else: new cluster
			else
			{
				++count;
				in_regional_pair = false;
			}

			prev_was_ZWJ      = is_zero_width_joiner(cp);
			prev_was_regional = is_regional_indicator(cp);
			if (not is_regional_indicator(cp))
				in_regional_pair = false;
		}

		return count;
	}

	std::optional<size_t> length(std::span<const u8> buffer)
	{
		if (buffer.empty())
			return 0uz;

		size_t i = 0;
		size_t len{};

		while (i < buffer.size())
		{
			auto c         = u8_at(buffer, i);
			u32  codepoint = 0;

			if (utf8::is_single_byte(c))
			{
				i += 1;
				codepoint = c;
			}
			else if (utf8::is_two_byte_codepoint(c))
			{
				if (i + 1 >= buffer.size() or not utf8::is_continuation_byte(u8_at(buffer, i + 1)))
				{
					dbg::println("Invalid or missing continuation byte at offset {}", i);
					return std::nullopt;
				}

				codepoint = (((c & 0x1F) << 6) | (u8_at(buffer, i + 1) & 0x3F));
				if (codepoint < 0x80)
				{
					dbg::println("Overlong 2-byte encoding at offset {}", i);
					return std::nullopt;
				}

				i += 2;
			}
			else if (utf8::is_three_byte_codepoint(c))
			{
				if (i + 2 >= buffer.size() or not utf8::is_continuation_byte(u8_at(buffer, i + 1)) or
					not utf8::is_continuation_byte(u8_at(buffer, i + 2)))
				{
					dbg::println("Invalid or missing continuation byte at offset {}", i);
					return std::nullopt;
				}

				codepoint = (((c & 0x0F) << 12) | ((u8_at(buffer, i + 1) & 0x3F) << 6) | (u8_at(buffer, i + 2) & 0x3F));
				if (codepoint < 0x800)
				{
					dbg::println("Overlong 3-byte encoding at offset {}", i);
					return std::nullopt;
				}

				if (codepoint >= 0xD800 and codepoint <= 0xDFFF)
				{
					dbg::println("UTF-8 encoded surrogate pair at offset {}", i);
					return std::nullopt;
				}

				i += 3;
			}
			else if (utf8::is_four_byte_codepoint(c))
			{
				if (i + 3 >= buffer.size() or not utf8::is_continuation_byte(u8_at(buffer, i + 1)) or
					not utf8::is_continuation_byte(u8_at(buffer, i + 2)) or
					not utf8::is_continuation_byte(u8_at(buffer, i + 3)))
				{
					dbg::println("Invalid or missing continuation byte at offset {}", i);
					return std::nullopt;
				}

				codepoint = (((c & 0x07) << 18) | ((u8_at(buffer, i + 1) & 0x3F) << 12) |
							 ((u8_at(buffer, i + 2) & 0x3F) << 6) | (u8_at(buffer, i + 3) & 0x3F));

				if (codepoint < 0x1'0000)
				{
					dbg::println("Overlong 4-byte encoding at offset {}", i);
					return std::nullopt;
				}

				if (codepoint > 0x10'FFFF)
				{
					dbg::println("Codepoint beyond U+10FFFF at offset {}", i);
					return std::nullopt;
				}

				i += 4;
			}
			else
			{
				dbg::println("Invalid leading byte at offset {}", i);
				return std::nullopt;
			}

			++len;
		}

		return std::optional<size_t>{len};
	}

	std::optional<size_t> length(std::string_view buffer) { return length(to_span(buffer)); }

	// std::optional<size_t> length(const char* str, u32 len) { return length({as<const u8*>(str), len}); }

	std::expected<void, std::string> valid(std::span<const u8> buffer)
	{
		auto location_at_index = [&](const size_t index)
		{
			size_t line   = 1;
			size_t column = 1;
			for (size_t pos = 0; pos < index and pos < buffer.size(); pos++)
			{
				if (u8_at(buffer, pos) == static_cast<u8>('\n'))
				{
					line += 1;
					column = 1;
				}
				else
				{
					column += 1;
				}
			}
			return std::pair{line, column};
		};

		size_t i = 0;
		while (i < buffer.size())
		{
			auto c         = u8_at(buffer, i);
			u32  codepoint = 0;

			if (utf8::is_single_byte(c))
			{
				i += 1;
				codepoint = c;
			}
			else if (utf8::is_two_byte_codepoint(c))
			{
				if (i + 1 >= buffer.size() or not utf8::is_continuation_byte(u8_at(buffer, i + 1)))
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(
					  std::format("Invalid or missing continuation byte at line {}, column {}", line, column));
				}

				codepoint = (((c & 0x1F) << 6) | (u8_at(buffer, i + 1) & 0x3F));
				if (codepoint < 0x80)
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(std::format("Overlong 2-byte encoding at line {}, column {}", line, column));
				}

				i += 2;
			}
			else if (utf8::is_three_byte_codepoint(c))
			{
				if (i + 2 >= buffer.size() or not utf8::is_continuation_byte(u8_at(buffer, i + 1)) or
					not utf8::is_continuation_byte(u8_at(buffer, i + 2)))
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(
					  std::format("Invalid or missing continuation byte at line {}, column {}", line, column));
				}

				codepoint = (((c & 0x0F) << 12) | ((u8_at(buffer, i + 1) & 0x3F) << 6) | (u8_at(buffer, i + 2) & 0x3F));
				if (codepoint < 0x800)
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(std::format("Overlong 3-byte encoding at line {}, column {}", line, column));
				}

				if (codepoint >= 0xD800 and codepoint <= 0xDFFF)
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(std::format("UTF-8 encoded surrogate pair at line {}, column {}", line, column));
				}

				i += 3;
			}
			else if (utf8::is_four_byte_codepoint(c))
			{
				if (i + 3 >= buffer.size() or not utf8::is_continuation_byte(u8_at(buffer, i + 1)) or
					not utf8::is_continuation_byte(u8_at(buffer, i + 2)) or
					not utf8::is_continuation_byte(u8_at(buffer, i + 3)))
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(
					  std::format("Invalid or missing continuation byte at line {}, column {}", line, column));
				}

				codepoint = (((c & 0x07) << 18) | ((u8_at(buffer, i + 1) & 0x3F) << 12) |
							 ((u8_at(buffer, i + 2) & 0x3F) << 6) | (u8_at(buffer, i + 3) & 0x3F));

				if (codepoint < 0x1'0000)
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(std::format("Overlong 4-byte encoding at line {}, column {}", line, column));
				}

				if (codepoint > 0x10'FFFF)
				{
					auto [line, column] = location_at_index(i);
					return std::unexpected(std::format("Codepoint beyond U+10FFFF at line {}, column {}", line, column));
				}

				i += 4;
			}
			else
			{
				auto [line, column] = location_at_index(i);
				return std::unexpected(std::format("Invalid leading byte at line {}, column {}", line, column));
			}
		}
		return {};
	}

	bool is_valid(std::string_view buffer)
	{
		auto ret = valid(to_span(buffer));

		return ret.has_value() == true;
	}

	// bool is_valid(const char* str, u32 len)
	//{
	//	auto ret = valid(utf8::as_ro_bytes(str, len));
	//
	//	return ret ? true : false;
	// }

	char32 decode_codepoint(std::span<const u8> buffer, u32 index = 0)
	{
		assert::check(index < buffer.size(), "Index out-of-bounds");

		if (index >= buffer.size())
			return REPLACEMENT_CHARACTER;

		u32 state     = 0;
		u32 codepoint = 0;
		for (; index < buffer.size(); index++)
		{
			u8        byte = u8_at(buffer, index);
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

	char32 decode_codepoint(std::string_view buffer) { return decode_codepoint(to_span(buffer)); }



} // namespace deckard::utf8
