module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:images;

import :swapchain;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class images
	{
	private:
		std::vector<VkImage>     swapchain_images{};
		std::vector<VkImageView> swapchain_imageviews{};

	public:
		bool initialize(VkDevice device, swapchain swapchain)
		{
			u32      swapchain_image_count{};
			VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
			if (result == VK_SUCCESS)
			{
				swapchain_images.resize(swapchain_image_count);
				result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());
				if (result != VK_SUCCESS)
				{
					dbg::println("Getting Vulkan swapchain images failed: {}", string_VkResult(result));
					return false;
				}
			}
			else
			{
				dbg::println("Getting Vulkan swapchain images failed: {}", string_VkResult(result));
				return false;
			}

			// swapchain imageviews
			swapchain_imageviews.resize(swapchain_image_count);


			for (size_t i = 0; i < swapchain_images.size(); ++i)
			{
				VkImageViewCreateInfo iv_create{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
				iv_create.image    = swapchain_images[i];
				iv_create.viewType = VK_IMAGE_VIEW_TYPE_2D;

				// TODO: better format getter
				iv_create.format = swapchain.desired_format().format;

				iv_create.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				iv_create.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				iv_create.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				iv_create.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				iv_create.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
				iv_create.subresourceRange.baseMipLevel   = 0;
				iv_create.subresourceRange.levelCount     = 1;
				iv_create.subresourceRange.baseArrayLayer = 0;
				iv_create.subresourceRange.layerCount     = 1;

				result = vkCreateImageView(device, &iv_create, nullptr, &swapchain_imageviews[i]);
				if (result != VK_SUCCESS)
				{
					dbg::println("Vulkan create image view for swapchain failed: {}", string_VkResult(result));
					return false;
				}
			}
			return true;
		}

		void deinitialize(VkDevice device)
		{

			for (auto& view : swapchain_imageviews)
				vkDestroyImageView(device, view, nullptr);
		}

		VkImage image(u32 index) const { return swapchain_images[index]; }

		VkImageView imageview(u32 index) const { return swapchain_imageviews[index]; }
	};

} // namespace deckard::vulkan
