module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:surface;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class presentation_surface
	{
	public:
		bool initialize(VkInstance instance, HINSTANCE window_instance, HWND window_handle) noexcept
		{
			VkWin32SurfaceCreateInfoKHR surface_create_info{.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
			surface_create_info.hinstance = window_instance;
			surface_create_info.hwnd      = window_handle;


			VkResult result = vkCreateWin32SurfaceKHR(instance, &surface_create_info, nullptr, &surface);
			if (result != VK_SUCCESS)
			{
				dbg::println("Could not create vulkan surface on window: {}", string_VkResult(result));
				return false;
			}


			return true;
		}

		void deinitialize(VkInstance instance) noexcept
		{

			if (surface != nullptr)
			{
				vkDestroySurfaceKHR(instance, surface, nullptr);
				surface = nullptr;
			}
		}

	private:
		VkSurfaceKHR surface{nullptr};
	};

} // namespace deckard::vulkan
