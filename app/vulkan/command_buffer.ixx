module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:command_buffer;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class command_buffer
	{
	public:
		bool initialize(VkDevice device, u32 queue_index) noexcept
		{
			assert::check(device != nullptr);

			// command buffers
			VkCommandPoolCreateInfo cmd_pool_create{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			cmd_pool_create.queueFamilyIndex = queue_index;
			cmd_pool_create.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			// | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VkResult result = vkCreateCommandPool(device, &cmd_pool_create, nullptr, &m_command_pool);
			if (result != VK_SUCCESS)
			{
				dbg::println("Command pool creation failed: {}", string_VkResult(result));
				return false;
			}


			VkCommandBufferAllocateInfo command_buffer_allocate{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			command_buffer_allocate.commandPool        = m_command_pool;
			command_buffer_allocate.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_allocate.commandBufferCount = 1;

			result = vkAllocateCommandBuffers(device, &command_buffer_allocate, &m_command_buffer);
			if (result != VK_SUCCESS)
			{
				dbg::println("Failed to allocate command buffers: {}", string_VkResult(result));
				return false;
			}

			return true;
		}

		VkResult begin()
		{
			VkCommandBufferBeginInfo begin{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			return vkBeginCommandBuffer(m_command_buffer, &begin);
		}

		VkResult end()
		{
			//
			return vkEndCommandBuffer(m_command_buffer);
		}

		void deinitialize(VkDevice device) noexcept
		{
			if (m_command_buffer != nullptr)
			{
				vkFreeCommandBuffers(device, m_command_pool, 1, &m_command_buffer);
				m_command_buffer = nullptr;
			}

			if (m_command_pool != nullptr)
			{
				vkDestroyCommandPool(device, m_command_pool, nullptr);
				m_command_pool = nullptr;
			}
		}

	private:
		VkCommandPool   m_command_pool{nullptr};
		VkCommandBuffer m_command_buffer{nullptr};
	};

} // namespace deckard::vulkan