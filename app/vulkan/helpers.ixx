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


	export PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT{nullptr};
	export PFN_vkSubmitDebugUtilsMessageEXT    vkSubmitDebugUtilsMessageEXT{nullptr};
	export PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};

	export void initialize_ext_functions(VkInstance instance)
	{
		vkCreateDebugUtilsMessengerEXT =
		  (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

		vkDestroyDebugUtilsMessengerEXT =
		  (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	}


} // namespace deckard::vulkan
