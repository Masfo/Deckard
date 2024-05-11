export module deckard.utf8;


import std;
import deckard.types;
import deckard.assert;
import deckard.debug;

namespace deckard::utf8
{
	const u8 utf8d[] = {
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
	  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,
	  8,  8,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
	  10, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,

	  0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 0,  12, 12, 12, 12, 12, 0,
	  12, 0,  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12,
	  12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12,
	  12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
	};

	constexpr u32 UTF8_ACCEPT{0};
	constexpr u32 UTF8_REPLACEMENT_CHARACTER{0xFFFD};
	constexpr u32 UTF8_BOM{0xFEFF};

	export constexpr bool is_bom(u32 codepoint) noexcept
	{
		switch (codepoint)
		{
			case 0xFEFF:
			case 0xFFFE: return true;
			default: return false;
		}
	}

	export std::vector<u32> codepoints_from_utf8_string(const std::span<u8> input)
	{
		u32              codepoint_count{0};
		u32              state{UTF8_ACCEPT};
		u32              codepoint{0};
		std::vector<u32> ret;
		ret.reserve(input.size());

		const auto decode = [&state, &codepoint](u8 byte) -> u32
		{
			u32 type  = utf8d[byte];
			codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (codepoint << 6) : (0xff >> type) & (byte);
			state     = utf8d[256 + state + type];
			return state;
		};

		for (const u8& b : input)
		{
			if (!decode(b))
			{
				ret.emplace_back(codepoint);
				codepoint_count += 1;
			}
			else
			{
				// state = UTF8_ACCEPT;
				// dbg::println("Ill-formed codepoint U+{:0X}", b);
				// ret.emplace_back(UTF8_REPLACEMENT_CHARACTER);
			}
		}

		if (state != UTF8_ACCEPT)
		{
			dbg::println("Ill-formed utf8 stream");
			return {};
		}


		return ret;
	}

	export auto codepoints_from_utf8_string(const std::string_view input) noexcept
	{
		return codepoints_from_utf8_string(std::span{(u8*)input.data(), input.size()});
	}

} // namespace deckard::utf8
