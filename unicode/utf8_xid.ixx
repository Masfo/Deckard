export module deckard.utf8:xid;

import :ascii;

namespace deckard::utf8
{

	export constexpr bool is_xid_start(char32_t codepoint) noexcept
	{
		if (codepoint < 128)
		{
			return is_ascii_alphabet(codepoint) or codepoint == '_' or codepoint == '$';
		}

		(codepoint);
		// xid_start
		return false;
	}

	export constexpr bool is_xid_continue(char32_t codepoint) noexcept
	{
		if (codepoint < 128)
		{
			return is_ascii_alphanumeric(codepoint) or codepoint == '_' or codepoint == '$';
		}
		(codepoint);
		// xid_continue
		return false;
	}

} // namespace deckard::utf8
