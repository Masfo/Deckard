export module deckard.base32;

import deckard.types;
import deckard.as;
import deckard.assert;
import std;

namespace deckard::utils::base32
{
	constexpr u8 INVALID_SYMBOL = 0x64;

	static constexpr std::array<byte, 32> encode_table{
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7'};

	static constexpr std::array<u8, 256> decode_table = []() constexpr
	{
		std::array<u8, 256> table{};
		table.fill(INVALID_SYMBOL);

		for (size_t i = 0; i < encode_table.size(); ++i)
			table[encode_table[i]] = as<u8>(i);

		return table;
	}();


	constexpr std::array<byte, 8> encode_five(const std::span<const u8> input)
	{

		const u64 combined = (u64(input[0]) << 32) | (u64(input[1]) << 24) | (u64(input[2]) << 16) | (u64(input[3]) << 8) | u64(input[4]);

		const byte b1 = encode_table[(combined >> 35) & 0x1F];
		const byte b2 = encode_table[(combined >> 30) & 0x1F];
		const byte b3 = encode_table[(combined >> 25) & 0x1F];
		const byte b4 = encode_table[(combined >> 20) & 0x1F];
		const byte b5 = encode_table[(combined >> 15) & 0x1F];
		const byte b6 = encode_table[(combined >> 10) & 0x1F];
		const byte b7 = encode_table[(combined >> 5) & 0x1F];
		const byte b8 = encode_table[combined & 0x1F];

		return {b1, b2, b3, b4, b5, b6, b7, b8};
	}

	constexpr auto decode_five(const std::span<const u8> input) -> std::array<u8, 5>
	{
		u64 combined = 0;
		for (size_t i = 0; i < 8; ++i)
			combined = (combined << 5) | decode_table[input[i]];

		return {as<u8>((combined >> 32) & 0xFF),
				as<u8>((combined >> 24) & 0xFF),
				as<u8>((combined >> 16) & 0xFF),
				as<u8>((combined >> 8) & 0xFF),
				as<u8>(combined & 0xFF)};
	}

	bool is_valid_base32_char(byte c) { return decode_table[c] != INVALID_SYMBOL; }

	bool is_valid_base32_str(std::string_view encoded_string)
	{
		if ((encoded_string.size() % 4) == 1)
			return false;
		if (!std::all_of(std::begin(encoded_string), std::end(encoded_string) - 2, [](byte c) { return is_valid_base32_char(c); }))
			return false;

		const auto last = std::rbegin(encoded_string);
		if (!is_valid_base32_char(*std::next(last)))
			return (*std::next(last) == '=') and (*last == '=');

		return is_valid_base32_char(*last) or (*last == '=');
	}


	export enum class padding { yes, no };

	export std::string encode(const std::span<const u8> input, padding add_padding = padding::yes)
	{
		if (input.empty())
			return {};

		const auto size      = input.size();
		const auto blocks    = size / 5;
		const auto remainder = size % 5;

		std::string output;
		size_t      reserve_size = 8ULL * std::ceil(input.size_bytes() / 5.0f);
		output.reserve(reserve_size);

		for (size_t i = 0; i < blocks; ++i)
		{
			const auto bytes = encode_five(input.subspan(i * 5, 5));
			output.append(bytes.begin(), bytes.end());
		}

		if (remainder > 0)
		{
			std::array<u8, 5> buffer{0};

			for (size_t i = 0; i < remainder; ++i)
				buffer[i] = input[blocks * 5 + i];


			const size_t encoded_len = (remainder * 8 + 4) / 5;

			const auto bytes = encode_five(buffer);
			output.append(bytes.begin(), bytes.begin() + encoded_len);

			if (add_padding == padding::yes)
			{
				output.append(8 - encoded_len, '=');
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

		if (!is_valid_base32_str(encoded_input))
			return std::nullopt;

		const auto unpadded_input = encoded_input.substr(0, encoded_input.find_first_of('='));
		const auto size           = unpadded_input.size();
		const auto blocks         = size / 8;
		const auto remainder      = size % 8;

		std::vector<u8> output;
		size_t          reserve_size = std::floor(unpadded_input.size() * 5ULL) / 8;
		output.reserve(reserve_size);


		for (size_t i = 0; i < blocks; ++i)
		{
			std::array<byte, 8> block{};
			std::copy_n(unpadded_input.begin() + (i * 8), 8, block.begin());
			const auto bytes = decode_five(block);
			output.insert(output.end(), bytes.begin(), bytes.end());
		}

		if (remainder > 0)
		{
			std::array<byte, 8> buffer{'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A'};
			for (size_t i = 0; i < remainder; ++i)
				buffer[i] = unpadded_input[blocks * 8 + i];

			const auto bytes = decode_five(buffer);
			output.insert(output.end(), bytes.begin(), bytes.begin() + ((remainder * 5) / 8));
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
