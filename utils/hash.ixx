export module deckard.utils.hash;


import deckard.types;
import std;

namespace deckard::utils
{
	constexpr u32 constant_seed_1 =
	  (__TIME__[7] - '0') * 1 + (__TIME__[6] - '0') * 10 + (__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600 +
	  (__TIME__[1] - '0') * 3'600 + (__TIME__[0] - '0') * 36'000;

	template<typename T>
	constexpr T xorshift(const T& n, int i)
	{
		return n ^ (n >> i);
	}

	constexpr u32 distribute(const u32& n)
	{
		constexpr u32 p = 0x5555'5555ul;
		constexpr u32 c = 0xCC0F'8E27ul;
		return c * xorshift(p * xorshift(n, 16), 16);
	}

	constexpr u64 distribute(const u64& n)
	{
		constexpr u64 p = 0x5555'5555'5555'5555ull;
		constexpr u64 c = 0xF04E'EA49'71D6'05C7ull;

		return c * xorshift(p * xorshift(n, 32), 32);
	}

	export template<typename T, typename... Rest>
	constexpr void hash_combine(std::size_t& seed, const T& v, Rest... rest)
	{
		seed = std::rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(std::hash<T>{}(v));
		(hash_combine(seed, rest), ...);
	}

	constexpr u32 constant_seed = distribute(constant_seed_1);

	export template<typename... Types>
	constexpr std::size_t hash_values(const Types&... args)
	{
		std::size_t seed = constant_seed;
		hash_combine(seed, args...);
		return seed;
	}
} // namespace deckard::utils
