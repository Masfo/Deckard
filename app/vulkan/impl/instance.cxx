module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

module deckard.vulkan;

import std;
import deckard.debug;
import deckard.types;
import deckard.assert;
import deckard.as;

namespace deckard::vulkan
{
	[[nodiscard]] VkInstance context::instance() const noexcept { return m_instance; }

	std::expected<void, std::string> context::initialize_instance(u32 minimum_apiversion)
	{
		auto extensions = enumerate<VkExtensionProperties>(
		  [](u32* count, VkExtensionProperties* items)
		  { return vkEnumerateInstanceExtensionProperties(nullptr, count, items); });
		if (not extensions)
			return std::unexpected("Failed to enumerate instance extensions");
		m_available_extensions = std::move(*extensions);

		auto layers = enumerate<VkLayerProperties>(
		  [](u32* count, VkLayerProperties* items) { return vkEnumerateInstanceLayerProperties(count, items); });
		if (not layers)
			return std::unexpected("Failed to enumerate validator layers");
		m_available_layers = std::move(*layers);

		const VkApplicationInfo app_info{
		  .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		  .pApplicationName   = "Deckard",
		  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		  .pEngineName        = "Deckard",
#ifndef _DEBUG
		  .engineVersion = VK_MAKE_VERSION(
			deckard_build::build::major, deckard_build::build::minor, deckard_build::build::patch),
#endif
		  .apiVersion = minimum_apiversion,
		};

		const std::array<const char*, 5> required_extensions{
		  VK_KHR_SURFACE_EXTENSION_NAME,
		  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		  // VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
		  // VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		  VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		  VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

		std::vector<const char*> required_layers;
#ifdef _DEBUG
		required_layers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_crash_diagnostic"};
#endif

		for (const char* extension : required_extensions)
			if (not has_name(m_available_extensions, extension, extension_name))
				return std::unexpected(std::format("Required extension not found: {}", extension));

		for (const char* layer : required_layers)
			if (not has_name(m_available_layers, layer, layer_name))
				return std::unexpected(std::format("Required layer not found: {}", layer));

		const VkInstanceCreateInfo instance_info{
		  .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		  .pApplicationInfo        = &app_info,
		  .enabledLayerCount       = as<u32>(required_layers.size()),
		  .ppEnabledLayerNames     = required_layers.data(),
		  .enabledExtensionCount   = as<u32>(required_extensions.size()),
		  .ppEnabledExtensionNames = required_extensions.data(),
		};

		switch (const VkResult result = vkCreateInstance(&instance_info, nullptr, &m_instance))
		{
			case VK_SUCCESS: return {};
			case VK_ERROR_INCOMPATIBLE_DRIVER: return std::unexpected("Vulkan driver is not compatible.");
			case VK_ERROR_EXTENSION_NOT_PRESENT: return std::unexpected("Vulkan extension not present.");
			case VK_ERROR_LAYER_NOT_PRESENT: return std::unexpected("Vulkan layer not present.");
			default: return std::unexpected(std::format("Failed to create Vulkan instance: {}", string_VkResult(result)));
		}
	}

	void context::deinitialize_instance()
	{
		if (m_instance)
		{
			vkDestroyInstance(m_instance, nullptr);
			m_instance = VK_NULL_HANDLE;
		}
	}

} // namespace deckard::vulkan
