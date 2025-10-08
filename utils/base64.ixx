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

	constexpr std::array<u8, 3> decode_four(byte a, byte b, byte c, byte d)
	{
		const u32 bytes = (decode_table[a] << 18) | (decode_table[b] << 12) | (decode_table[c] << 6) | decode_table[d];

		const byte b1 = (bytes >> 16) & 0xFF;
		const byte b2 = (bytes >> 8) & 0xFF;
		const byte b3 = bytes & 0xFF;
		return {b1, b2, b3};
	}

	export std::optional<std::vector<u8>> decode(std::string_view encoded_input)
	{
		if (encoded_input.empty())
			return std::nullopt;

		if (!is_valid_base64_str(encoded_input))
			return std::nullopt;

		const auto unpadded_input = encoded_input.substr(0, encoded_input.find_first_of('='));
		const auto blocks         = unpadded_input.size() / 4;
		const auto remainder      = unpadded_input.size() % 4;

		std::vector<u8> output;
		size_t          reserve_size = as<size_t>(std::floor(unpadded_input.size() * 3uz) / 4uz);
		output.reserve(reserve_size);

		for (u64 i = 0; i < blocks; ++i)
		{
			const auto four  = unpadded_input.substr(i * 4, 4);
			const auto bytes = decode_four(four[0], four[1], four[2], four[3]);
			std::ranges::copy(bytes, std::back_inserter(output));
		}

		if (remainder > 0)
		{
			std::array<byte, 4> buffer{};
			buffer.fill('A');

			for (size_t i = 0; i < remainder; ++i)
				buffer[i] = unpadded_input[blocks * 4 + i];

			const auto bytes = decode_four(buffer[0], buffer[1], buffer[2], buffer[3]);
			output.insert(output.end(), bytes.begin(), bytes.begin() + ((remainder * 3) / 4));
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


} // namespace deckard::utils::base64
