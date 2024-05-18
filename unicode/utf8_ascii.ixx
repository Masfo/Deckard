export module deckard.utf8:ascii;

export namespace deckard::utf8
{


	constexpr bool is_ascii_uppercase(char32_t codepoint) noexcept { return ((codepoint >= 'A') and (codepoint <= 'Z')); }

	constexpr bool is_ascii_lowercase(char32_t codepoint) noexcept { return ((codepoint >= 'a') and (codepoint <= 'z')); }

	constexpr bool is_digit(char32_t codepoint) noexcept { return ((codepoint >= '0') and (codepoint <= '9')); }

	constexpr bool is_ascii_alphabet(char32_t codepoint) noexcept
	{
		return ((codepoint >= 'A') and (codepoint <= 'Z')) or ((codepoint >= 'a') and (codepoint <= 'z'));
	}

	constexpr bool is_ascii_alphanumeric(char32_t codepoint) noexcept
	{
		return ((codepoint >= 'A') and (codepoint <= 'Z')) or ((codepoint >= 'a') and (codepoint <= 'z')) or
			   ((codepoint >= '0') and (codepoint <= '9'));
	}

	constexpr bool is_whitespace(char32_t codepoint)
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
		  (codepoint == 0x3000));                              // IDEOGRAPHIC SPACE
	}
} // namespace deckard::utf8
