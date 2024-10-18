module;
#include <random> // random doesn't work without header (6.10.2024)
export module deckard.random;


#ifndef _DEBUG
import deckard_build;
#endif

import std;
import deckard.types;
import deckard.debug;
import deckard.as;

namespace deckard::random
{
	// Global
	export std::random_device rd;
	export std::mt19937       mersenne_twister;

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

	export void initialize()
	{
		std::random_device::result_type random_data[(64 - 1) / sizeof(rd()) + 1];
		std::generate(std::begin(random_data), std::end(random_data), std::ref(rd));
		std::seed_seq seeds(std::begin(random_data), std::end(random_data));

		mersenne_twister.seed(seeds);
		mersenne_twister.discard(700000);
	}


} // namespace deckard::random
