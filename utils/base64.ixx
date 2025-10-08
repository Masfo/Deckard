export module deckard.base64;

import deckard.types;
import deckard.as;
import std;

namespace deckard::utils::base64
{
	constexpr u8 INVALID_SYMBOL = 0x64;

	static constexpr std::array<byte, 64> encode_table{
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
	  'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
	  's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};


	static constexpr std::array<u8, 256> decode_table = []() static constexpr
	{
		std::array<u8, 256> table{};
		table.fill(INVALID_SYMBOL);

		for (size_t i = 0; i < encode_table.size(); ++i)
			table[encode_table[i]] = as<u8>(i);

		return table;
	}();

	bool is_valid_base64_char(byte c) { return decode_table[c] != INVALID_SYMBOL; }

	bool is_valid_base64_str(std::string_view encoded_string)
	{
		if ((encoded_string.size() % 4) == 1)
			return false;
		if (!std::all_of(std::begin(encoded_string), std::end(encoded_string) - 2, [](byte c) { return is_valid_base64_char(c); }))
			return false;

		const auto last = std::rbegin(encoded_string);
		if (!is_valid_base64_char(*std::next(last)))
			return (*std::next(last) == '=') and (*last == '=');

		return is_valid_base64_char(*last) or (*last == '=');
	}

	export enum class padding { yes, no };

	export std::string encode(const std::span<const u8> input, padding add_padding = padding::yes)
	{
		if (input.empty())
			return {};


		std::string  encoded;
		const size_t input_size = input.size();
		encoded.reserve(((input_size + 2) / 3) * 4);

		std::uint32_t value = 0;
		std::size_t   bits  = 0;

		std::size_t i = 0;
		while (i < input_size)
		{
			value = (value << 8) | input[i++];
			bits += 8;

			while (bits >= 6)
			{
				bits -= 6;
				encoded += encode_table[(value >> bits) & 0x3F];
			}

			if (bits > 0)
				value &= ((1u << bits) - 1);
		}

		if (bits > 0)
			encoded += encode_table[(value << (6 - bits)) & 0x3F];

		while (add_padding == padding::yes and encoded.size() % 4 != 0)
			encoded += '=';

		return encoded;
	}

	export std::string encode_str(std::string_view input, padding add_padding = padding::yes)
	{
		return encode({std::bit_cast<u8*>(input.data()), input.size()}, add_padding);
	}

	// ########################################################################
	// Decode

	export std::optional<std::vector<u8>> decode(std::string_view encoded_input)
	{
		if (encoded_input.empty())
			return std::nullopt;

		std::vector<u8> out;
		out.reserve((encoded_input.size() / 4) * 3);

		u32 buf  = 0;
		i32 bits = 0;

		for (unsigned char c : encoded_input)
		{
			if (c == '=')
				break;

			u8 v = decode_table[c];
			if (v == 64)
				return {};

			buf = (buf << 6) | v;
			bits += 6;

			if (bits >= 8)
			{
				bits -= 8;
				out.push_back(as<u8>((buf >> bits) & 0xFF));

				if (bits > 0)
					buf &= ((1u << bits) - 1);
			}
		}

		// strict check
		// if(bits != 0)

		out.shrink_to_fit();
		return out;
	}

	export std::string decode_str(std::string_view encoded_input)
	{

		if (auto result = decode(encoded_input); result)
		{
			std::string r;
			r.reserve(result->size());
			std::ranges::copy(*result, std::back_inserter(r));
			return r;
		}
		return {};
	}


} // namespace deckard::utils::base64
