export module deckard.helpers;

import std;
import deckard.types;
import deckard.assert;

export namespace deckard
{
	// upto 0..n-1, P3060
	inline constexpr auto upto = []<std::integral I>(I n) { return std::views::iota(I{}, n); };

	// loop (n, n+1, n+..)
	inline constexpr auto loop = []<std::integral I>(I start = 0) { return std::views::iota(start); };

	inline void to_hex(const std::span<u8> &input, std::span<u8> &output, char delimiter = ' ')
	{
		//
		constexpr char dict[32 + 1]{"0123456789abcdef0123456789ABCDEF"};

		// 61 61 61 61
		assert::if_true(output.size() >= (input.size() * 2 + input.size() - 1), "Not enough space in output buffer");
	}

} // namespace deckard
