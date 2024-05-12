export module deckard.utf8;


import std;
import deckard.types;
import deckard.assert;
import deckard.debug;

namespace deckard::utf8
{
	const u8 utf8d[] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 00..1f
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 20..3f
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 40..5f
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   // 60..7f
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,   // 80..9f
	  7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,
	  7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   // a0..bf
	  8,   8,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
	  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   // c0..df
	  0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, // e0..ef
	  0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, // f0..ff
	  0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1, // s0..s0
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   0,   1,   1,   1,   1,   1,   0,   1,   0,   1,   1,   1,   1,   1,   1,   // s1..s2
	  1,   2,   1,   1,   1,   1,   1,   2,   1,   2,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,   1,   1,   // s3..s4
	  1,   2,   1,   1,   1,   1,   1,   1,   1,   2,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1,   // s5..s6
	  1,   3,   1,   1,   1,   1,   1,   3,   1,   3,   1,   1,   1,   1,   1,   1,
	  1,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   // s7..s8
	};

	constexpr u32 UTF8_ACCEPT{0};
	constexpr u32 UTF8_REJECT{1};

	constexpr u32 UTF8_REPLACEMENT_CHARACTER{0xFFFD};

	// TODO: is_digit
	// TODO: is_whitespace
	// TODO: is_xid_start, is_xid_continue

	export class decoder
	{
	public:
		decoder(std::string_view input) { buffer = std::span{(u8*)input.data(), input.size()}; }

		decoder(std::span<u8> input)
			: buffer(input)
		{
		}

		std::optional<u32> next()
		{
			if (index >= buffer.size())
				return {};

			u8 byte = 0;

			for (state = 0; index < buffer.size(); index++)
			{
				byte = buffer[index];


				auto leading_ones = std::countl_one(byte);
				switch (leading_ones)
				{
					case 0: ascii_bytes++; break;
					case 1: continuation_bytes++; break;
					case 2: two_bytes++; break;
					case 3: three_bytes++; break;
					case 4: four_bytes++; break;
					default: dbg::panic("utf8 not handled");
				}


				if (!read(byte))
				{
					index += 1;
					return codepoint;
				}
				else if (state == UTF8_REJECT)
				{
					u32 skip = state / 3 + 1;
					index += 1 + skip;
					invalid_bytes += 1;

					return UTF8_REPLACEMENT_CHARACTER;
				}
			}


			if (state != UTF8_ACCEPT)
			{
				state = UTF8_ACCEPT;
				return UTF8_REPLACEMENT_CHARACTER;
			}

			return codepoint;
		}

	private:
		u32 read(u8 byte) noexcept
		{
			u32 type = utf8d[byte];

			codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (codepoint << 6) : (0xff >> type) & (byte);
			state     = utf8d[256 + state * 16 + type];

			// codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3fu) | (codepoint << 6) : (0xff >> type) & (byte);
			// state     = utf8d[256 + state + type];
			return state;
		}

	public:
		u32 ascii_bytes{0};
		u32 continuation_bytes{0};
		u32 two_bytes{0};
		u32 three_bytes{0};
		u32 four_bytes{0};
		u32 invalid_bytes{0};

	private:
		std::span<u8> buffer;
		u32           index{0};
		u32           prev{0};
		u32           codepoint{0};
		u32           state{UTF8_ACCEPT};
	};

	u32 decode(u32* state, u32* codepoint, u8 byte) noexcept
	{
		u32 type   = utf8d[byte];
		*codepoint = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codepoint << 6) : (0xff >> type) & (byte);
		//*state     = utf8d[256 + *state + type];
		*state = utf8d[256 + *state * 16 + type];

		return *state;
	}

	export constexpr bool is_bom(u32 codepoint) noexcept { return codepoint == 0xFEFF or codepoint == 0xFFFE; }

	export u32 count_codepoints(const std::span<u8> input) noexcept
	{
		u32 codepoint_count{0};
		u32 codepoint{0};
		u32 state{UTF8_ACCEPT};

		for (const u8& b : input)
		{
			if (!decode(&state, &codepoint, b))
				codepoint_count += 1;
		}

		return state == UTF8_ACCEPT ? codepoint_count : 0;
	}

	export auto count_codepoints(const std::string_view input) noexcept
	{
		return count_codepoints(std::span{(u8*)input.data(), input.size()});
	}

	export std::vector<u32> codepoints(const std::span<u8> input) noexcept
	{
		u32              codepoint_count{0};
		u32              state{UTF8_ACCEPT};
		u32              codepoint{0};
		std::vector<u32> ret;
		ret.reserve(input.size());


		for (const u8& b : input)
		{
			if (!decode(&state, &codepoint, b))
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

	export auto codepoints(const std::string_view input) noexcept { return codepoints(std::span{(u8*)input.data(), input.size()}); }

} // namespace deckard::utf8
