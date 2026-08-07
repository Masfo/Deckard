module;
#include <intrin.h>

export module deckard.utils.hash;

#ifndef _DEBUG
import deckard_build;
#endif


import deckard.types;
import deckard.as;
import deckard.helpers;
import deckard.debug;
import std;

namespace deckard::utils
{


	constexpr u32 constant_seed_1 =
	  (__TIME__[7] - '0') * 1 + (__TIME__[6] - '0') * 10 + (__TIME__[4] - '0') * 60 + (__TIME__[3] - '0') * 600 +
	  (__TIME__[1] - '0') * 3600 + (__TIME__[0] - '0') * 36000;

	template<std::unsigned_integral T>
	constexpr T distribute(T x) noexcept
	{
		if constexpr (sizeof(T) == 8)
		{
			x ^= x >> 12;
			x ^= x << 25;
			x ^= x >> 27;
			return x * 0x2545'F491'4F6C'DD1DULL;
		}
		else if constexpr (sizeof(T) == 4)
		{
			x ^= x >> 13;
			x ^= x << 17;
			x ^= x >> 5;
			return x * 0x85eb'ca6bU;
		}
		else
		{
			static_assert(sizeof(T) == 8 || sizeof(T) == 4, "distribute: add a branch if you need 16/8-bit support");
		}
	}

	export template<typename T, typename... Rest>
	constexpr u64 hash_combine(u64 seed, const T& v, Rest&&... rest)
	{

		if constexpr (std::is_integral_v<T>)
		{
			seed = std::rotl(seed, std::numeric_limits<u64>::digits / 3) ^
				   distribute(static_cast<u64>(static_cast<std::make_unsigned_t<T>>(v)));
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			using ftype = std::conditional_t<sizeof(T) == 4, u32, u64>;
			seed =
			  std::rotl(seed, std::numeric_limits<u64>::digits / 3) ^ distribute(static_cast<u64>(std::bit_cast<ftype>(v)));
		}
		else if constexpr (requires { std::hash<T>{}(v); })
		{
			seed = std::rotl(seed, std::numeric_limits<u64>::digits / 3) ^ distribute(static_cast<u64>(std::hash<T>{}(v)));
		}

		if constexpr (sizeof...(rest) > 0)
			return hash_combine(seed, std::forward<Rest>(rest)...);
		else
			return seed;
	}

	export constexpr u64 constant_seed = distribute<u64>(constant_seed_1);

	export template<typename... Types>
	constexpr u64 hash_values(const Types&... args)
	{
		u64 seed = constant_seed;
		seed     = hash_combine(seed, args...);
		return seed;
	}

	export template<typename T>
	constexpr u64 hash_values(std::span<const T> args)
	{
		return std::ranges::fold_left(args, constant_seed, [](u64 seed, const T& v) { return hash_combine(seed, v); });
	}

	export template<typename T>
	constexpr u64 hash_values(std::span<T> args)
	{
		return hash_values(std::span<const T>{args.data(), args.size()});
	}

	export template<typename T>
	constexpr u64 hash_values(const std::vector<T>& args)
	{
		return hash_values(std::span<const T>{args.data(), args.size()});
	}

	export template<typename T, size_t N>
	constexpr u64 hash_values(const std::array<T, N>& args)
	{
		return hash_values(std::span<const T>{args.data(), args.size()});
	}

	export constexpr u64 hash_values(std::string_view str) { return hash_values(to_span(str)); }

	export constexpr u64 hash_values(const std::string& str) { return hash_values(to_span(str)); }

	export constexpr u64 hash_values(const char* str) { return hash_values(to_span(str)); }

	// ########################################################################
	// Siphash


	export u64 siphash(std::span<const u8, 16> key, std::span<const u8> m)
	{
		u64    mi{0};
		size_t i{0}, blocks{0};

		u64 k0 = as<u64>(key.subspan(0, 8));
		u64 k1 = as<u64>(key.subspan(8, 8));

		u64 v0 = k0 ^ 0x736f'6d65'7073'6575ull;
		u64 v1 = k1 ^ 0x646f'7261'6e64'6f6dull;
		u64 v2 = k0 ^ 0x6c79'6765'6e65'7261ull;
		u64 v3 = k1 ^ 0x7465'6462'7974'6573ull;

		u64 last7 = (u64)(m.size() & 0xff) << 56;

		auto sipcompress = [&]
		{
			v0 += v1;
			v2 += v3;
			v1 = std::rotl(v1, 13);
			v3 = std::rotl(v3, 16);
			v1 ^= v0;
			v3 ^= v2;
			v0 = std::rotl(v0, 32);
			v2 += v1;
			v0 += v3;
			v1 = std::rotl(v1, 17);
			v3 = std::rotl(v3, 21);
			v1 ^= v2;
			v3 ^= v0;
			v2 = std::rotl(v2, 32);
		};

		for (i = 0, blocks = (m.size() & ~7); i < blocks; i += 8)
		{
			mi = as<u64>(m.subspan(i, 8));
			v3 ^= mi;
			sipcompress();
			sipcompress();
			v0 ^= mi;
		}

		switch (m.size() - blocks)
		{
			case 7: last7 |= (u64)m[i + 6] << 48; [[fallthrough]];
			case 6: last7 |= (u64)m[i + 5] << 40; [[fallthrough]];
			case 5: last7 |= (u64)m[i + 4] << 32; [[fallthrough]];
			case 4: last7 |= (u64)m[i + 3] << 24; [[fallthrough]];
			case 3: last7 |= (u64)m[i + 2] << 16; [[fallthrough]];
			case 2: last7 |= (u64)m[i + 1] << 8; [[fallthrough]];
			case 1: last7 |= (u64)m[i + 0];
			case 0:
			default:;
		};

		v3 ^= last7;
		sipcompress();
		sipcompress();
		v0 ^= last7;
		v2 ^= 0xff;
		sipcompress();
		sipcompress();
		sipcompress();
		sipcompress();

		return v0 ^ v1 ^ v2 ^ v3;
	}

	export u64 siphash(std::span<const u8> buffer)
	{
#ifdef _DEBUG

		// key from random.org
		constexpr static std::array<u8, 16> key = {
		  0x5A, 0x90, 0x6D, 0x41, 0xBC, 0xBA, 0xEC, 0xDF, 0x6E, 0x64, 0xE6, 0x5C, 0x3A, 0x71, 0xD9, 0xA1};
		return siphash(key, buffer);

#else
		constexpr static std::array<u8, 16> key = {
		  0x5A, 0x90, 0x6D, 0x41, 0xBC, 0xBA, 0xEC, 0xDF, 0x6E, 0x64, 0xE6, 0x5C, 0x3A, 0x71, 0xD9, 0xA1};
		return siphash(key, buffer);
#endif
	}

	export u64 siphash(std::string_view str) { return siphash(to_span(str)); }

	export u64 operator""_siphash(char const* s, size_t count) { return siphash({s, count}); }

	// ########################################################################
	// fnv
	constexpr u32 val_32_const   = 0x811c'9dc5;
	constexpr u32 prime_32_const = 0x100'0193;
	constexpr u64 val_64_const   = 0xcbf2'9ce4'8422'2325;
	constexpr u64 prime_64_const = 0x100'0000'01b3;

	export constexpr u32 fnv1a_32(char const* buffer, size_t count)
	{
		uint32_t hash  = val_32_const;
		uint32_t prime = prime_32_const;

		for (int i = 0; i < count; ++i)
		{
			uint8_t value = buffer[i];
			hash          = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	export constexpr u32 fnv1a_32(const std::span<const u8> buffer)
	{
		u32 hash = val_32_const;

		for (int i = 0; i < buffer.size(); ++i)
		{
			hash = hash ^ buffer[i];
			hash *= prime_32_const;
		}

		return hash;
	}

	export constexpr u64 fnv1a_64(const std::span<const u8> buffer)
	{
		u64 hash = val_64_const;

		for (int i = 0; i < buffer.size(); ++i)
		{
			hash = hash ^ buffer[i];
			hash *= prime_64_const;
		}

		return hash;
	}

	export constexpr u32 fnv1a_32(std::string_view str) { return fnv1a_32(to_span(str)); }

	export constexpr u64 fnv1a_64(std::string_view str) { return fnv1a_64(to_span(str)); }

	export constexpr u32 operator""_fnv32(char const* s, size_t count) { return fnv1a_32({s, count}); }

	export constexpr u64 operator""_fnv64(char const* s, size_t count) { return fnv1a_64({s, count}); }

	// ########################################################################
	// rapiphash

	constexpr u64 RAPID_SEED = 0xbdd8'9aa9'8270'4029ull;

	constexpr u64 rapid_secret[3] = {0x2d35'8dcc'aa6c'78a5ull, 0x8bb8'4b93'962e'acc9ull, 0x4b33'a62e'd433'd4a3ull};

	void rapid_mum(u64* A, u64* B)
	{
#if defined(_MSC_VER)
		*A = _umul128(*A, *B, B);
#else
		uint64_t ha = *A >> 32, hb = *B >> 32, la = (uint32_t)*A, lb = (uint32_t)*B;
		uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb, t = rl + (rm0 << 32), c = t < rl;
		uint64_t lo = t + (rm1 << 32);
		c += lo < t;
		uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
		*A          = lo;
		*B          = hi;
#endif
	}

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
				a               = (rapid_read32(p) << 32) | rapid_read32(plast);
				const u64 delta = ((len & 24) >> (len >> 3));
				b               = ((rapid_read32(p + delta) << 32) | rapid_read32(plast - delta));
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

	export u64 rapidhash(std::string_view str) { return rapidhash(str.data(), str.size(), RAPID_SEED); }

	export u64 rapidhash(std::span<const u8> buffer) { return rapidhash(buffer.data(), buffer.size_bytes(), RAPID_SEED); }

	// ########################################################################
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

	export constexpr u64 chibihash64(const void* keyIn, size_t len, u64 seed = CHIBI_SEED)
	{
		// https://github.com/N-R-K/ChibiHash/blob/master/chibihash64.h

		const u8* p = (const u8*)keyIn;
		ptrdiff_t l = len;

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
				h[i]       = (stripe + h[i]) * K;
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

	export u64 constexpr chibihash64(std::span<const u8> buffer, const u64 seed = CHIBI_SEED)
	{
		return chibihash64(buffer.data(), buffer.size_bytes(), seed);
	}

	export u64 constexpr chibihash64(std::string_view buffer, const u64 seed = CHIBI_SEED)
	{
		return chibihash64({as<u8*>(buffer.data()), buffer.size()}, seed);
	}

	export u64 operator""_chibihash(char const* buffer, size_t len) { return chibihash64({buffer, len}); }

	// ########################################################

	export u64 xxh64(std::span<const u8> buffer, u64 seed = 0)
	{
		static constexpr u64 prime1 = 0x9e37'79b1'85eb'ca87ULL;
		static constexpr u64 prime2 = 0xc2b2'ae3d'27d4'eb4fULL;
		static constexpr u64 prime3 = 0x1656'67b1'9e37'79f9ULL;
		static constexpr u64 prime4 = 0x85eb'ca77'c2b2'ae63ULL;
		static constexpr u64 prime5 = 0x27d4'eb2f'1656'67c5ULL;

		const u8* p   = buffer.data();
		size_t    len = buffer.size();
		const u64 n   = len;
		u64       h   = seed + prime5 + n;

		if (n >= 32)
		{
			u64 v0 = seed + prime1 + prime2;
			u64 v1 = seed + prime2;
			u64 v2 = seed + 0;
			u64 v3 = seed - prime1;

			do
			{
				v0 += load_as<u64>(p + 0) * prime2;
				v0 = std::rotl(v0, 31);
				v0 *= prime1;
				v1 += load_as<u64>(p + 8) * prime2;
				v1 = std::rotl(v1, 31);
				v1 *= prime1;
				v2 += load_as<u64>(p + 16) * prime2;
				v2 = std::rotl(v2, 31);
				v2 *= prime1;
				v3 += load_as<u64>(p + 24) * prime2;
				v3 = std::rotl(v3, 31);
				v3 *= prime1;

				p += 32;
				len -= 32;
			} while (len >= 32);

			h = std::rotl(v0, 1) + std::rotl(v1, 7) + std::rotl(v2, 12) + std::rotl(v3, 18);

			for (u64 v : {v0, v1, v2, v3})
			{
				v *= prime2;
				v = std::rotl(v, 31);
				v *= prime1;
				h ^= v;
				h = h * prime1 + prime4;
			}

			h += n;
		}

		while (len >= 8)
		{
			u64 k1 = load_as<u64>(p) * prime2;
			k1     = std::rotl(k1, 31);
			k1 *= prime1;
			h ^= k1;
			h = std::rotl(h, 27) * prime1 + prime4;
			p += 8;
			len -= 8;
		}

		if (len >= 4)
		{
			h ^= load_as<u32>(p) * prime1;
			h = std::rotl(h, 23) * prime2 + prime3;
			p += 4;
			len -= 4;
		}

		while (len > 0)
		{
			h ^= (*p) * prime5;
			h = std::rotl(h, 11) * prime1;
			++p;
			--len;
		}

		h ^= h >> 33;
		h *= prime2;
		h ^= h >> 29;
		h *= prime3;
		h ^= h >> 32;

		return h;
	}

	export u64 xxh64(std::string_view str, u64 seed = 0) { return xxh64(to_span(str), seed); }

	export u64 operator""_xxh64(char const* buffer, size_t len) { return xxh64({buffer, len}); }

	// ##########################

	export class xxhash64_hasher
	{
		static constexpr u64 prime1 = 0x9E37'79B1'85EB'CA87ULL;
		static constexpr u64 prime2 = 0xC2B2'AE3D'27D4'EB4FULL;
		static constexpr u64 prime3 = 0x1656'67B1'9E37'79F9ULL;
		static constexpr u64 prime4 = 0x85EB'CA77'C2B2'AE63ULL;
		static constexpr u64 prime5 = 0x27D4'EB2F'1656'67C5ULL;

		std::array<u64, 4> state_{prime1 + prime2, prime2, 0, static_cast<u64>(-static_cast<i64>(prime1))};
		std::array<u8, 32> buffer_{};
		size_t             buffer_size_ = 0;
		u64                total_len_   = 0;

	public:
		xxhash64_hasher() = default;

		void update(std::span<const u8> data) noexcept
		{
			auto ptr = data.data();
			auto len = data.size();
			total_len_ += len;

			// Fill buffer if we have leftover data
			if (buffer_size_ > 0)
			{
				auto to_copy = std::min(size_t{32} - buffer_size_, len);
				std::copy_n(ptr, to_copy, buffer_.data() + buffer_size_);
				buffer_size_ += to_copy;
				ptr += to_copy;
				len -= to_copy;

				if (buffer_size_ == 32)
				{
					// pull into locals, process, write back once — same
					// register-residency trick as the bulk loop below
					u64 v0 = state_[0], v1 = state_[1], v2 = state_[2], v3 = state_[3];
					process_block_into(buffer_.data(), v0, v1, v2, v3);
					state_[0] = v0, state_[1] = v1, state_[2] = v2, state_[3] = v3;
					buffer_size_ = 0;
				}
			}

			// Process full 32-byte blocks: state lives in locals for the
			// *entire* loop, only spilled back to the member array once at
			// the end. This is the part that was costing you the loads/
			// stores per block in the original.
			if (len >= 32)
			{
				u64 v0 = state_[0], v1 = state_[1], v2 = state_[2], v3 = state_[3];

				do
				{
					process_block_into(ptr, v0, v1, v2, v3);
					ptr += 32;
					len -= 32;
				} while (len >= 32);

				state_[0] = v0, state_[1] = v1, state_[2] = v2, state_[3] = v3;
			}

			// Store remaining bytes in buffer
			if (len > 0)
			{
				std::copy_n(ptr, len, buffer_.data());
				buffer_size_ = len;
			}
		}

		void update(std::string_view data) noexcept { update(std::span<const u8>{as<const u8*>(data.data()), data.size()}); }

		[[nodiscard]] u64 digest() const noexcept
		{
			u64 hash{};

			if (total_len_ >= 32)
			{
				hash =
				  std::rotl(state_[0], 1) + std::rotl(state_[1], 7) + std::rotl(state_[2], 12) + std::rotl(state_[3], 18);

				for (u64 acc : state_)
				{
					acc *= prime2;
					acc = std::rotl(acc, 31);
					acc *= prime1;
					hash ^= acc;
					hash = hash * prime1 + prime4;
				}
			}
			else
			{
				hash = state_[2] + prime5;
			}

			hash += total_len_;

			auto buffer_span = std::span<const u8>{buffer_.data(), buffer_size_};
			auto remaining   = buffer_span.size();
			auto ptr         = buffer_span.data();

			while (remaining >= 8)
			{
				u64 k1 = load_as<u64>(ptr);
				k1 *= prime2;
				k1 = std::rotl(k1, 31);
				k1 *= prime1;
				hash ^= k1;
				hash = std::rotl(hash, 27) * prime1 + prime4;
				ptr += 8;
				remaining -= 8;
			}

			if (remaining >= 4)
			{
				u64 k1 = load_as<u32>(ptr);
				hash ^= k1 * prime1;
				hash = std::rotl(hash, 23) * prime2 + prime3;
				ptr += 4;
				remaining -= 4;
			}

			for (; remaining > 0; ++ptr, --remaining)
			{
				hash ^= (*ptr) * prime5;
				hash = std::rotl(hash, 11) * prime1;
			}

			hash ^= hash >> 33;
			hash *= prime2;
			hash ^= hash >> 29;
			hash *= prime3;
			hash ^= hash >> 32;

			return hash;
		}

		void reset() noexcept
		{
			state_       = {prime1 + prime2, prime2, 0, static_cast<u64>(-static_cast<i64>(prime1))};
			buffer_size_ = 0;
			total_len_   = 0;
		}

		void clear() noexcept { reset(); }

	private:
		// Manually unrolled (no index loop) and forced inline, taking the
		// 4 lanes by reference so callers keep them in locals/registers
		// across many invocations instead of round-tripping through state_.
		static void process_block_into(const u8* block, u64& v0, u64& v1, u64& v2, u64& v3) noexcept
		{
			v0 += load_as<u64>(block + 0) * prime2;
			v0 = std::rotl(v0, 31);
			v0 *= prime1;
			v1 += load_as<u64>(block + 8) * prime2;
			v1 = std::rotl(v1, 31);
			v1 *= prime1;
			v2 += load_as<u64>(block + 16) * prime2;
			v2 = std::rotl(v2, 31);
			v2 *= prime1;
			v3 += load_as<u64>(block + 24) * prime2;
			v3 = std::rotl(v3, 31);
			v3 *= prime1;
		}
	};

	// ########################################################


	export u64 xxhash64(std::string_view str) { return xxh64(str); }

	export template<typename... Views>
	u64 xxhash64(std::string_view first, Views... rest)
	{
		xxhash64_hasher hasher;
		hasher.update(first);
		(hasher.update(rest), ...);
		return hasher.digest();
	}

	export u64 xxhash64(std::span<const u8> buffer) { return xxh64(buffer); }

	// ########################################################
	// default secret (wyhash "final4.3" reference constants)

	namespace wyhash_detail
	{
		// portable 64x64->128 multiply, returning {low, high}
		[[nodiscard]] inline std::pair<u64, u64> mul128(u64 a, u64 b) noexcept {
#if defined(_MSC_VER) && defined(_M_X64)
			u64 hi{};
			u64 lo = _umul128(a, b, &hi);
			return {lo, hi};
#else
			// manual 32-bit split multiply — portable fallback (e.g. MSVC on ARM)
			const u64 a_lo = static_cast<u32>(a), a_hi = a >> 32;
			const u64 b_lo = static_cast<u32>(b), b_hi = b >> 32;

			const u64 ll = a_lo * b_lo;
			const u64 lh = a_lo * b_hi;
			const u64 hl = a_hi * b_lo;
			const u64 hh = a_hi * b_hi;

			const u64 mid = (ll >> 32) + static_cast<u32>(lh) + static_cast<u32>(hl);
			const u64 lo  = (mid << 32) | static_cast<u32>(ll);
			const u64 hi  = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
			return {lo, hi};
#endif
		}


		// 64x64->128 multiply-and-xor-fold ("MUM")
		[[nodiscard]] inline u64 mix(u64 a, u64 b) noexcept
		{
			const auto [lo, hi] = mul128(a, b);
			return lo ^ hi;
		}

		[[nodiscard]] inline u64 read_u64(const u8* p) noexcept
		{
			u64 v{0};
			std::memcpy(&v, p, 8);
			if constexpr (std::endian::native == std::endian::big)
				v = std::byteswap(v);
			return v;
		}

		[[nodiscard]] inline u64 read_u32(const u8* p) noexcept
		{
			u32 v{0};
			std::memcpy(&v, p, 4);
			if constexpr (std::endian::native == std::endian::big)
				v = std::byteswap(v);
			return v;
		}

		// reads 3 bytes for len in [1,3]: p[0], p[len/2], p[len-1]
		[[nodiscard]] inline u64 read_short(const u8* p, size_t len) noexcept
		{
			return (static_cast<u64>(p[0]) << 16) | (static_cast<u64>(p[len >> 1]) << 8) | p[len - 1];
		}
	} // namespace detail



	constexpr std::array<u64, 4> default_secret{
	  0x2d35'8dcc'aa6c'78a5ull,
	  0x8bb8'4b93'962e'acc9ull,
	  0x4b33'a62e'd433'd4a3ull,
	  0x4d5a'2da5'1de1'aa47ull,
	};

	export [[nodiscard]] u64
	wyhash(std::span<const u8> data, u64 seed = 0, const std::array<u64, 4>& secret = default_secret) noexcept
	{
		using namespace wyhash_detail;

		const u8*    p   = data.data();
		const size_t len = data.size();

		seed ^= mix(seed ^ secret[0], secret[1]);

		u64 a{};
		u64 b{};

		if (len <= 16) [[likely]]
		{
			if (len >= 4) [[likely]]
			{
				a = (read_u32(p) << 32) | read_u32(p + ((len >> 3) << 2));
				b = (read_u32(p + len - 4) << 32) | read_u32(p + len - 4 - ((len >> 3) << 2));
			}
			else if (len > 0) [[likely]]
			{
				a = read_short(p, len);
				b = 0;
			}
			else
			{
				a = b = 0;
			}
		}
		else
		{
			size_t i = len;
			if (i >= 48) [[unlikely]]
			{
				u64 see1 = seed;
				u64 see2 = seed;
				do
				{
					seed = mix(read_u64(p) ^ secret[1], read_u64(p + 8) ^ seed);
					see1 = mix(read_u64(p + 16) ^ secret[2], read_u64(p + 24) ^ see1);
					see2 = mix(read_u64(p + 32) ^ secret[3], read_u64(p + 40) ^ see2);
					p += 48;
					i -= 48;
				} while (i >= 48);
				seed ^= see1 ^ see2;
			}
			while (i > 16) [[unlikely]]
			{
				seed = mix(read_u64(p) ^ secret[1], read_u64(p + 8) ^ seed);
				i -= 16;
				p += 16;
			}
			a = read_u64(p + i - 16);
			b = read_u64(p + i - 8);
		}

		a ^= secret[1];
		b ^= seed;

		const auto [ha, hb] = mul128(a, b);
		a                   = ha;
		b                   = hb;

		return mix(a ^ secret[0] ^ static_cast<u64>(len), b ^ secret[1]);
	}

	export template<typename R>
	requires std::convertible_to<R, std::span<const u8>>
	[[nodiscard]] inline u64 wyhash(const R& range, u64 seed = 0, const std::array<u64, 4>& secret = default_secret) noexcept
	{
		return wyhash(std::span<const u8>(range), seed, secret);
	}

	export u64 wyhash(std::string_view str, u64 seed = 0, const std::array<u64, 4>& secret = default_secret)
	{
		return wyhash(to_span(str), seed, secret);
	}

	// ########################################################


	export u64 hash(std::string_view str, u64 seed=0) { return wyhash(str, seed); }

	export u64 hash(std::span<const u8> buffer, u64 seed=0) { return wyhash(buffer, seed); }



} // namespace deckard::utils

// ########################################################
// std::hash specialization for xxhash64_hasher

export template<>
struct std::hash<deckard::utils::xxhash64_hasher>
{
	[[nodiscard]] size_t operator()(const deckard::utils::xxhash64_hasher& hasher) const noexcept { return hasher.digest(); }
};
