export module deckard.colors;


import std;
import deckard.types;


namespace deckard
{
#ifdef __cpp_lib_constexpr_cmath
#error "Use constexpr round";
#endif

	export constexpr u8 clamp_rgb(f32 color) { return static_cast<u8>((std::clamp(color, 0.0f, 1.0f) * 255.0f) + 0.5f); }



	

}; // namespace deckard
