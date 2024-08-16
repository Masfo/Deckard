module;
#include <emmintrin.h>
#include <xmmintrin.h>

export module deckard.utils.hash;


import deckard.types;
import deckard.as;
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

	static u64 U8TO64_LE(const unsigned char* p) { return *(const u64*)p; }

	export u64 siphash(const unsigned char key[16], std::span<u8> m)
	{
		u64    mi{0};
		size_t i{0}, blocks{0};

		u64 k0 = U8TO64_LE(key + 0);
		u64 k1 = U8TO64_LE(key + 8);
		u64 v0 = k0 ^ 0x736f'6d65'7073'6575ull;
		u64 v1 = k1 ^ 0x646f'7261'6e64'6f6dull;
		u64 v2 = k0 ^ 0x6c79'6765'6e65'7261ull;
		u64 v3 = k1 ^ 0x7465'6462'7974'6573ull;

		u64 last7 = (u64)(m.size() & 0xff) << 56;

#define sipcompress()                                                                                                                      \
	v0 += v1;                                                                                                                              \
	v2 += v3;                                                                                                                              \
	v1 = std::rotl(v1, 13);                                                                                                                \
	v3 = std::rotl(v3, 16);                                                                                                                \
	v1 ^= v0;                                                                                                                              \
	v3 ^= v2;                                                                                                                              \
	v0 = std::rotl(v0, 32);                                                                                                                \
	v2 += v1;                                                                                                                              \
	v0 += v3;                                                                                                                              \
	v1 = std::rotl(v1, 17);                                                                                                                \
	v3 = std::rotl(v3, 21);                                                                                                                \
	v1 ^= v2;                                                                                                                              \
	v3 ^= v0;                                                                                                                              \
	v2 = std::rotl(v2, 32);

		for (i = 0, blocks = (m.size() & ~7); i < blocks; i += 8)
		{
			mi = U8TO64_LE(m.data() + i);
			v3 ^= mi;
			sipcompress() sipcompress() v0 ^= mi;
		}

		switch (m.size() - blocks)
		{
			case 7: last7 |= (uint64_t)m[i + 6] << 48;
			case 6: last7 |= (uint64_t)m[i + 5] << 40;
			case 5: last7 |= (uint64_t)m[i + 4] << 32;
			case 4: last7 |= (uint64_t)m[i + 3] << 24;
			case 3: last7 |= (uint64_t)m[i + 2] << 16;
			case 2: last7 |= (uint64_t)m[i + 1] << 8;
			case 1: last7 |= (uint64_t)m[i + 0];
			case 0:
			default:;
		};
		v3 ^= last7;
		sipcompress() sipcompress() v0 ^= last7;
		v2 ^= 0xff;
		sipcompress() sipcompress() sipcompress() sipcompress() return static_cast<size_t>(v0 ^ v1 ^ v2 ^ v3);
#undef sipcompress
	}

	export u64 siphash(std::string_view str)
	{
		// key from random.org
		const unsigned char key[16] = {0x5A, 0x90, 0x6D, 0x41, 0xBC, 0xBA, 0xEC, 0xDF, 0x6E, 0x64, 0xE6, 0x5C, 0x3A, 0x71, 0xD9, 0xA1};

		return siphash(key, {as<u8*>(str.data()), str.length()});
	}

	export u64 operator""_siphash(char const* s, size_t count) { return siphash({s, count}); }

	// fnv
	constexpr u32 val_32_const   = 0x811c'9dc5;
	constexpr u32 prime_32_const = 0x100'0193;
	constexpr u64 val_64_const   = 0xcbf2'9ce4'8422'2325;
	constexpr u64 prime_64_const = 0x100'0000'01b3;

	export constexpr u32 fnv1a_32(char const* s, size_t count) noexcept
	{
		return count ? (fnv1a_32(s, count - 1) ^ s[count - 1]) * prime_32_const : val_32_const;
	}

	export constexpr u64 fnv1a_64(char const* s, size_t count) noexcept
	{
		return count ? (fnv1a_64(s, count - 1) ^ s[count - 1]) * prime_64_const : val_64_const;
	}

	export constexpr u32 fnv1a_32(std::string_view str) noexcept { return fnv1a_32(str.data(), str.length()); }

	export constexpr u64 fnv1a_64(std::string_view str) noexcept { return fnv1a_64(str.data(), str.length()); }

	export constexpr u32 operator"" _hash32(char const* s, size_t count) { return fnv1a_32(s, count); }

	export constexpr u64 operator"" _hash64(char const* s, size_t count) { return fnv1a_64(s, count); }

	static_assert("hello world"_hash32 == 0xd58b'3fa7);
	static_assert("hello world"_hash64 == 0x779a'65e7'023c'd2e7);

} // namespace deckard::utils
