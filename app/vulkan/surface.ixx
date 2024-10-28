module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:surface;

import :device;
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

	private:
		VkSurfaceKHR             surface{nullptr};
		VkSurfaceCapabilitiesKHR surface_capabilities;

	public:
		bool initialize(VkInstance instance, device device, HWND window_handle)
		{
			VkWin32SurfaceCreateInfoKHR surface_create_info{.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
			surface_create_info.hinstance = GetModuleHandle(nullptr);
			surface_create_info.hwnd      = window_handle;


			VkResult result = vkCreateWin32SurfaceKHR(instance, &surface_create_info, nullptr, &surface);
			if (result != VK_SUCCESS)
			{
				dbg::println("Could not create vulkan surface on window: {}", string_VkResult(result));
				return false;
			}

			update(device);

			return true;
		}

		void update(device device)
		{
			// surface capabilities
			VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical_device(), surface, &surface_capabilities);
			if (result != VK_SUCCESS)
				dbg::println("Device surface capabilities query failed: {}", string_VkResult(result));
		}

		void deinitialize(VkInstance instance)
		{

			if (surface != nullptr)
			{
				vkDestroySurfaceKHR(instance, surface, nullptr);
				surface = nullptr;
			}
		}

		VkSurfaceCapabilitiesKHR capabilities() const { return surface_capabilities; }

		VkExtent2D extent() const { return surface_capabilities.currentExtent; }

		operator VkSurfaceKHR() const { return surface; }
	};

} // namespace deckard::vulkan
