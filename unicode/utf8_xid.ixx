export module deckard.utf8:xid;
import :ascii;
import :xid_tables;

import deckard.debug;
import deckard.types;
import deckard.helpers;
import std;

namespace deckard::utf8
{

	template<size_t N>
	bool is_in_range(char32_t codepoint, const std::array<char32_range, N>& range)
	{
#if 1
		// ~20-40% faster (depends on input) than lower_bound
		size_t low    = 0;
		size_t high   = range.size() - 1;
		size_t middle = 0;

		while (low <= high)
		{
			middle = low + ((high - low) / 2);

			if (codepoint < range[middle].start)
				high = middle - 1;
			else if (codepoint > range[middle].end)
				low = middle + 1;
			else
			{
				return true;
			}
		}
		return false;

#else
		auto it = std::lower_bound(
		  range.begin(),
		  range.end(),
		  codepoint,
		  [](const char32_range& r, const char32_t& value) //
		  {                                                //
			  return value > r.end;
		  });
		return (codepoint >= (*it).start) and (codepoint <= (*it).end);
#endif
	}

	export constexpr bool is_xid_start(char32_t codepoint)
	{
		if (codepoint > max_xid_start)
			return false;

		if (codepoint < 128)
		{
			return is_ascii_identifier_start(codepoint);
		}

		return is_in_range(codepoint, xid_start);
	}

	export constexpr bool is_xid_continue(char32_t codepoint)
	{
		if (codepoint > max_xid_continue)
			return false;

		if (codepoint < 128)
		{
			return is_ascii_identifier_continue(codepoint);
		}
		return is_in_range(codepoint, xid_continue);
	}

} // namespace deckard::utf8
