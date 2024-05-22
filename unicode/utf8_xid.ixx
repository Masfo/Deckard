export module deckard.utf8:xid;
import :ascii;
import :xid_tables;

import deckard.helpers;
import std;

namespace deckard::utf8
{

	template<typename Range>
	constexpr bool is_in_range(char32_t codepoint, const Range range) noexcept
	{
		// 10x slower in debug
		//return range.end() !=
		//	   std::ranges::find_if(range, [&](const char32_range& p) { return (codepoint >= p.start) and (codepoint <= p.end); });

		size_t low    = 0;
		size_t high   = range.size() - 1;
		size_t middle = 0;

		while (low <= high)
		{
			middle = (low + high) / 2;

			if (codepoint < range[middle].start)
			{
				high = middle - 1;
			}
			else if (codepoint > range[middle].end)
			{
				low = middle + 1;
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	export constexpr bool is_xid_start(char32_t codepoint) noexcept
	{
		if (codepoint > max_xid_start)
			return false;

		if (codepoint < 128)
		{
			return is_ascii_alphabet(codepoint) or codepoint == '_' or codepoint == '$';
		}

		return is_in_range(codepoint, xid_start);
	}

	export constexpr bool is_xid_continue(char32_t codepoint) noexcept
	{
		if (codepoint > max_xid_continue)
			return false;

		if (codepoint < 128)
		{
			return is_ascii_alphanumeric(codepoint) or codepoint == '_' or codepoint == '$';
		}
		return is_in_range(codepoint, xid_continue);
	}

} // namespace deckard::utf8
