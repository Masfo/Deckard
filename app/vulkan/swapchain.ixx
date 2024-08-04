module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:swapchain;
import deckard.vulkan_helpers;
import :surface;


import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class swapchain
	{
	public:
		bool initialize(VkDevice device, presentation_surface surface) noexcept
		{

			VkSwapchainCreateInfoKHR create_swapchain{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

			auto surface_capabilities     = surface.capabilities();
			u32  desired_number_of_images = std::min(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount);

			create_swapchain.surface       = surface;
			create_swapchain.minImageCount = desired_number_of_images;

			// TODO: query desired format somehow
			VkFormat           swapchain_format;
			VkSurfaceFormatKHR desired_format;

			desired_format               = {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
			swapchain_format             = desired_format.format;
			create_swapchain.imageFormat = swapchain_format;

			create_swapchain.imageColorSpace = desired_format.colorSpace;

			// extent
			create_swapchain.imageExtent      = surface_capabilities.currentExtent;
			create_swapchain.imageArrayLayers = 1;

			VkImageUsageFlags usage{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
			if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
				usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

			usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			create_swapchain.imageUsage = usage;

			create_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

			create_swapchain.queueFamilyIndexCount = 0;
			create_swapchain.pQueueFamilyIndices   = nullptr;

			// surface_capability
			VkSurfaceTransformFlagBitsKHR transform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR};
			transform                     = surface_capabilities.currentTransform;
			create_swapchain.preTransform = transform;

			create_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			// present mode
			VkPresentModeKHR present_mode{VK_PRESENT_MODE_FIFO_KHR};

			std::array<VkPresentModeKHR, 3> try_order{};

			bool vsync = false;

			if (vsync)
			{
				try_order[2] = VK_PRESENT_MODE_MAILBOX_KHR; //
				try_order[1] = VK_PRESENT_MODE_IMMEDIATE_KHR;
				try_order[0] = VK_PRESENT_MODE_FIFO_KHR;    // vsync
			}
			else
			{
				try_order[2] = VK_PRESENT_MODE_MAILBOX_KHR; //
				try_order[1] = VK_PRESENT_MODE_FIFO_KHR;    // vsync
				try_order[0] = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}

			std::vector<VkPresentModeKHR> present_modes;


			for (const auto& mode : present_modes)
			{
				if (mode == try_order[0])
				{
					present_mode = mode;
					break;
				}
			}

			if (present_mode != try_order[0])
				for (const auto& mode : present_modes)
				{
					if (mode == try_order[1])
					{
						present_mode = mode;
						break;
					}
				}
			create_swapchain.presentMode = present_mode;

			// clipped
			create_swapchain.clipped = VK_TRUE;


			VkSwapchainKHR oldSwapchain = m_swapchain;
			// old
			create_swapchain.oldSwapchain = oldSwapchain;


			VkResult result = vkCreateSwapchainKHR(device, &create_swapchain, nullptr, &m_swapchain);
			if (result != VK_SUCCESS)
			{
				dbg::println("Vulkan swapchain creation failed: {}", string_VkResult(result));
				return false;
			}

			if (oldSwapchain != VK_NULL_HANDLE)
			{
				vkDestroySwapchainKHR(device, oldSwapchain, NULL);
			}


			return true;
		}

		void deinitialize(VkDevice device) noexcept { vkDestroySwapchainKHR(device, m_swapchain, nullptr); }

	private:
		VkSwapchainKHR m_swapchain{nullptr};
	};

} // namespace deckard::vulkan
