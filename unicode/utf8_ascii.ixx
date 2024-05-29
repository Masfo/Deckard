export module deckard.utf8:ascii;

export namespace deckard::utf8
{
	constexpr bool is_ascii(char32_t codepoint) noexcept { return codepoint < 128; }

	constexpr bool is_ascii_newline(char32_t codepoint) noexcept { return codepoint == '\n' or codepoint == '\r'; }

	constexpr bool is_ascii_uppercase(char32_t codepoint) noexcept { return ((codepoint >= 'A') and (codepoint <= 'Z')); }

	constexpr bool is_ascii_lowercase(char32_t codepoint) noexcept { return ((codepoint >= 'a') and (codepoint <= 'z')); }

	constexpr bool is_ascii_digit(char32_t codepoint) noexcept { return ((codepoint >= '0') and (codepoint <= '9')); }

	constexpr bool is_ascii_alphabet(char32_t codepoint) noexcept
	{
		return ((codepoint >= 'A') and (codepoint <= 'Z')) or ((codepoint >= 'a') and (codepoint <= 'z'));
	}

	constexpr bool is_ascii_hex_digit(char32_t codepoint) noexcept
	{
		return is_ascii_digit(codepoint) or ((codepoint >= 'A') and (codepoint <= 'F')) or ((codepoint >= 'a') and (codepoint <= 'f'));
	}

	constexpr bool is_ascii_binary_digit(char32_t codepoint) noexcept { return codepoint == '0' or codepoint == '1'; }

	constexpr bool is_ascii_alphanumeric(char32_t codepoint) noexcept
	{
		return ((codepoint >= 'A') and (codepoint <= 'Z')) or ((codepoint >= 'a') and (codepoint <= 'z')) or
			   ((codepoint >= '0') and (codepoint <= '9'));
	}

	constexpr bool is_ascii_identifier_start(char32_t codepoint) noexcept
	{
		return is_ascii_alphabet(codepoint) or codepoint == '_' or codepoint == '$';
	}

	constexpr bool is_ascii_identifier_continue(char32_t codepoint) noexcept
	{
		return is_ascii_alphanumeric(codepoint) or codepoint == '_' or codepoint == '$';
	}


} // namespace deckard::utf8
