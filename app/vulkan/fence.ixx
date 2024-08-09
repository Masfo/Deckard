module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


export module deckard.vulkan:fence;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class fence
	{
	public:
		bool initialize(VkDevice device, bool set_signaled = true)
		{
			VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
			fence_info.flags = set_signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

			VkResult result = vkCreateFence(device, &fence_info, nullptr, &m_fence);
			if (result != VK_SUCCESS)
			{
				dbg::println("Fence creation failed: {}", string_VkResult(result));
				return false;
			}
			return true;
		}

		void wait(VkDevice device)
		{
			assert::check(device != nullptr);
			assert::check(m_fence != nullptr);

			VkResult result = vkWaitForFences(device, 1, &m_fence, VK_TRUE, UINT64_MAX);
			assert::check(result == VK_SUCCESS or result == VK_TIMEOUT);
		}

		void reset(VkDevice device)
		{
			assert::check(device != nullptr);
			assert::check(m_fence != nullptr);

			VkResult result = vkResetFences(device, 1, &m_fence);
			assert::check(result == VK_SUCCESS);
		}

		void deinitialize(VkDevice device)
		{
			if (m_fence != nullptr)
				vkDestroyFence(device, m_fence, nullptr);
		}

		operator VkFence() const { return m_fence; }

	private:
		VkFence m_fence{nullptr};
	};

} // namespace deckard::vulkan
