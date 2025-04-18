export module deckard.utf8:ascii;

import deckard.types;

export namespace deckard::utf8
{
	constexpr bool is_ascii(char32 codepoint) { return codepoint < 128; }

	constexpr bool is_ascii_newline(char32 codepoint) { return codepoint == '\n' or codepoint == '\r'; }

	constexpr bool is_ascii_uppercase(char32 codepoint) { return ((codepoint >= 'A') and (codepoint <= 'Z')); }

	constexpr bool is_ascii_lowercase(char32 codepoint) { return ((codepoint >= 'a') and (codepoint <= 'z')); }

	constexpr bool is_ascii_digit(char32 codepoint) { return ((codepoint >= '0') and (codepoint <= '9')); }

	constexpr bool is_ascii_alphabet(char32 codepoint)
	{
		return ((codepoint >= 'A') and (codepoint <= 'Z')) or ((codepoint >= 'a') and (codepoint <= 'z'));
	}

	constexpr bool is_ascii_hex_digit(char32 codepoint)
	{
		return is_ascii_digit(codepoint) or ((codepoint >= 'A') and (codepoint <= 'F')) or ((codepoint >= 'a') and (codepoint <= 'f'));
	}

	constexpr bool is_ascii_binary_digit(char32 codepoint) { return codepoint == '0' or codepoint == '1'; }

	constexpr bool is_ascii_alphanumeric(char32 codepoint)
	{
		return ((codepoint >= 'A') and (codepoint <= 'Z')) or ((codepoint >= 'a') and (codepoint <= 'z')) or
			   ((codepoint >= '0') and (codepoint <= '9'));
	}

	constexpr bool is_ascii_identifier_start(char32 codepoint)
	{
		return is_ascii_alphabet(codepoint) or codepoint == '_' or codepoint == '$';
	}

	constexpr bool is_ascii_identifier_continue(char32 codepoint)
	{
		return is_ascii_alphanumeric(codepoint) or codepoint == '_' or codepoint == '$';
	}

	constexpr size_t ascii_hex_to_int(char codepoint)
	{
		if (is_ascii_digit(codepoint))
			return codepoint - '0';
		else if (codepoint >= 'A' and codepoint <= 'F')
			return codepoint - 'A' + 10;
		else if (codepoint >= 'a' and codepoint <= 'f')
			return codepoint - 'a' + 10;
		return 0;
	}

} // namespace deckard::utf8
