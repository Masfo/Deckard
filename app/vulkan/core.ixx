module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:core;

import std;
import deckard.debug;
import deckard.types;

namespace deckard::vulkan
{
	export class core
	{
	private:
		// instance
		std::vector<VkLayerProperties>     validator_layers;
		std::vector<VkExtensionProperties> instance_extensions;
		VkInstance                         instance{nullptr};

		bool enumerate_instance_extensions(std::vector<VkExtensionProperties>& );
		bool enumerate_validator_layers(std::vector<VkLayerProperties>& );
		[[nodiscard("Check the instance status")]] bool initialize_instance(u32 );

		// Device
		std::vector<VkExtensionProperties> device_extensions;
		VkDevice                           device{nullptr};
		VkPhysicalDevice                   physical_device{nullptr};
		VkQueue                            queue{nullptr};

		// Surface
		VkSurfaceKHR             surface{nullptr};
		VkSurfaceCapabilitiesKHR surface_capabilities;

		// Swapchain
		VkSwapchainKHR m_swapchain{nullptr};


		// Debug
		VkDebugUtilsMessengerEXT debug_messenger{nullptr};

	public:
		void deinitialize()
		{
			if (instance != nullptr)
			{
				vkDestroyInstance(instance, nullptr);
				instance = nullptr;
			}
		}

	public:

	};


} // namespace deckard::vulkan
