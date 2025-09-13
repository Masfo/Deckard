module;
#include <intrin.h>

export module deckard.utils.hash;

#ifndef _DEBUG
import deckard_build;
#endif


import deckard.types;
import deckard.as;
import deckard.helpers;
import std;

namespace deckard::utils
{
	constexpr u32 constant_seed_1 =
	  (__TIME__[7] - '0') * 1 + (__TIME__[6] - '0') * 10 + (__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600 +
	  (__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000;

	template<typename T>
	constexpr T simple_xorshift(const T& n, int i)
	{
		return n ^ (n >> i);
	}

	constexpr u32 distribute(const u32& n)
	{
		constexpr u32 p = 0x5555'5555ul;
		constexpr u32 c = 0xCC0F'8E27ul;
		return c * simple_xorshift(p * simple_xorshift(n, 16), 16);
	}

	constexpr u64 distribute(const u64& n)
	{
		constexpr u64 p = 0x5555'5555'5555'5555ull;
		constexpr u64 c = 0xF04E'EA49'71D6'05C7ull;

		return c * simple_xorshift(p * simple_xorshift(n, 32), 32);
	}

	export template<typename T, typename... Rest>
	constexpr void hash_combine(std::size_t& seed, const T& v, Rest... rest)
	{
		seed = std::rotl(seed, std::numeric_limits<size_t>::digits / 3) ^ distribute(std::hash<T>{}(v));
		(hash_combine(seed, rest), ...);
	}

	export constexpr u32 constant_seed = distribute(constant_seed_1);

	export template<typename... Types>
	constexpr std::size_t hash_values(const Types&... args)
	{
		std::size_t seed = constant_seed;
		hash_combine(seed, args...);
		return seed;
	}

	export template<typename T>
	constexpr size_t hash_values(const std::span<T>& args)
	{
		std::size_t seed = constant_seed;
		for (const auto& arg : args)
		{
			hash_combine(seed, arg);
		}
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
			case 7: last7 |= (u64)m[i + 6] << 48;
			case 6: last7 |= (u64)m[i + 5] << 40;
			case 5: last7 |= (u64)m[i + 4] << 32;
			case 4: last7 |= (u64)m[i + 3] << 24;
			case 3: last7 |= (u64)m[i + 2] << 16;
			case 2: last7 |= (u64)m[i + 1] << 8;
			case 1: last7 |= (u64)m[i + 0];
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
#ifdef _DEBUG
		// key from random.org
		const unsigned char key[16] = {0x5A, 0x90, 0x6D, 0x41, 0xBC, 0xBA, 0xEC, 0xDF, 0x6E, 0x64, 0xE6, 0x5C, 0x3A, 0x71, 0xD9, 0xA1};
		return siphash(key, {as<u8*>(str.data()), str.length()});

#else
		return siphash(deckard_build::build::rng_buffer, {as<u8*>(str.data()), str.length()});
#endif
	}

	export u64 operator""_siphash(char const* s, size_t count) { return siphash({s, count}); }

	// fnv
	constexpr u32 val_32_const   = 0x811c'9dc5;
	constexpr u32 prime_32_const = 0x100'0193;
	constexpr u64 val_64_const   = 0xcbf2'9ce4'8422'2325;
	constexpr u64 prime_64_const = 0x100'0000'01b3;

	export constexpr u32 fnv1a_32(char const* s, size_t count)
	{
		return count ? (fnv1a_32(s, count - 1) ^ s[count - 1]) * prime_32_const : val_32_const;
	}

	export constexpr u64 fnv1a_64(char const* s, size_t count)
	{
		return count ? (fnv1a_64(s, count - 1) ^ s[count - 1]) * prime_64_const : val_64_const;
	}

	export constexpr u32 fnv1a_32(std::string_view str) { return fnv1a_32(str.data(), str.length()); }

	export constexpr u64 fnv1a_64(std::string_view str) { return fnv1a_64(str.data(), str.length()); }


	//
	constexpr u64 RAPID_SEED = 0xbdd8'9aa9'8270'4029ull;

	constexpr u64 rapid_secret[3] = {0x2d35'8dcc'aa6c'78a5ull, 0x8bb8'4b93'962e'acc9ull, 0x4b33'a62e'd433'd4a3ull};

	void rapid_mum(u64* A, u64* B) { *A = _umul128(*A, *B, B); }

	u64 rapid_read64(const u8* p)
	{
		u64 v;
		std::memcpy(&v, p, sizeof(u64));
		return std::byteswap(v);
	}

	u64 rapid_read32(const u8* p)
	{
		u32 v;
		std::memcpy(&v, p, sizeof(u32));
		return std::byteswap(v);
	}

	u64 rapid_mix(u64 A, u64 B)
	{
		rapid_mum(&A, &B);
		return A ^ B;
	}

	u64 rapid_readsmall(const u8* p, size_t k) { return (((u64)p[0]) << 56) | (((u64)p[k >> 1]) << 32) | p[k - 1]; }

	u64 rapidhash_internal(const void* key, size_t len, u64 seed, const u64* secret)
	{
		const u8* p = (const u8*)key;
		seed ^= rapid_mix(seed ^ secret[0], secret[1]) ^ len;
		u64 a, b;
		if (len <= 16)
		{
			if (len >= 4)
			{
				const u8* plast = p + len - 4;
				a                    = (rapid_read32(p) << 32) | rapid_read32(plast);
				const u64 delta = ((len & 24) >> (len >> 3));
				b                    = ((rapid_read32(p + delta) << 32) | rapid_read32(plast - delta));
			}
			else if (len > 0)
			{
				a = rapid_readsmall(p, len);
				b = 0;
			}
			else
				a = b = 0;
		}
		else
		{
			size_t i = len;
			if (i > 48)
			{
				u64 see1 = seed, see2 = seed;
				do
				{
					seed = rapid_mix(rapid_read64(p) ^ secret[0], rapid_read64(p + 8) ^ seed);
					see1 = rapid_mix(rapid_read64(p + 16) ^ secret[1], rapid_read64(p + 24) ^ see1);
					see2 = rapid_mix(rapid_read64(p + 32) ^ secret[2], rapid_read64(p + 40) ^ see2);
					p += 48;
					i -= 48;
				} while (i >= 48);
				seed ^= see1 ^ see2;
			}
			if (i > 16)
			{
				seed = rapid_mix(rapid_read64(p) ^ secret[2], rapid_read64(p + 8) ^ seed ^ secret[1]);
				if (i > 32)
					seed = rapid_mix(rapid_read64(p + 16) ^ secret[2], rapid_read64(p + 24) ^ seed);
			}
			a = rapid_read64(p + i - 16);
			b = rapid_read64(p + i - 8);
		}
		a ^= secret[1];
		b ^= seed;
		rapid_mum(&a, &b);
		return rapid_mix(a ^ secret[0] ^ len, b ^ secret[1]);
	}

	export u64 rapidhash(const void* key, size_t len, u64 seed) { return rapidhash_internal(key, len, seed, rapid_secret); }

	export u64 rapidhash(const void* key, size_t len) { return rapidhash(key, len, RAPID_SEED); }

	export u64 rapidhash(std::span<u8> buffer) { return rapidhash(buffer.data(), buffer.size_bytes(), RAPID_SEED); }


	// Chibihash - https://nrk.neocities.org/articles/chibihash
	constexpr u64 CHIBI_SEED = 0x1918'05f9'ed90'9da0;

	constexpr u64 chibihash64__load32le(const u8* p)
	{
		return (u64)p[0] << 0 | (u64)p[1] << 8 | (u64)p[2] << 16 | (u64)p[3] << 24;

	}

	constexpr u64 chibihash64__load64le(const u8* p)
	{
		return chibihash64__load32le(p) | (chibihash64__load32le(p + 4) << 32);

	}

	constexpr u64 chibihash64__rotl(u64 x, int n) { return (x << n) | (x >> (-n & 63)); }

	export constexpr u64 chibihash64(const void* keyIn, size_t len, u64 seed)
	{
		// https://github.com/N-R-K/ChibiHash/blob/master/chibihash64.h

		const u8* p = (const u8*)keyIn;
		ptrdiff_t      l = len;

		const u64 K     = 0x2B7E'1516'28AE'D2A7ULL; // digits of e
		u64       seed2 = chibihash64__rotl(seed - K, 15) + chibihash64__rotl(seed - K, 47);
		u64       h[4]  = {seed, seed + K, seed2, seed2 + (K * K ^ K)};

		// depending on your system unrolling might (or might not) make things
		// a tad bit faster on large strings. on my system, it actually makes
		// things slower.
		// generally speaking, the cost of bigger code size is usually not
		// worth the trade-off since larger code-size will hinder inlinability
		// but depending on your needs, you may want to uncomment the pragma
		// below to unroll the loop.
		// #pragma GCC unroll 2
		for (; l >= 32; l -= 32)
		{
			for (int i = 0; i < 4; ++i, p += 8)
			{
				u64 stripe = chibihash64__load64le(p);
				h[i]            = (stripe + h[i]) * K;
				h[(i + 1) & 3] += chibihash64__rotl(stripe, 27);
			}
		}

		for (; l >= 8; l -= 8, p += 8)
		{
			h[0] ^= chibihash64__load32le(p + 0);
			h[0] *= K;
			h[1] ^= chibihash64__load32le(p + 4);
			h[1] *= K;
		}

		if (l >= 4)
		{
			h[2] ^= chibihash64__load32le(p);
			h[3] ^= chibihash64__load32le(p + l - 4);
		}
		else if (l > 0)
		{
			h[2] ^= p[0];
			h[3] ^= p[l / 2] | ((u64)p[l - 1] << 8);
		}

		h[0] += chibihash64__rotl(h[2] * K, 31) ^ (h[2] >> 31);
		h[1] += chibihash64__rotl(h[3] * K, 31) ^ (h[3] >> 31);
		h[0] *= K;
		h[0] ^= h[0] >> 31;
		h[1] += h[0];

		u64 x = (u64)len * K;
		x ^= chibihash64__rotl(x, 29);
		x += seed;
		x ^= h[1];

		x ^= chibihash64__rotl(x, 15) ^ chibihash64__rotl(x, 42);
		x *= K;
		x ^= chibihash64__rotl(x, 13) ^ chibihash64__rotl(x, 31);

		return x;
	}

	export u64 chibihash64(const void* keyIn, size_t len) { return chibihash64(keyIn, len, CHIBI_SEED); }

	export u64 chibihash64(std::span<u8> buffer) { return chibihash64(buffer.data(), buffer.size_bytes(), CHIBI_SEED); }

	export u64 chibihash64(std::string_view buffer) { return chibihash64({as<u8*>(buffer.data()), buffer.size()}); }



} // namespace deckard::utils
