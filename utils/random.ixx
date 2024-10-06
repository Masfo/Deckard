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
	std::random_device rd;
	std::mt19937       mersenne_twister;
	std::mt19937       mersenne_twister_fixed;

	// signed
	std::uniform_int_distribution<i16> i8dist(min_value<i8>, max_value<i8>);
	std::uniform_int_distribution<i16> i16dist;
	std::uniform_int_distribution<i32> i32dist;
	std::uniform_int_distribution<i64> i64dist;

	// unsigned
	std::uniform_int_distribution<u16> u8dist(min_value<u8>, max_value<u8>);
	std::uniform_int_distribution<u16> u16dist;
	std::uniform_int_distribution<u32> u32dist;
	std::uniform_int_distribution<u64> u64dist;

	std::uniform_real_distribution<float> fdist11(-1.0f, 1.0f);
	std::uniform_real_distribution<float> fdist01(0.0f, 1.0f);

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

	export void random_bytes(std::span<u8> buffer)
	{
		std::ranges::generate(buffer, [] { return randu8(); });
	}

	export void initialize()
	{
		std::random_device::result_type random_data[(64 - 1) / sizeof(rd()) + 1];
		std::generate(std::begin(random_data), std::end(random_data), std::ref(rd));
		std::seed_seq seeds(std::begin(random_data), std::end(random_data));

		mersenne_twister.seed(seeds);
		mersenne_twister.discard(700000);

		mersenne_twister_fixed.seed(0xB00'B1E5);
		mersenne_twister_fixed.discard(700000);
	}


} // namespace deckard::random
