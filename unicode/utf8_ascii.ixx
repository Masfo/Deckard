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


} // namespace deckard::utf8
