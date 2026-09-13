export module deckard.base_encoding;


import deckard.types;
import deckard.debug;
import deckard.as;
import deckard.assert;
import std;

// Base32
namespace deckard::utils::base32
{
	constexpr u8 INVALID_SYMBOL = 0x64;

	static constexpr std::array<u8, 32> encode_table{
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7'};

	static constexpr std::array<u8, 256> decode_table = []() static constexpr
	{
		std::array<u8, 256> table{};
		table.fill(INVALID_SYMBOL);

		for (usize i = 0; i < encode_table.size(); ++i)
			table[encode_table[i]] = as<u8>(i);

		return table;
	}();

	constexpr std::array<u8, 8> encode_five(const std::span<const u8> input)
	{

		const u64 combined = (u64(input[0]) << 32) | (u64(input[1]) << 24) | (u64(input[2]) << 16) | (u64(input[3]) << 8)
							 | u64(input[4]);

		const u8 b1 = encode_table[(combined >> 35) & 0x1F];
		const u8 b2 = encode_table[(combined >> 30) & 0x1F];
		const u8 b3 = encode_table[(combined >> 25) & 0x1F];
		const u8 b4 = encode_table[(combined >> 20) & 0x1F];
		const u8 b5 = encode_table[(combined >> 15) & 0x1F];
		const u8 b6 = encode_table[(combined >> 10) & 0x1F];
		const u8 b7 = encode_table[(combined >> 5) & 0x1F];
		const u8 b8 = encode_table[combined & 0x1F];

		return {b1, b2, b3, b4, b5, b6, b7, b8};
	}

	constexpr auto decode_five(const std::span<const u8> input) -> std::array<u8, 5>
	{
		u64 combined = 0;
		for (usize i = 0; i < 8; ++i)
			combined = (combined << 5) | decode_table[input[i]];

		return {as<u8>((combined >> 32) & 0xFF),
				as<u8>((combined >> 24) & 0xFF),
				as<u8>((combined >> 16) & 0xFF),
				as<u8>((combined >> 8) & 0xFF),
				as<u8>(combined & 0xFF)};
	}

	bool is_valid_base32_char(u8 c) { return decode_table[c] != INVALID_SYMBOL; }

	bool is_valid_base32_str(std::string_view encoded_string)
	{
		if ((encoded_string.size() % 4) == 1)
			return false;
		if (!std::all_of(
			  std::begin(encoded_string), std::end(encoded_string) - 2, [](u8 c) { return is_valid_base32_char(c); }))
			return false;

		const auto last = std::rbegin(encoded_string);
		if (!is_valid_base32_char(*std::next(last)))
			return (*std::next(last) == '=') and (*last == '=');

		return is_valid_base32_char(*last) or (*last == '=');
	}


	export enum class padding { yes, no };

	export std::string encode(std::span<const u8> input, padding add_padding = padding::yes)
	{
		if (input.empty())
			return {};

		const auto size      = input.size();
		const auto blocks    = size / 5;
		const auto remainder = size % 5;

		std::string output;
		size_t      reserve_size = 8ull * as<size_t>(std::ceil(input.size_bytes() / 5.0f));
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
		size_t          reserve_size = as<size_t>(std::floor(unpadded_input.size() * 5ull) / 8);
		output.reserve(reserve_size);


		for (size_t i = 0; i < blocks; ++i)
		{
			std::array<u8, 8> block{};
			std::copy_n(unpadded_input.begin() + (i * 8), 8, block.begin());
			const auto bytes = decode_five(block);
			output.insert(output.end(), bytes.begin(), bytes.end());
		}

		if (remainder > 0)
		{
			std::array<u8, 8> buffer{};
			buffer.fill('A');

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

// Base64
namespace deckard::utils::base64
{
	constexpr u8 INVALID_SYMBOL = 0x64;

	static constexpr std::array<u8, 64> encode_table{
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

	bool is_valid_base64_char(u8 c) { return decode_table[c] != INVALID_SYMBOL; }

	bool is_valid_base64_str(std::string_view encoded_string)
	{
		if ((encoded_string.size() % 4) == 1)
			return false;
		if (!std::all_of(
			  std::begin(encoded_string), std::end(encoded_string) - 2, [](u8 c) { return is_valid_base64_char(c); }))
			return false;

		const auto last = std::rbegin(encoded_string);
		if (!is_valid_base64_char(*std::next(last)))
			return (*std::next(last) == '=') and (*last == '=');

		return is_valid_base64_char(*last) or (*last == '=');
	}

	export enum class padding { yes, no };

	export std::string encode(std::span<const u8> input, padding add_padding = padding::yes)
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
			return {};

		if (not is_valid_base64_str(encoded_input))
			return {};

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

// Base85
namespace deckard::utils::base85
{
	constexpr std::string_view alphabet{
	  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	  "abcdefghijklmnopqrstuvwxyz"
	  "0123456789"
	  "!#$%&'()+,-;=@[]^_`{}~"};

	static_assert(alphabet.size() == 84, "Alphabet must be exactly 85 characters.");

	inline constexpr std::array<u32, 5> powers_of_85 = {52'200'625, 614'125, 7225, 85, 1};

	constexpr auto make_decode_table()
	{
		std::array<i16, 256> table{};
		table.fill(-1);
		for (usize i = 0; i < alphabet.size(); ++i)
		{
			table[static_cast<u8>(alphabet[i])] = static_cast<u16>(i);
		}
		return table;
	}

	inline constexpr auto decode_table = make_decode_table();

	export [[nodiscard]] std::string encode(std::span<const u8> data)
	{
		std::string result;
		result.reserve(((data.size() + 3) / 4) * 5);

		usize i = 0;
		while (i + 4 <= data.size())
		{
			u32 val = (static_cast<u32>(data[i]) << 24) | (static_cast<u32>(data[i + 1]) << 16)
					  | (static_cast<u32>(data[i + 2]) << 8) | static_cast<u32>(data[i + 3]);

			for (u32 p : powers_of_85)
			{
				result.push_back(alphabet[(val / p) % 85]);
			}
			i += 4;
		}

		// Handle partial trailing bytes
		if (usize rem = data.size() - i; rem > 0)
		{
			u32 val = 0;
			for (usize j = 0; j < rem; ++j)
				val |= static_cast<u32>(data[i + j]) << (24 - j * 8);

			for (usize j = 0; j < rem + 1; ++j)
				result.push_back(alphabet[(val / powers_of_85[j]) % 85]);
		}

		return result;
	}

	export std::string encode(std::string_view input) { return encode({std::bit_cast<u8*>(input.data()), input.size()}); }

	// ######################################################################


	export [[nodiscard]] std::expected<std::vector<u8>, std::string> decode(std::string_view encoded)
	{
		if (encoded.empty())
			return {};

		std::vector<u8> result;
		result.reserve((encoded.size() * 4) / 5);

		usize i = 0;
		while (i + 5 <= encoded.size())
		{
			u64 val = 0;
			for (usize j = 0; j < 5; ++j)
			{
				i16 digit = decode_table[static_cast<u8>(encoded[i + j])];
				if (digit < 0)
					return std::unexpected(std::format("Invalid character '{}' at position {}", encoded[i + j], i + j));
				val += static_cast<u64>(digit) * powers_of_85[j];
			}

			if (val > 0xFFFF'FFFFull)
				return std::unexpected(std::format("Value overflow at position {}: {:#010x} (exceeds 0xFFFFFFFF)", i, val));

			result.push_back(static_cast<u8>((val >> 24) & 0xFF));
			result.push_back(static_cast<u8>((val >> 16) & 0xFF));
			result.push_back(static_cast<u8>((val >> 8) & 0xFF));
			result.push_back(static_cast<u8>(val & 0xFF));
			i += 5;
		}

		// remaining characters
		if (usize rem = encoded.size() - i; rem > 0)
		{
			if (rem == 1)
				return std::unexpected(std::format("Invalid length at position {}", i));

			u64 val = 0;
			for (usize j = 0; j < 5; ++j)
			{
				i16 digit = (j < rem) ? decode_table[static_cast<u8>(encoded[i + j])] : 84;
				if (digit < 0)
					return std::unexpected(std::format("Invalid character '{}' at position {}", encoded[i + j], i + j));
				val += static_cast<u64>(digit) * powers_of_85[j];
			}

			if (val > 0xFFFF'FFFFull)
				return std::unexpected(std::format("Value overflow at position {}: {:#010x} (exceeds 0xFFFFFFFF)", i, val));

			for (usize j = 0; j < rem - 1; ++j)
				result.push_back(static_cast<u8>((val >> (24 - j * 8)) & 0xFF));
		}

		return result;
	}

	export [[nodiscard]] std::expected<std::string, std::string> decode_as_string(std::string_view encoded) noexcept
	{
		auto result = decode(encoded);
		if (not result)
			return std::unexpected(result.error());

		std::string str;
		str.reserve(result->size());
		std::ranges::copy(*result, std::back_inserter(str));

		return str;
	}


} // namespace deckard::utils::base85
