module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

module deckard.vulkan;


import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;


namespace deckard::vulkan
{
	void context::set_debug_name([[maybe_unused]] VkDevice dev, u64 handle, VkObjectType type, std::string_view name) noexcept
	{
#ifdef _DEBUG
		const VkDebugUtilsObjectNameInfoEXT info{
		  .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		  .objectType   = type,
		  .objectHandle = handle,
		  .pObjectName  = name.data(),
		};
		// Load once via vkGetInstanceProcAddr in real code
		static auto fn = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
		  vkGetDeviceProcAddr(dev, "vkSetDebugUtilsObjectNameEXT"));
		if (fn)
			fn(dev, &info);
#endif
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL context::static_debug_callback(
	  [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
	  [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT             message_type,
	  [[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* callback_data, [[maybe_unused]] void* userdata)
	{
#ifdef _DEBUG
		auto* self = static_cast<context*>(userdata);

		const std::string_view severity = [&]() -> std::string_view
		{
			switch (message_severity)
			{
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "Info";
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
				case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "Error";
				default: return "Unknown";
			}
		}();

		const std::string_view type = [&]() -> std::string_view
		{
			switch (message_type)
			{
				case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
				case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
				case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
				case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "Device address binding";
				default: return "Unknown";
			}
		}();

		if (callback_data->pMessage and self->debug_callback)
			self->debug_callback(severity, type, std::string_view(callback_data->pMessage));
#endif
		return VK_FALSE;
	}

	std::expected<void, std::string> context::initialize_debug_functions()
	{
#ifdef _DEBUG
		assert::check(m_instance != VK_NULL_HANDLE, "Vulkan instance must be created before initializing debug functions");

		vkCreateDebugUtilsMessengerEXT = load_instance_proc<PFN_vkCreateDebugUtilsMessengerEXT>(
		  "vkCreateDebugUtilsMessengerEXT");
		vkSubmitDebugUtilsMessageEXT = load_instance_proc<PFN_vkSubmitDebugUtilsMessageEXT>("vkSubmitDebugUtilsMessageEXT");
		vkDestroyDebugUtilsMessengerEXT = load_instance_proc<PFN_vkDestroyDebugUtilsMessengerEXT>(
		  "vkDestroyDebugUtilsMessengerEXT");

		if (not vkCreateDebugUtilsMessengerEXT or not vkSubmitDebugUtilsMessageEXT or not vkDestroyDebugUtilsMessengerEXT)
			return std::unexpected("Failed to load debug functions");
#endif
		return {};
	}

	std::expected<void, std::string> context::initialize_debug()
	{
#ifdef _DEBUG
		if (auto result = initialize_debug_functions(); not result)
			return result;

		constexpr VkDebugUtilsMessageSeverityFlagsEXT severity
		  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

		const VkDebugUtilsMessengerCreateInfoEXT create_info{
		  .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		  .messageSeverity = severity,
		  .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
							 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		  .pfnUserCallback = &context::static_debug_callback,
		  .pUserData       = this,
		};

		assert::check(m_instance != VK_NULL_HANDLE, "Vulkan instance must be created before initializing debug messenger");

		if (VkResult result = vkCreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &debug_messenger);
			result != VK_SUCCESS)
			return std::unexpected(std::format("Failed to create debug messenger: {}", string_VkResult(result)));
#endif
		return {};
	}

	void context::deinitialize_debug()
	{
#ifdef _DEBUG
		if (m_instance == VK_NULL_HANDLE)
			return;

		if (debug_messenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(m_instance, debug_messenger, nullptr);
			debug_messenger = VK_NULL_HANDLE;
		}
#endif
	}
}
