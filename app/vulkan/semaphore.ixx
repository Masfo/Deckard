module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


export module deckard.vulkan:semaphore;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class semaphore
	{

	private:
		VkSemaphore m_semaphore{nullptr};

	public:
		bool initialize(VkDevice device)
		{
			VkSemaphoreCreateInfo semaphore_create{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

			VkResult result = vkCreateSemaphore(device, &semaphore_create, nullptr, &m_semaphore);
			if (result != VK_SUCCESS)
			{
				dbg::println("Semaphore creation failed: {}", string_VkResult(result));
				return false;
			}
			return true;
		}

		void deinitialize(VkDevice device)
		{

			if (m_semaphore != nullptr)
				vkDestroySemaphore(device, m_semaphore, nullptr);
		}

		operator VkSemaphore() const { return m_semaphore; }
	};

} // namespace deckard::vulkan
