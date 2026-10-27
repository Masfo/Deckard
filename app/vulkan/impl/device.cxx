module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_win32.h>

module deckard.vulkan;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;
import deckard.helpers;

namespace deckard::vulkan
{

	// Extensions that are enabled when the device offers them.
	constexpr std::array<const char*, 8> wanted_device_extensions{
	  VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
	  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	  VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	  VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
	  VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
	  VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
	  VK_KHR_MAINTENANCE2_EXTENSION_NAME,
	  VK_EXT_MEMORY_BUDGET_EXTENSION_NAME};

	// Prefers discrete, then integrated, then the first device. Devices missing the required features or
	// API version are skipped while looking for discrete/integrated.
	std::expected<void, std::string> context::select_physical_device()
	{
		auto devices = enumerate<VkPhysicalDevice>(
		  [this](u32* count, VkPhysicalDevice* items) { return vkEnumeratePhysicalDevices(m_instance, count, items); });
		if (not devices)
			return std::unexpected(
			  std::format("Failed to enumerate physical devices: {}", string_VkResult(devices.error())));

		// TODO: Implement scoring
		std::optional<VkPhysicalDevice> discrete;
		std::optional<VkPhysicalDevice> integrated;

		for (const VkPhysicalDevice device : *devices)
		{
			VkPhysicalDeviceProperties props{};
			vkGetPhysicalDeviceProperties(device, &props);

			VkPhysicalDeviceFeatures2        features2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
			VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
			VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
			VkPhysicalDeviceVulkan14Features features14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};

			vulkan_pnext_chain(features2, features12, features13, features14);

			vkGetPhysicalDeviceFeatures2(device, &features2);

			if (not features13.dynamicRendering or not features13.synchronization2 or not features12.timelineSemaphore)
			{
				dbg::println(
				  "Skipping device {}: need dynamic rendering, synchronization2 and timelineSemaphore", props.deviceName);
				continue;
			}

			if (not version_at_least(
				  props.apiVersion, VK_API_VERSION_MAJOR(minimum_api_version), VK_API_VERSION_MINOR(minimum_api_version)))
			{
				dbg::println(
				  "Skipping device {}: {}.{}.{}, requires at least {}.{}",
				  props.deviceName,
				  VK_API_VERSION_MAJOR(props.apiVersion),
				  VK_API_VERSION_MINOR(props.apiVersion),
				  VK_API_VERSION_PATCH(props.apiVersion),
				  VK_API_VERSION_MAJOR(minimum_api_version),
				  VK_API_VERSION_MINOR(minimum_api_version));
				continue;
			}

			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU and not discrete)
				discrete = device;
			else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU and not integrated)
				integrated = device;
		}

		if (discrete)
			m_physical_device = *discrete;
		else if (integrated)
			m_physical_device = *integrated;
		else if (not devices->empty())
			m_physical_device = devices->front();
		else
			return std::unexpected("No suitable Vulkan devices found");

		return {};
	}

	void context::log_device_info() const
	{
		VkPhysicalDeviceVulkan12Properties vulkan12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
		VkPhysicalDeviceVulkan13Properties vulkan13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
		VkPhysicalDeviceVulkan14Properties vulkan14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES};
		VkPhysicalDeviceProperties2        properties2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

		vulkan_pnext_chain(properties2, vulkan12, vulkan13, vulkan14);

		vkGetPhysicalDeviceProperties2(m_physical_device, &properties2);

		dbg::println(
		  "Conformance version: {}.{}.{}.{}",
		  vulkan12.conformanceVersion.major,
		  vulkan12.conformanceVersion.minor,
		  vulkan12.conformanceVersion.subminor,
		  vulkan12.conformanceVersion.patch);
		dbg::println("Device name: {}", properties2.properties.deviceName);
		dbg::println("Driver name: {}", vulkan12.driverName);
		dbg::println("Driver version: {}.0.{}",
					 VK_VERSION_MAJOR(properties2.properties.driverVersion),
					 VK_VERSION_PATCH(properties2.properties.driverVersion));
	}

	std::expected<void, std::string> context::create_device()
	{
		auto extensions = enumerate<VkExtensionProperties>(
		  [this](u32* count, VkExtensionProperties* items)
		  { return vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, count, items); });

		if (not extensions)
		{
			return std::unexpected(
			  std::format("Failed to enumerate device extensions: {}", string_VkResult(extensions.error())));
		}

		m_device_extensions = std::move(*extensions);

		log_device_info();

		auto queue_family = select_queue_family_index();
		if (not queue_family)
			return std::unexpected(queue_family.error());
		m_graphics_queue_family_index = *queue_family;

		const f32                     queue_priority = 1.0f;
		const VkDeviceQueueCreateInfo queue_info{
		  .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		  .queueFamilyIndex = m_graphics_queue_family_index,
		  .queueCount       = 1,
		  .pQueuePriorities = &queue_priority,
		};

		std::vector<const char*> enabled_extensions;
		for (const char* name : wanted_device_extensions)
			if (has_name(m_device_extensions, name, extension_name))
				enabled_extensions.push_back(name);

		bool                                           enable_unified_image_layouts{false};
		VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unified_image_layouts{
		  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR};
		if (has_name(m_device_extensions, VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME, extension_name))
		{
			VkPhysicalDeviceFeatures2 features2{
			  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &unified_image_layouts};
			vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);

			if (unified_image_layouts.unifiedImageLayouts)
			{
				enabled_extensions.push_back(VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME);
				unified_image_layouts.unifiedImageLayouts = VK_TRUE;
				enable_unified_image_layouts              = true;
			}
		}

		// required features
		VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
		features13.synchronization2 = VK_TRUE;                             // Barrier synchronization2
		features13.dynamicRendering = VK_TRUE;                             // Dynamic rendering

		VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
		features12.timelineSemaphore = VK_TRUE;                            // Timeline semaphores

		features12.descriptorIndexing                           = VK_TRUE; // Descriptor indexing
		features12.shaderSampledImageArrayNonUniformIndexing    = VK_TRUE; // Non-uniform indexing of sampled image arrays

		features12.runtimeDescriptorArray                       = VK_TRUE; // Runtime descriptor arrays
		features12.descriptorBindingPartiallyBound              = VK_TRUE; // Partially bound descriptor sets
		features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE; // Update-after-bind for sampled images
		features12.descriptorBindingVariableDescriptorCount     = VK_TRUE; // Variable descriptor count

		features12.bufferDeviceAddress = VK_TRUE;                          // Buffer device address

		VkPhysicalDeviceShaderObjectFeaturesEXT shader_object{
		  .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
		  .shaderObject = VK_TRUE, // Shader object
		};

		if (unified_image_layouts.unifiedImageLayouts)
			vulkan_pnext_chain(features13, features12, shader_object, unified_image_layouts);
		else
			vulkan_pnext_chain(features13, features12, shader_object);

		const VkDeviceCreateInfo device_info{
		  .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		  .pNext                   = &features13,
		  .queueCreateInfoCount    = 1,
		  .pQueueCreateInfos       = &queue_info,
		  .enabledExtensionCount   = as<u32>(enabled_extensions.size()),
		  .ppEnabledExtensionNames = enabled_extensions.data(),
		};

		if (VkResult result = vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device); result != VK_SUCCESS)
			return std::unexpected(std::format("Failed to create Vulkan device: {}", string_VkResult(result)));

		m_unified_image_layouts = enable_unified_image_layouts;
		vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue);
		if (m_graphics_queue == VK_NULL_HANDLE)
			return std::unexpected("Failed to get graphics queue from device");

		return {};
	}

	[[nodiscard]] VkDevice context::device() const noexcept { return m_device; }

	std::expected<void, std::string> context::initialize_device()
	{
		assert::check(m_instance != VK_NULL_HANDLE, "Vulkan instance must be created before initializing device");

		return select_physical_device()
		  .and_then([this] { return create_device(); })
		  .and_then([this] { return update_surface_capabilities(); });
	}

	void context::deinitialize_device()
	{
		if (m_device != VK_NULL_HANDLE)
		{
			vkDeviceWaitIdle(m_device);
			vkDestroyDevice(m_device, nullptr);
			m_device = VK_NULL_HANDLE;
		}
	}

} // namespace deckard::vulkan
