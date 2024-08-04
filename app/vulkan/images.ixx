module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:images;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class images
	{
	public:
		bool initialize(VkInstance instance, HINSTANCE window_instance, HWND window_handle) noexcept
		{
			//
			return true;
		}

		void deinitialize(VkInstance instance) noexcept
		{
			//
		}

	private:
	};

} // namespace deckard::vulkan
