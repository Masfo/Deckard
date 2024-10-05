module;
#include <random> // random doesn't work without header (17.8.2024)
export module deckard.random;


#ifndef _DEBUG
import deckard_build;
#endif

import std;
import deckard.types;

namespace deckard::random
{
	export std::random_device random_device;
	export std::mt19937       mersenne_twister;
	export std::mt19937       mersenne_twister_fixed;


	// signed
	export std::uniform_int_distribution<i16> i8dist(min_value<i8>, max_value<i8>); // Note: cannot be u8 dist
	export std::uniform_int_distribution<i16> i16dist(min_value<i16>, max_value<i16>);
	export std::uniform_int_distribution<i32> i32dist(min_value<i32>, max_value<i32>);
	export std::uniform_int_distribution<i64> i64dist(min_value<i64>, max_value<i64>);

	// unsigned
	export std::uniform_int_distribution<u16> u8dist(min_value<u8>, max_value<u8>); // Note: cannot be u8 dist
	export std::uniform_int_distribution<u16> u16dist(min_value<u16>, max_value<u16>);
	export std::uniform_int_distribution<u32> u32dist(min_value<u32>, max_value<u32>);
	export std::uniform_int_distribution<u64> u64dist(min_value<u64>, max_value<u64>);


	export std::uniform_real_distribution<float> fdist01(0.0f, 1.0f);
	export std::uniform_real_distribution<float> fdist11(-1.0f, 1.0f);

	export void initialize()
	{
		//
		mersenne_twister.seed(random_device());
		mersenne_twister_fixed.seed(0xB00'B1E5);
	}

	// signed
	export i8 randi8() { return static_cast<i8>(i8dist(mersenne_twister)); }

	export i16 randi16() { return i16dist(mersenne_twister); }

	export i32 randi32() { return i32dist(mersenne_twister); }

	export i64 randi64() { return i64dist(mersenne_twister); }

	// unsigned
	export u8 randu8() { return static_cast<u8>(u8dist(mersenne_twister)); }

	export u16 randu16() { return u16dist(mersenne_twister); }

	export u32 randu32() { return u32dist(mersenne_twister); }

	export u64 randu64() { return u64dist(mersenne_twister); }

	export auto rand() { return randi32(); }

	export float random_float01() { return fdist01(mersenne_twister); }

	export float random_float11() { return fdist11(mersenne_twister); }


} // namespace deckard::random
