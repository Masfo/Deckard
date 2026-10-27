module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


module deckard.vulkan;

import std;
import deckard.types;
import deckard.debug;

namespace deckard::vulkan
{
	// vulkan.ixx
	extern constexpr VkImageSubresourceRange color_range;




	std::expected<void, std::string> context::initialize_swapchain()
	{
		assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing swapchain");

		if (auto result = update_surface_capabilities(); not result)
			return result;

		u32 min_image_count = m_surface_capabilities.minImageCount + 1;
		if (m_surface_capabilities.maxImageCount != 0)
			min_image_count = std::min(min_image_count, m_surface_capabilities.maxImageCount);

		m_surface_format = select_surface_format();

		// usage: transfer src/dst only when the surface supports them
		const VkImageUsageFlags usage
		  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| (m_surface_capabilities.supportedUsageFlags
			   & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));

		// present mode: first one from the try-order that the surface offers, FIFO is always available
		auto present_modes = enumerate<VkPresentModeKHR>(
		  [this](u32* count, VkPresentModeKHR* items)
		  { return vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, count, items); });
		if (not present_modes)
			return std::unexpected(
			  std::format("Failed to get surface present modes: {}", string_VkResult(present_modes.error())));

		static constexpr std::array vsync_order{
		  VK_PRESENT_MODE_FIFO_KHR,
		  VK_PRESENT_MODE_FIFO_RELAXED_KHR,
		  VK_PRESENT_MODE_MAILBOX_KHR,
		  VK_PRESENT_MODE_IMMEDIATE_KHR};

		static constexpr std::array no_vsync_order{
		  VK_PRESENT_MODE_IMMEDIATE_KHR,
		  VK_PRESENT_MODE_FIFO_KHR,
		  VK_PRESENT_MODE_FIFO_RELAXED_KHR,
		  VK_PRESENT_MODE_MAILBOX_KHR};

		const auto& try_order    = vsync ? vsync_order : no_vsync_order;
		const auto  preferred    = std::ranges::find_first_of(try_order, *present_modes);
		const auto  present_mode = preferred != try_order.end() ? *preferred : VK_PRESENT_MODE_FIFO_KHR;

		const VkSwapchainKHR old_swapchain = m_swapchain;

		const VkSwapchainCreateInfoKHR create_info{
		  .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		  .surface               = m_surface,
		  .minImageCount         = min_image_count,
		  .imageFormat           = m_surface_format.format,
		  .imageColorSpace       = m_surface_format.colorSpace,
		  .imageExtent           = m_surface_capabilities.currentExtent,
		  .imageArrayLayers      = 1,
		  .imageUsage            = usage,
		  .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
		  .queueFamilyIndexCount = 0,
		  .pQueueFamilyIndices   = nullptr,
		  .preTransform          = m_surface_capabilities.currentTransform,
		  .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		  .presentMode           = present_mode,
		  .clipped               = VK_TRUE,
		  .oldSwapchain          = old_swapchain,
		};

		if (VkResult result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain); result != VK_SUCCESS)
			return std::unexpected(std::format("Failed to create swapchain: {}", string_VkResult(result)));

		m_swapchain_transfer_src = (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
		if (old_swapchain != VK_NULL_HANDLE)
			vkDestroySwapchainKHR(m_device, old_swapchain, nullptr);

		// refresh the extent/capabilities of the new swapchain
		return update_surface_capabilities();
	}

	std::expected<void, std::string> context::resize_swapchain() { return initialize_swapchain(); }

	void context::deinitialize_swapchain() { destroy(m_swapchain, vkDestroySwapchainKHR); }

	// images

	std::expected<void, std::string> context::initialize_swapchain_images()
	{
		if (m_swapchain == VK_NULL_HANDLE)
			return std::unexpected("Vulkan swapchain must be created before initializing swapchain images");

		auto images = enumerate<VkImage>(
		  [this](u32* count, VkImage* items) { return vkGetSwapchainImagesKHR(m_device, m_swapchain, count, items); });
		if (not images)
			return std::unexpected(std::format("Failed to get swapchain images: {}", string_VkResult(images.error())));

		m_swapchain_images    = std::move(*images);
		swapchain_image_count = as<u32>(m_swapchain_images.size());

		// image views
		m_swapchain_image_views.assign(swapchain_image_count, VK_NULL_HANDLE);

		for (std::size_t i = 0; i < m_swapchain_images.size(); ++i)
		{
			const VkImageViewCreateInfo view_info{
			  .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			  .image            = m_swapchain_images[i],
			  .viewType         = VK_IMAGE_VIEW_TYPE_2D,
			  .format           = m_surface_format.format, // TODO: better format selection
			  .subresourceRange = color_range,             // components default to IDENTITY
			};

			if (VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &m_swapchain_image_views[i]);
				result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create image view: {}", string_VkResult(result)));
		}

		m_swapchain_extent = m_surface_capabilities.currentExtent;

		return {};
	}

	std::expected<void, std::string> context::resize_swapchain_images()
	{
		deinitialize_swapchain_images();
		return initialize_swapchain_images();
	}

	void context::deinitialize_swapchain_images()
	{
		for (auto& view : m_swapchain_image_views)
			destroy(view, vkDestroyImageView);

		m_swapchain_image_views.clear();
		m_swapchain_images.clear();
	}



	
		[[nodiscard]] VkImage context::get_swapchain_image(u32 index) const
	{
		assert::check(index < m_swapchain_images.size(), "Swapchain image index out of range");
		return m_swapchain_images[index];
	}

	[[nodiscard]] VkImageView context::get_swapchain_image_view(u32 index) const
	{
		assert::check(index < m_swapchain_image_views.size(), "Swapchain image view index out of range");
		return m_swapchain_image_views[index];
	}

} // namespace deckard::vulkan
