export module deckard.base32;

import deckard.types;
import deckard.as;
import std;

namespace deckard::utils::base32
{
	static constexpr std::array<byte, 32> encode_table{
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7'};

	static constexpr std::array<u8, 256> decode_table{
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0-15
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 16-31
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 32-47
	  0xFF, 0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 48-63
	  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, // 64-79
	  0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 80-95
	  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, // 96-111
	  0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 112-127
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 128-143
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 144-159
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 160-175
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 176-191
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 192-207
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 208-223
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 224-239
	  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // 240-255
	};

	constexpr std::array<byte, 4> encode_three(u8 a, u8 b, u8 c)
	{
		const u32 concat_bits = (a << 16) | (b << 8) | c;

		const byte c1 = encode_table[(concat_bits >> 18) & 0x3F];
		const byte c2 = encode_table[(concat_bits >> 12) & 0x3F];
		const byte c3 = encode_table[(concat_bits >> 6) & 0x3F];
		const byte c4 = encode_table[concat_bits & 0x3F];
		return {c1, c2, c3, c4};
	}

	constexpr std::array<u8, 3> decode_four(byte a, byte b, byte c, byte d)
	{
		const u32 bytes = (decode_table[a] << 18) | (decode_table[b] << 12) | (decode_table[c] << 6) | decode_table[d];

		const byte b1 = (bytes >> 16) & 0xFF;
		const byte b2 = (bytes >> 8) & 0xFF;
		const byte b3 = bytes & 0xFF;
		return {b1, b2, b3};
	}

	bool is_valid_base64_char(byte c) { return decode_table[c] != 0x64; }

	bool is_valid_base64_str(std::string_view encoded_string)
	{
		if ((encoded_string.size() % 4) == 1)
			return false;
		if (!std::all_of(std::begin(encoded_string), std::end(encoded_string) - 2, [](byte c) { return is_valid_base64_char(c); }))
			return false;

		const auto last = std::rbegin(encoded_string);
		if (!is_valid_base64_char(*std::next(last)))
			return (*std::next(last) == '=') && (*last == '=');

		return is_valid_base64_char(*last) || (*last == '=');
	}

	export enum class padding { yes, no };

	export std::string encode(const std::span<const u8> input, padding add_padding = padding::yes)
	{
		const auto size   = input.size();
		const auto blocks = size / 3ULL;

		std::string output;
		output.reserve((blocks + 2) * 4);

		for (u64 i = 0; i < blocks; ++i)
		{
			const auto triplet      = input.subspan(i * 3, 3);
			const auto base64_chars = encode_three(triplet[0], triplet[1], triplet[2]);
			std::ranges::copy(base64_chars, std::back_inserter(output));
		}

		if (const auto remaining = size - blocks * 3; remaining == 2)
		{
			const auto last         = input.last(2);
			const auto base64_chars = encode_three(last[0], last[1], 0);
			output.push_back(base64_chars[0]);
			output.push_back(base64_chars[1]);
			output.push_back(base64_chars[2]);
			if (add_padding == padding::yes)
			{
				output.push_back('=');
			}
		}
		else if (remaining == 1)
		{
			const auto base64_chars = encode_three(input.back(), 0, 0);
			output.push_back(base64_chars[0]);
			output.push_back(base64_chars[1]);
			if (add_padding == padding::yes)
			{
				output.push_back('=');
				output.push_back('=');
			}
		}

		output.shrink_to_fit();

		return output;
	}

	export std::string encode_str(std::string_view input, padding add_padding = padding::yes)
	{
		return encode({std::bit_cast<u8*>(input.data()), input.size()}, add_padding);
	}

	export std::optional<std::vector<u8>> decode(std::string_view encoded_input)
	{
		if (encoded_input.empty())
			return std::nullopt;

		if (!is_valid_base64_str(encoded_input))
			return std::nullopt;

		const auto unpadded_input = encoded_input.substr(0, encoded_input.find_first_of('='));
		const auto blocks         = unpadded_input.size() / 4;

		std::vector<u8> output;
		output.reserve(((blocks + 2) * 3) / 4);

		for (u64 i = 0; i < blocks; ++i)
		{
			const auto four  = unpadded_input.substr(i * 4, 4);
			const auto bytes = decode_four(four[0], four[1], four[2], four[3]);
			std::ranges::copy(bytes, std::back_inserter(output));
		}

		if (const auto last = unpadded_input.substr(blocks * 4); last.size() == 0)
		{
			return output;
		}
		else if (last.size() == 2 || (last[2] == '='))
		{
			const auto bytes = decode_four(last[0], last[1], 'A', 'A');
			output.emplace_back(bytes[0]);
		}
		else
		{
			const auto bytes = decode_four(last[0], last[1], last[2], 'A');
			std::ranges::copy_n(bytes.begin(), 2, std::back_inserter(output));
		}

		output.shrink_to_fit();
		return output;
	}

	export std::string decode_str(std::string_view encoded_input)
	{
		auto result = decode(encoded_input);
		if (result)
		{
			std::string r;
			r.reserve(result->size());
			std::ranges::copy(*result, std::back_inserter(r));
			return r;
		}
		return {};
	}

} // namespace deckard::utils::base32
