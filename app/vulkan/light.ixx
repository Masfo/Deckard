export module deckard.vulkan.light;

import deckard.types;
import deckard.vec;

namespace deckard::vulkan
{
	export struct light
	{
		vec::vec3 position{0.0f, 0.0f, 0.0f};
		vec::vec3 color{1.0f, 1.0f, 1.0f};
		f32       intensity{1.0f};
		f32       radius{1.0f};
		f32       padding[2]{};
	};
}; // namespace deckard::vulkan
