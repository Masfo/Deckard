export module deckard.utils.hash;


import deckard.types;
import std;

namespace deckard::utils
{

	template<typename T>
	T xorshift(const T& n, int i)
	{
		return n ^ (n >> i);
	}

	u32 distribute(const u32& n)
	{
		u32 p = 0x5555'5555ul;
		u32 c = 3'423'571'495ul;
		return c * xorshift(p * xorshift(n, 16), 16);
	}

	u64 distribute(const u64& n)
	{
		u64 p = 0x5555'5555'5555'5555ull;
		u64 c = 17'316'035'218'449'499'591ull;
		return c * xorshift(p * xorshift(n, 32), 32);
	}

	export template<typename T, typename... Rest>
	void hash_combine(std::size_t& seed, const T& v, Rest... rest)
	{
		seed = std::rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(std::hash<T>{}(v));
		(hash_combine(seed, rest), ...);
	}

	export template<typename... Types>
	std::size_t hash_values(const Types&... args)
	{
		std::size_t seed = 0;
		hash_combine(seed, args...);
		return seed;
	}
} // namespace deckard::utils
