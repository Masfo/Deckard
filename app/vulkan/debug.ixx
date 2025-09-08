module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

export module deckard.vulkan:debug;

import :core;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{

	bool debug_callback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
						[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT             messageType,
						[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
	{

		#ifdef _DEBUG
		[[maybe_unused]]core* self = reinterpret_cast<core*>(pUserData);

		std::string_view severity, type;

		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "Verbose"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "Info"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "Warning"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "Error"; break;
			default: severity = "Unknown"; break;
		}

		switch (messageType)
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: type = "General"; break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: type = "Validation"; break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: type = "Performance"; break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: type = "Device address binding"; break;
			default: type = "Unknown"; break;
		}

		if (pCallbackData->pMessage)
			dbg::println("Vulkan {}.{}: {}", severity, type, pCallbackData->pMessage);
		#endif
		return VK_FALSE;
	}

	bool core::initialize_debug_functions([[maybe_unused]] void* userdata = nullptr)
	{
		#ifdef _DEBUG
		vkCreateDebugUtilsMessengerEXT =
		  (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

		vkDestroyDebugUtilsMessengerEXT =
		  (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");


		// TODO: way to dynamically filter messages?
		u32 severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		// severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		//
		// debug
		VkDebugUtilsMessengerCreateInfoEXT create{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		create.messageSeverity = severity;
		create.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create.pfnUserCallback = as<PFN_vkDebugUtilsMessengerCallbackEXT>(debug_callback);
		create.pUserData       = userdata;

		VkResult result{};
		result = vkCreateDebugUtilsMessengerEXT(instance, &create, nullptr, &debug_messenger);
		if (result != VK_SUCCESS)
		{
			dbg::println("Creating Vulkan debug messenger failed: {}", string_VkResult(result));
			return false;
		}

		#endif
		return true;
	}

	void initialize_ext_functions([[maybe_unused]] VkInstance instance)
	{
		#ifdef _DEBUG
		vkCreateDebugUtilsMessengerEXT =
		  (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

		vkDestroyDebugUtilsMessengerEXT =
		  (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		#endif
	}

	bool debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
						const VkDebugUtilsMessengerCallbackDataEXT*, void*);

	export class debug
	{
	public:
		bool initialize(const VkInstance instance, void* userdata)
		{

			assert::check(instance != nullptr);

			// debug functions
			initialize_ext_functions(instance);

			// TODO: way to dynamically filter messages?
			u32 severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			// severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
			severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
			//
			// debug
			VkDebugUtilsMessengerCreateInfoEXT create{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
			create.messageSeverity = severity;
			create.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			create.pfnUserCallback = as<PFN_vkDebugUtilsMessengerCallbackEXT>(debug_callback);
			create.pUserData       = userdata;

			VkResult result{};
			result = vkCreateDebugUtilsMessengerEXT(instance, &create, nullptr, &debug_messenger);
			if (result != VK_SUCCESS)
			{
				dbg::println("Creating Vulkan debug messenger failed: {}", string_VkResult(result));
				return false;
			}

			return true;
		}

		void deinitialize(const VkInstance instance)
		{
			if (vkDestroyDebugUtilsMessengerEXT != nullptr)
				vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
		}


	private:
		VkDebugUtilsMessengerEXT debug_messenger{nullptr};
	};

} // namespace deckard::vulkan
