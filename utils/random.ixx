﻿export module deckard.random;


#ifndef _DEBUG
import deckard_build;
#endif

import std;
import deckard.types;
import deckard.debug;
import deckard.as;

namespace deckard::random
{


	export class splitmix64
	{
	private:
#ifndef _DEBUG
		u64 state{deckard_build::build::random_seed};
#else
		u64 state{0xdead'beef'1234'5678ULL};
#endif

		void seed_from(u64 seed)
		{
			state = seed;

			if (seed == 0)
			{
				state = std::chrono::high_resolution_clock::now().time_since_epoch().count();
				state ^= std::hash<std::thread::id>{}(std::this_thread::get_id());
			}

			for (int i = 0; i < 503; ++i)
				next();
		}

	public:
		splitmix64() { seed_from(0); }

		splitmix64(u64 seed) { seed_from(seed); }

		u64 next()
		{
			u64 z = (state += 0x9e37'79b9'7f4a'7c15);
			z     = (z ^ (z >> 30)) * 0xbf58'476d'1ce4'e5b9;
			z     = (z ^ (z >> 27)) * 0x94d0'49bb'1331'11eb;
			return z ^ (z >> 31);
		}
	};

	export class xoroshiro256
	{
		// https://vigna.di.unimi.it/xorshift/xoshiro256starstar.c

	private:
		using statetype = std::array<u64, 4>;
		statetype state;

		void from_seed(u64 seed = 0)
		{
			splitmix64 sm64(seed);
			std::generate(state.begin(), state.end(), [&sm64] { return sm64.next(); });
		}

	public:
		xoroshiro256() { from_seed(0); }

		xoroshiro256(u64 seed) { from_seed(seed); }

		explicit xoroshiro256(const statetype& seed)
			: state(seed)
		{
		}

		u64 operator()() { return next(); }

		operator u64() { return next(); }

		u64 next()
		{
			const uint64_t result = std::rotl(state[1] * 5, 7) * 9;

			const uint64_t t = state[1] << 17;

			state[2] ^= state[0];
			state[3] ^= state[1];
			state[1] ^= state[2];
			state[0] ^= state[3];
			state[2] ^= t;
			state[3] = std::rotl(state[3], 45);

			return result;
		}

		void jump()
		{
			static const statetype JUMP = {0x8a5c'd789'635d'2dff, 0x121f'd215'5c47'2f96};
			uint64_t               s0   = 0;
			uint64_t               s1   = 0;

			for (size_t i = 0; i < JUMP.size(); i++)
			{
				for (auto b = 0; b < 64; b++)
				{
					if (JUMP[i] & 1ULL << b)
					{
						s0 ^= state[0];
						s1 ^= state[1];
					}
					next();
				}
			}

			state[0] = s0;
			state[1] = s1;
		}
	};

	export class dualmix128 // https://github.com/the-othernet/DualMix128
	{
	private:
		// Golden ratio fractional part * 2^64
		const u64 GR = 0x9e37'79b9'7f4a'7c15ULL;

	public:
		dualmix128()
		{
			splitmix64 sm64;
			state0 = sm64.next();
			state1 = sm64.next();
		}

		// Initialized to non-zero with SplitMix64 (or equivalent)
		u64 state0, state1;

		// --- DualMix128 ---
		u64 next()
		{
			uint64_t mix = state0 + state1;
			state0       = mix + std::rotl(state0, 16);
			state1       = mix + std::rotl(state1, 2);

			return GR * mix;
		}

		u64 operator()() { return next(); }

		operator u64() { return next(); }
	};

	std::random_device rd;
	std::mt19937       mersenne_twister;

	constexpr std::string_view alphanum_special{
	  R"(abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !@#$%^&*()_+-={}[]|\:;"'<>,.?/~)"};
	constexpr std::string_view alphabet{alphanum_special.substr(0, 52)};
	constexpr std::string_view alphanumeric{alphanum_special.substr(0, 62)};
	constexpr std::string_view digits { alphanum_special.substr(52, 10) };

	export template<integral_or_bool T = i32>
	T rnd(T minimum = limits::min<T>, T maximum = limits::max<T>)
	{

		if constexpr (std::is_same_v<T, unsigned char> or std::is_same_v<T, char>)
		{
			std::uniform_int_distribution<i16> cdist(limits::min<T>, limits::max<T>);
			return static_cast<T>(cdist(mersenne_twister));
		}
		else if constexpr (std::is_same_v<T, bool>)
		{
			std::uniform_int_distribution<i16> bdist(0, 1);
			return bdist(mersenne_twister) ? true : false;
		}
		else if constexpr (std::is_same_v<T, i8> or std::is_same_v<T, u8>)
		{
			std::uniform_int_distribution<i16> i16_dist(minimum, maximum);
			return static_cast<T>(i16_dist(mersenne_twister));
		}
		else
		{
			std::uniform_int_distribution<T> dist(minimum, maximum);
			return static_cast<T>(dist(mersenne_twister));
		}


		static_assert(true, "Type not handled");
	}

	export template<std::floating_point T = f32>
	T rnd(T minimum = T{0}, T maximum = T{1})
	{
		std::uniform_real_distribution<T> dist(minimum, maximum);

		return dist(mersenne_twister);
	}

	export bool randbool() { return rnd<bool>(); }

	// signed
	export i8 randi8(i8 min = limits::min<i8>, i8 max = limits::max<i8>) { return as<i8>(rnd<i16>(min, max)); }

	export i16 randi16(i16 min = limits::min<i16>, i16 max = limits::max<i16>) { return rnd<i16>(min, max); }

	export i32 randi32(i32 min = limits::min<i32>, i32 max = limits::max<i32>) { return rnd<i32>(min, max); }

	export i64 randi64(i64 min = limits::min<i64>, i64 max = limits::max<i64>) { return rnd<i64>(min, max); }

	// unsigned
	export u8 randu8(u8 min = limits::min<u8>, u8 max = limits::max<u8>) { return as<u8>(rnd<u16>(min, max)); }

	export u16 randu16(u16 min = limits::min<u16>, u16 max = limits::max<u16>) { return rnd<u16>(min, max); }

	export u32 randu32(u32 min = limits::min<u32>, u32 max = limits::max<u32>) { return rnd<u32>(min, max); }

	export u64 randu64(u64 min = limits::min<u64>, u64 max = limits::max<u64>) { return rnd<u64>(min, max); }

	// the default
	export auto rand() { return randu32(); }

	// float
	export f32 float01() { return rnd<f32>(0.0f, 1.0f); }

	export f32 float11() { return rnd<f32>(-1.0f, 1.0f); }

	export void random_bytes(std::span<u8> buffer)
	{
		std::uniform_int_distribution<i16> dist(0, 255);

		std::ranges::generate(buffer, [&] { return static_cast<u8>(dist(rd) & 0xFF); });
	}

	std::string generate_with_dictionary(u32 len, const std::string_view dictionary)
	{
		std::string ret;
		ret.resize(len);
		std::ranges::generate(ret, [&dictionary] { return dictionary[rnd<u64>(0u, dictionary.length() - 1)]; });

		return ret;
	}

	export std::string alpha(u32 len = 12) { return generate_with_dictionary(len, alphabet); }

	export std::string alphanum(u32 len = 12) { return generate_with_dictionary(len, alphanumeric); }

	export std::string password(u32 len = 12) { return generate_with_dictionary(len, alphanum_special); }

	export std::string digit(u32 len = 12) { return generate_with_dictionary(len, digits); }

	export void initialize()
	{
		std::random_device::result_type random_data[(64 - 1) / sizeof(rd()) + 1];
		std::generate(std::begin(random_data), std::end(random_data), std::ref(rd));
		std::seed_seq seeds(std::begin(random_data), std::end(random_data));

		mersenne_twister.seed(seeds);
		mersenne_twister.discard(700'000);
	}


} // namespace deckard::random
