export module deckard.utf8:codepoints;
import :xid;

import std;
import deckard.as;
import deckard.types;
import deckard.assert;
import deckard.debug;

namespace deckard::utf8
{
	export constexpr bool is_single_byte(const u8 byte) { return (byte & 0x80) == 0; }

	export constexpr bool is_start_byte(const u8 rune) { return ((rune & 0xc0) == 0xc0 or is_single_byte(rune)); }

	export constexpr bool is_continuation_byte(const u8 byte) { return (byte & 0xC0) == 0x80; }

	export constexpr bool is_start_of_codepoint(const u8 byte)
	{
		return (((byte & 0xE0) == 0xC0) or ((byte & 0xF0) == 0xE0) or ((byte & 0xF8) == 0xF0));
	}

	export constexpr bool is_two_byte_codepoint(const u8 byte) { return ((byte >> 5) == 0x6); }

	export constexpr bool is_three_byte_codepoint(const u8 byte) { return ((byte >> 4) == 0xE); }

	export constexpr bool is_four_byte_codepoint(const u8 byte) { return ((byte >> 3) == 0x1E); }

	export constexpr bool is_combining_codepoint(const char32 cp)
	{
		return (
		  (cp >= 0x0300 and cp <= 0x036F) or     // Combining diacritical marks
		  (cp >= 0x1AB0 and cp <= 0x1AFF) or     // Extended
		  (cp >= 0x1DC0 and cp <= 0x1DFF) or     // Supplement
		  (cp >= 0x20D0 and cp <= 0x20FF) or     // Symbols
		  (cp >= 0xFE20 and cp <= 0xFE2F) or     // Half marks
		  (cp >= 0x1'E000 and cp <= 0x1'E02F) or // Glagolitic
		  (cp >= 0x1'D167 and cp <= 0x1'D169)    // Musical marks
		);
	}

	export constexpr u32 codepoint_width(u8 codepoint_byte)
	{
		if (codepoint_byte < 0x80)
			return 1;
		else if ((codepoint_byte & 0xE0) == 0xC0)
			return 2;
		else if ((codepoint_byte & 0xF0) == 0xE0)
			return 3;
		else if ((codepoint_byte & 0xF8) == 0xF0)
			return 4;
#if 0
		else if ((codepoint_byte & 0xFC) == 0xF8)
			return 5;
		else if ((codepoint_byte & 0xFE) == 0xFC)
			return 6;
#endif


		return 0;
	}

	export constexpr u32 codepoint_width(char32_t codepoint)
	{
		if (codepoint < 0x80)
			return 1;
		else if (codepoint < 0x800)
			return 2;
		else if (codepoint < 0x1'0000)
			return 3;
		else if (codepoint < 0x11'0000)
			return 4;

		return 0;
	}

	export constexpr bool is_bom(char32_t codepoint) { return codepoint == 0xFEFF or codepoint == 0xFFFE; }

	export constexpr bool start_with_bom(const std::span<u8> buffer)
	{
		return buffer.size() >= 3 and (buffer[0] == 0xEF and buffer[1] == 0xBB and buffer[2] == 0xBF);
	}

	export constexpr bool is_whitespace(char32_t codepoint)
	{
		// PropList-16.txt
		// Pattern_White_Space
		// White_Space

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
		  (codepoint >= 0x205F) or                             // Medium Mathematical Space.
		  (codepoint == 0x3000));                              // IDEOGRAPHIC SPACE
	}

	export constexpr bool is_identifier_start(char32_t codepoint)
	{
		return is_ascii_identifier_start(codepoint) or is_xid_start(codepoint);
	}

	export constexpr bool is_identifier_continue(char32_t codepoint)
	{
		return is_ascii_identifier_continue(codepoint) or is_xid_continue(codepoint);
	}

	export struct EncodedCodepoint
	{
		std::array<u8, 4> bytes{0};
		u8                count{0};
	};

	export EncodedCodepoint encode_codepoint(char32 cp)
	{
		EncodedCodepoint ecp;

		if (cp <= 0x7F)
		{

			ecp.bytes[0] = (static_cast<u8>(cp));
			ecp.count    = 1;
		}
		else if (cp <= 0x7FF)
		{
			ecp.bytes[0] = (static_cast<u8>((cp >> 6) | 0xC0));
			ecp.bytes[1] = (static_cast<u8>((cp & 0x3F) | 0x80));
			ecp.count    = 2;
		}
		else if (cp <= 0xFFFF)
		{
			ecp.bytes[0] = (static_cast<u8>((cp >> 12) | 0xE0));
			ecp.bytes[1] = (static_cast<u8>(((cp >> 6) & 0x3F) | 0x80));
			ecp.bytes[2] = (static_cast<u8>((cp & 0x3F) | 0x80));
			ecp.count    = 3;
		}
		else if (cp <= 0x10'FFFF)
		{
			ecp.bytes[0] = (static_cast<u8>((cp >> 18) | 0xF0));
			ecp.bytes[1] = (static_cast<u8>(((cp >> 12) & 0x3F) | 0x80));
			ecp.bytes[2] = (static_cast<u8>(((cp >> 6) & 0x3F) | 0x80));
			ecp.bytes[3] = (static_cast<u8>((cp & 0x3F) | 0x80));
			ecp.count    = 4;
		}
		else
		{
			ecp.bytes[0] = (0xEF);
			ecp.bytes[1] = (0xBF);
			ecp.bytes[2] = (0xBD); // U+FFFD replacement character
			ecp.count    = 3;
		}
		return ecp;
	}


} // namespace deckard::utf8
