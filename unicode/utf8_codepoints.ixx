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

	export constexpr auto is_combining_codepoint(char32_t cp) noexcept -> bool
	{
		constexpr std::pair<char32_t, char32_t> ranges[] = {
		  {0x0300, 0x036F},     // Combining diacritical marks
		  {0x1AB0, 0x1AFF},     // Extended
		  {0x1DC0, 0x1DFF},     // Supplement
		  {0x20D0, 0x20FF},     // Symbols
		  {0xFE00, 0xFE0F},     // Variation selectors  ← added
		  {0xFE20, 0xFE2F},     // Combining half marks
		  {0x1'D167, 0x1'D169}, // Musical marks
		  {0x1'E000, 0x1'E02F}, // Glagolitic supplement
		};

		return std::ranges::any_of(ranges, [cp](const auto& r) noexcept { return cp >= r.first && cp <= r.second; });
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

	export constexpr bool is_identifier_start(char32 codepoint)
	{
		return is_ascii_identifier_start(codepoint) or is_xid_start(codepoint);
	}

	export constexpr bool is_identifier_continue(char32 codepoint)
	{
		return is_ascii_identifier_continue(codepoint) or is_xid_continue(codepoint);
	}

	export bool is_valid_codepoint(char32 codepoint)
	{
		// return codepoint <= 0x10'FFFF and not(codepoint >= 0xD800 and codepoint <= 0xDFFF);
		return (codepoint <= 0x10'FFFF) && ((codepoint & 0xFFFF'F800) != 0xD800);
	}

	[[nodiscard]] constexpr auto is_variation_selector(uint32_t cp) noexcept -> bool
	{
		return (cp >= 0xFE00 && cp <= 0xFE0F)        // VS1–VS16
			   || (cp >= 0xE'0100 && cp <= 0xE'01EF) // VS17–VS256
			   || (cp >= 0x180B && cp <= 0x180D)     // Mongolian FVS1–3
			   || cp == 0x180F;                      // Mongolian FVS4
	}

	export bool is_skintone_modifier(char32 codepoint)
	{
		return codepoint >= 0x1'F3FB and
			   codepoint <= 0x1'F3FF; // EMOJI MODIFIER FITZPATRICK TYPE-1-2..EMOJI MODIFIER FITZPATRICK TYPE-6
	}

	export constexpr bool is_zero_width_non_joiner(char32 codepoint) { return codepoint == 0x200C; }

	export constexpr bool is_zero_width_joiner(char32 codepoint) { return codepoint == 0x200D; }

	export constexpr bool is_zero_width_codepoint(char32 codepoint)
	{
		return is_zero_width_joiner(codepoint) or is_zero_width_non_joiner(codepoint);
	}

	export bool is_regional_indicator(char32 codepoint)
	{
		return codepoint >= 0x1'F1E6 and
			   codepoint <= 0x1'F1FF; // REGIONAL INDICATOR SYMBOL LETTER A..REGIONAL INDICATOR SYMBOL LETTER Z
	}

	export bool is_combining_mark(char32 codepoint)
	{
		return (codepoint >= 0x0300 and codepoint <= 0x036F) or // Combining Diacritical Marks
			   (codepoint >= 0x1AB0 and codepoint <= 0x1AFF) or // Combining Diacritical Marks Extended
			   (codepoint >= 0x1DC0 and codepoint <= 0x1DFF) or // Combining Diacritical Marks Supplement
			   (codepoint >= 0x20D0 and codepoint <= 0x20FF) or // Combining Diacritical Marks for Symbols
			   (codepoint >= 0xFE20 and codepoint <= 0xFE2F);   // Combining Half Marks
	}

	[[nodiscard]] constexpr bool is_extended_pictographic(uint32_t cp) noexcept
	{
		return (cp == 0x00A9) || (cp == 0x00AE) || (cp >= 0x203C && cp <= 0x2B55) // misc symbols/dingbats region
			   || (cp >= 0x1'F000 && cp <= 0x1'F0FF)                              // mahjong/dominos
			   || (cp >= 0x1'F100 && cp <= 0x1'F1FF)                              // enclosed alphanumerics (incl. regional)
			   || (cp >= 0x1'F300 && cp <= 0x1'F9FF)                              // misc emoji
			   || (cp >= 0x1'FA00 && cp <= 0x1'FAFF);                             // extended symbols
	}

	[[nodiscard]] constexpr auto is_spacing_mark(uint32_t cp) noexcept -> bool
	{
		return (cp >= 0x0900 && cp <= 0x097F &&                                    // Devanagari vowel signs
				((cp >= 0x093E && cp <= 0x0940) || (cp >= 0x0949 && cp <= 0x094C) || cp == 0x094E || cp == 0x094F)) ||
			   (cp >= 0x0982 && cp <= 0x0983)                                      // Bengali
			   || (cp >= 0x0A02 && cp <= 0x0A03)                                   // Gurmukhi
			   || (cp >= 0x0A82 && cp <= 0x0A83)                                   // Gujarati
			   || (cp >= 0x0B02 && cp <= 0x0B03)                                   // Oriya
			   || (cp >= 0x0BBE && cp <= 0x0BBF)                                   // Tamil
			   || (cp >= 0x0BC1 && cp <= 0x0BC2) || (cp >= 0x0C01 && cp <= 0x0C03) // Telugu
			   || (cp >= 0x0C82 && cp <= 0x0C83)                                   // Kannada
			   || (cp >= 0x0D02 && cp <= 0x0D03)                                   // Malayalam
			   || (cp >= 0x0D3E && cp <= 0x0D40) || cp == 0x0E33                   // Thai sara am
			   || cp == 0x0EB3;                                                    // Lao sara am
	}

	export bool is_grapheme_extend(char32 codepoint)
	{
		return is_combining_mark(codepoint) or is_zero_width_joiner(codepoint) or is_variation_selector(codepoint) or
			   is_skintone_modifier(codepoint) or is_regional_indicator(codepoint);
	}

	[[nodiscard]] constexpr bool is_tag_character(uint32_t cp) noexcept
	{
		return cp >= 0xE'0000 && cp <= 0xE'007F; // includes CANCEL TAG at E007F
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

