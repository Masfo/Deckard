module;
// #include <Windows.h>
#include <vulkan/vulkan.h>

export module deckard.vulkan_helpers;
import deckard.debug;
import deckard.types;
import deckard.as;
import std;

namespace deckard::vulkan
{
	export std::vector<VkExtensionProperties> instance_extensions;
	export std::vector<VkExtensionProperties> device_extensions;
	export std::vector<VkLayerProperties>     validator_layers;

	export PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT{nullptr};
	export PFN_vkSubmitDebugUtilsMessageEXT    vkSubmitDebugUtilsMessageEXT{nullptr};
	export PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};

	export bool enumerate_instance_extensions() noexcept
	{
		u32      count{0};
		VkResult result = vkEnumerateInstanceExtensionProperties(0, &count, 0);

		if (result == VK_SUCCESS)
		{
			instance_extensions.resize(count);
			result = vkEnumerateInstanceExtensionProperties(0, &count, instance_extensions.data());
		}

		return result == VK_SUCCESS;
	}

	export bool enumerate_validator_layers() noexcept
	{
		u32      count{0};
		VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (result == VK_SUCCESS)
		{

			validator_layers.resize(count);
			result = vkEnumerateInstanceLayerProperties(&count, validator_layers.data());
		}
		return result == VK_SUCCESS;
	}

	export void initialize_ext_functions(VkInstance instance)
	{
		vkCreateDebugUtilsMessengerEXT =
		  (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

		vkDestroyDebugUtilsMessengerEXT =
		  (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	}


} // namespace deckard::vulkan
