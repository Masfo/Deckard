export module deckard.utf8:codepoints;
import :xid;
import :ascii;
import :basic_characters;

import std;
import deckard.as;
import deckard.types;

namespace deckard::utf8
{
	export constexpr bool is_single_byte(const u8 byte) { return (byte & 0x80) == 0; }

	export constexpr bool is_continuation_byte(const u8 byte) { return (byte & 0xC0) == 0x80; }

	export constexpr bool is_start_byte(const u8 rune) { return not is_continuation_byte(rune); }

	export constexpr bool is_start_of_codepoint(const u8 byte)
	{
		return is_single_byte(byte) or (((byte & 0xE0) == 0xC0) or ((byte & 0xF0) == 0xE0) or ((byte & 0xF8) == 0xF0));
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

	export constexpr bool is_bom(char32 codepoint) { return codepoint == 0xFEFF or codepoint == 0xFFFE; }

	export constexpr bool start_with_bom(const std::span<const u8> buffer)
	{
		return buffer.size() >= 3 and (buffer[0] == 0xEF and buffer[1] == 0xBB and buffer[2] == 0xBF);
	}

	export constexpr bool is_newline(char32 codepoint)
	{
		using namespace utf8::basic_characters;
		return (codepoint == LINE_FEED) or     // LF \n
			   (codepoint == CARRIAGE_RETURN); // RF \r
	}

	export constexpr bool is_whitespace(char32 codepoint)
	{
		// PropList-17.txt
		// Pattern_White_Space
		// White_Space

		return (
		  (codepoint == 0x0020) or                             // space										White_Space / Pattern_White_Space
		  ((codepoint >= 0x0009) and
		   (codepoint <= 0x000D)) or                           // control									White_Space / Pattern_White_Space
		  (codepoint == 0x0085) or                             // control									White_Space / Pattern_White_Space
		  (codepoint == 0x00A0) or                             // no-break space							White_Space /
		  (codepoint == 0x1680) or                             // Ogham space mark							White_Space /
		  ((codepoint >= 0x2000) and (codepoint <= 0x200A)) or // EN QUAD..HAIR SPACE						White_Space /
		  ((codepoint >= 0x200E) and
		   (codepoint <= 0x200F)) or // LEFT-TO-RIGHT MARK..RIGHT-TO-LEFT MARK                / Pattern_White_Space
		  (codepoint == 0x2028) or   // LINE SEPARATOR							White_Space / Pattern_White_Space
		  (codepoint == 0x2029) or   // PARAGRAPH SEPARATOR						White_Space / Pattern_White_Space
		  (codepoint == 0x202F) or   // NARROW NO-BREAK SPACE						White_Space /
		  (codepoint == 0x205F) or   // Medium Mathematical Space.				White_Space /
		  (codepoint == 0x3000));    // IDEOGRAPHIC SPACE							White_Space /
	}

	export constexpr bool is_digit(char32 codepoint)
	{
		// TODO: digit codepoints
		// Proplist: ASCII_Hex_Digit + Hex_Digit
		// UnicodeData: Nd
		return is_ascii_digit(codepoint);
	}

	export constexpr bool is_hex_digit(char32 codepoint) { return is_ascii_hex_digit(codepoint); }

	export constexpr bool is_identifier_start(char32 codepoint) { return is_ascii_identifier_start(codepoint) or is_xid_start(codepoint); }

	export constexpr bool is_identifier_continue(char32 codepoint)
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

		// Surrogates (U+D800–U+DFFF) are not valid Unicode codepoints
		if (cp >= 0xD800 and cp <= 0xDFFF)
		{
			ecp.bytes[0] = 0xEF;
			ecp.bytes[1] = 0xBF;
			ecp.bytes[2] = 0xBD;
			ecp.count    = 3;
			return ecp;
		}

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
