export module deckard.base64;

import deckard.types;
import deckard.as;
import std;

namespace deckard::utils::base64
{
	constexpr u8 INVALID_SYMBOL= 0x64;

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

	constexpr auto encode_three(const std::span<const u8> input) -> std::array<byte, 4>
	{
		u32 combined = 0;
		for (size_t i = 0; i < 3; ++i)
			combined = (combined << 8) | input[i];

		const byte c1 = encode_table[(combined >> 18) & 0x3F];
		const byte c2 = encode_table[(combined >> 12) & 0x3F];
		const byte c3 = encode_table[(combined >> 6) & 0x3F];
		const byte c4 = encode_table[combined & 0x3F];
		return {c1, c2, c3, c4};
	}

	constexpr std::array<byte, 4> encode_three(u8 a, u8 b, u8 c)
	{
		const u32 combined = (a << 16) | (b << 8) | c;
		
		const byte c1 = encode_table[(combined >> 18) & 0x3F];
		const byte c2 = encode_table[(combined >> 12) & 0x3F];
		const byte c3 = encode_table[(combined >> 6) & 0x3F];
		const byte c4 = encode_table[combined & 0x3F];
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

		const auto size   = input.size();
		const auto blocks = size / 3ULL;

		std::string output;

		size_t reserve_size = 4uz * as<size_t>(std::ceil(input.size_bytes() / 3.0f));
		output.reserve(reserve_size);

		for (u64 i = 0; i < blocks; ++i)
		{
			const auto bytes = encode_three(input.subspan(i * 3, 3));
			output.append(bytes.begin(), bytes.end());
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
		const auto remainder      = unpadded_input.size() % 4;

		std::vector<u8> output;
		size_t          reserve_size =  as<size_t>(std::floor(unpadded_input.size() * 3uz) / 4uz);
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
