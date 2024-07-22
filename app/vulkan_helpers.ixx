module;
// #include <Windows.h>
#include <vulkan/vulkan.h>

export module deckard.vulkan_helpers;
import deckard.debug;
import deckard.types;
import std;

namespace deckard::vulkan
{
	export std::vector<VkExtensionProperties> extensions;
	export std::vector<VkLayerProperties>     validator_layers;

	export PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT{nullptr};
	export PFN_vkSubmitDebugUtilsMessageEXT    vkSubmitDebugUtilsMessageEXT{nullptr};
	export PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};

	export std::string_view result_to_string(VkResult result)
	{
		switch (result)
		{
			case VK_SUCCESS: return "Success";
			case VK_NOT_READY: return "A fence or query has not yet completed";
			case VK_TIMEOUT: return "A wait operation has not completed in the specified time";
			case VK_EVENT_SET: return "An event is signaled";
			case VK_EVENT_RESET: return "An event is unsignaled";
			case VK_INCOMPLETE: return "A return array was too small for the result";
			case VK_ERROR_OUT_OF_HOST_MEMORY: return "A host memory allocation has failed";
			case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "A device memory allocation has failed";
			case VK_ERROR_INITIALIZATION_FAILED:
				return "Initialization of an object could not be completed for implementation-specific reasons";
			case VK_ERROR_DEVICE_LOST: return "The logical or physical device has been lost";
			case VK_ERROR_MEMORY_MAP_FAILED: return "Mapping of a memory object has failed";
			case VK_ERROR_LAYER_NOT_PRESENT: return "A requested layer is not present or could not be loaded";
			case VK_ERROR_EXTENSION_NOT_PRESENT: return "A requested extension is not supported";
			case VK_ERROR_FEATURE_NOT_PRESENT: return "A requested feature is not supported";
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible";
			case VK_ERROR_TOO_MANY_OBJECTS: return "Too many objects of the type have already been created";
			case VK_ERROR_FORMAT_NOT_SUPPORTED: return "A requested format is not supported on this device";
			case VK_ERROR_SURFACE_LOST_KHR: return "A surface is no longer available";
			case VK_SUBOPTIMAL_KHR: return "A swapchain no longer matches the surface properties exactly, but can still be used";
			case VK_ERROR_OUT_OF_DATE_KHR: return "A surface has changed in such a way that it is no longer compatible with the swapchain";
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "The display used by a swapchain does not use the same presentable image layout";
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
				return "The requested window is already connected to a VkSurfaceKHR, or to some other non-Vulkan API";
			case VK_ERROR_VALIDATION_FAILED_EXT: return "A validation layer found an error";
			default: return "ERROR: UNKNOWN VULKAN ERROR";
		}
	}

	export bool enumerate_instance_extensions() noexcept
	{
		u32      count{0};
		VkResult result = vkEnumerateInstanceExtensionProperties(0, &count, 0);

		if (result == VK_SUCCESS)
		{
			extensions.resize(count);
			result = vkEnumerateInstanceExtensionProperties(0, &count, extensions.data());
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
