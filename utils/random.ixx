module;
#include <random> // random doesn't work without header (17.8.2024)
export module deckard.random;


#ifndef _DEBUG
import deckard_build;
#endif

import std;

namespace deckard::random
{
	export std::random_device random_device;
	export std::mt19937       mersenne_twister;
	export std::mt19937       mersenne_twister_fixed;

	export std::uniform_real_distribution<float> fdist01(0.0f, 1.0f);
	export std::uniform_real_distribution<float> fdist11(-1.0f, 1.0f);

	export void initialize()
	{
		//
		mersenne_twister.seed(random_device());
		mersenne_twister_fixed.seed(0xB00'B1E5);
	}

	export float random_float01() { return fdist01(mersenne_twister); }

	export float random_float11() { return fdist11(mersenne_twister); }


} // namespace deckard::random
