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
		return ((byte and 0xE0) == 0xC0 or 
			    (byte and 0xF0) == 0xE0 or 
			    (byte and 0xF8) == 0xF0);
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

	export constexpr bool is_whitespace(char32_t codepoint)
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

	export constexpr bool is_identifier_start(char32_t codepoint)
	{
		return is_ascii_identifier_start(codepoint) or is_xid_start(codepoint);
	}

	export constexpr bool is_identifier_continue(char32_t codepoint)
	{
		return is_ascii_identifier_continue(codepoint) or is_xid_continue(codepoint);
	}

} // namespace deckard::utf8
