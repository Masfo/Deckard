module;
#include <random> // random doesn't work without header (6.10.2024)
export module deckard.random;


#ifndef _DEBUG
import deckard_build;
#endif

import std;
import deckard.types;
import deckard.debug;

namespace deckard::random
{
	// Global
	export std::random_device rd;
	export std::mt19937       mersenne_twister;

	export template<integral_or_bool T = i32>
	T rnd(T minimum = min_value<T>, T maximum = max_value<T>)
	{

		if constexpr (std::is_same_v<T, unsigned char> or std::is_same_v<T, char>)
		{
			std::uniform_int_distribution<i16> cdist(min_value<T>, max_value<T>);
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
	export i8 randi8() { return rnd<i8>(); }

	export i16 randi16() { return rnd<i16>(); }

	export i32 randi32() { return rnd<i32>(); }

	export i64 randi64() { return rnd<i64>(); }

	// unsigned
	export u8 randu8() { return rnd<u8>(); }

	export u16 randu16() { return rnd<u16>(); }

	export u32 randu32() { return rnd<u32>(); }

	export u64 randu64() { return rnd<u64>(); }

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
