module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


module deckard.vulkan;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{

	std::expected<void, std::string> context::initialize_surface(HWND hwnd)
	{
		const VkWin32SurfaceCreateInfoKHR create_info{
		  .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		  .hinstance = GetModuleHandle(nullptr),
		  .hwnd      = hwnd,
		};

		if (VkResult result = vkCreateWin32SurfaceKHR(m_instance, &create_info, nullptr, &m_surface); result != VK_SUCCESS)
			return std::unexpected(std::format("Failed to create Win32 surface: {}", string_VkResult(result)));

		return {};
	}

	[[nodiscard]] std::expected<VkSurfaceCapabilitiesKHR, VkResult> context::query_surface_capabilities() const noexcept
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &capabilities);
			result != VK_SUCCESS)
			return std::unexpected(result);

		return capabilities;
	}

	[[nodiscard]] bool context::surface_extent_changed() const
	{
		const auto capabilities = query_surface_capabilities();
		if (not capabilities)
			return false;

		const VkExtent2D current = capabilities->currentExtent;
		if (current.width == 0 or current.height == 0)
			return false;

		return current.width != m_swapchain_extent.width or current.height != m_swapchain_extent.height;
	}


	[[nodiscard]] VkSurfaceFormatKHR context::select_surface_format() const
	{
		constexpr VkSurfaceFormatKHR default_format{VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		assert::check(m_surface != VK_NULL_HANDLE, "Vulkan surface must be created before selecting surface format");

		auto formats = enumerate<VkSurfaceFormatKHR>(
		  [this](u32* count, VkSurfaceFormatKHR* items)
		  { return vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, count, items); });

		if (not formats or formats->empty() or formats->front().format == VK_FORMAT_UNDEFINED)
			return default_format;

		constexpr std::array<VkSurfaceFormatKHR, 2> preferred{{
		  {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
		  {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
		}};

		for (const auto& want : preferred)
			if (auto have = std::ranges::find_if(
				  *formats, [&](const auto& f) { return f.format == want.format and f.colorSpace == want.colorSpace; });
				have != formats->end())
				return *have;

		// Otherwise, first supported format wins
		return formats->front();
	}

	std::expected<void, std::string> context::update_surface_capabilities()
	{
		if (m_surface == VK_NULL_HANDLE)
			return std::unexpected("Vulkan surface must be created before updating surface capabilities");

		if (m_physical_device == VK_NULL_HANDLE)
			return std::unexpected("Vulkan physical device must be created before updating surface capabilities");

		auto capabilities = query_surface_capabilities();
		if (not capabilities)
			return std::unexpected(
			  std::format("Failed to get surface capabilities: {}", string_VkResult(capabilities.error())));

		m_surface_capabilities = *capabilities;
		return {};
	}

	void context::deinitialize_surface()
	{
		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			m_surface = VK_NULL_HANDLE;
		}
	}

	[[nodiscard]] bool context::can_present_win32(u32 queue_family_index) const
	{
		VkBool32 present_support = VK_FALSE;
		if (vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, queue_family_index, m_surface, &present_support)
			!= VK_SUCCESS)
			return false;

		return present_support == VK_TRUE;
	}

	std::expected<u32, std::string> context::select_queue_family_index()
	{
		auto families = enumerate<VkQueueFamilyProperties>(
		  [this](u32* count, VkQueueFamilyProperties* items)
		  { vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, count, items); });
		if (not families or families->empty())
			return std::unexpected("Failed to get queue family properties");

		for (u32 i = 0; i < families->size(); ++i)
		{
			const auto& family = (*families)[i];
			if (family.queueCount != 0 and (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) and can_present_win32(i))
				return i;
		}

		return std::unexpected("Failed to find a suitable queue family");
	}

} // namespace deckard::vulkan
