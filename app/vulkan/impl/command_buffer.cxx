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

	
		std::expected<void, std::string> context::allocate_command_buffers(std::span<VkCommandBuffer> out) const
	{
		const VkCommandBufferAllocateInfo allocate_info{
		  .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		  .commandPool        = m_command_pool,
		  .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		  .commandBufferCount = as<u32>(out.size()),
		};

		if (VkResult result = vkAllocateCommandBuffers(m_device, &allocate_info, out.data()); result != VK_SUCCESS)
			return std::unexpected(std::format("Failed to allocate command buffers: {}", string_VkResult(result)));

		return {};
	}

	std::expected<void, std::string> context::initialize_command_buffers()
	{
		assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing command buffer");

		// transient command buffers are short-lived and can be reset after execution, which can improve performance
		const VkCommandPoolCreateInfo pool_info{
		  .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		  .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		  .queueFamilyIndex = m_graphics_queue_family_index,
		};

		if (VkResult result = vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool); result != VK_SUCCESS)
			return std::unexpected(std::format("Failed to create command pool: {}", string_VkResult(result)));

		m_command_buffers.resize(swapchain_image_count);
		return allocate_command_buffers(m_command_buffers);
	}

	void context::deinitialize_command_buffers()
	{
		if (not m_command_buffers.empty())
		{
			vkFreeCommandBuffers(m_device, m_command_pool, as<u32>(m_command_buffers.size()), m_command_buffers.data());
			m_command_buffers.clear();
		}

		destroy(m_command_pool, vkDestroyCommandPool);
	}

	[[nodiscard]] VkCommandBuffer& context::get_command_buffer(u32 index)
	{
		assert::check(index < m_command_buffers.size(), "Command buffer index out of range");
		return m_command_buffers[index];
	}

	// submit

	VkResult context::submit(VkSemaphore wait, VkCommandBuffer cmd, VkSemaphore rendering_finished, u64 signal_value) const
	{
		const VkSemaphoreSubmitInfo wait_info{
		  .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		  .semaphore = wait,
		  .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, // covered by the top barrier's srcStageMask
		};

		const std::array<VkSemaphoreSubmitInfo, 2> signal_infos{{
		  {.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		   .semaphore = rendering_finished,
		   .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT}, // binary: value ignored
		  {.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		   .semaphore = m_in_flight_timeline,
		   .value     = signal_value,
		   .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT},
		}};

		const VkCommandBufferSubmitInfo cmd_info{
		  .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		  .commandBuffer = cmd,
		};

		const VkSubmitInfo2 submit_info{
		  .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		  .waitSemaphoreInfoCount   = 1,
		  .pWaitSemaphoreInfos      = &wait_info,
		  .commandBufferInfoCount   = 1,
		  .pCommandBufferInfos      = &cmd_info,
		  .signalSemaphoreInfoCount = as<u32>(signal_infos.size()),
		  .pSignalSemaphoreInfos    = signal_infos.data(),
		};

		return vkQueueSubmit2(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	}

	VkResult context::submit_command() const
	{
		return submit(
		  m_image_available[m_currentframe],
		  m_command_buffers[m_command_buffer_index],
		  m_rendering_finished[m_command_buffer_index],
		  m_signal_value);
	}

	VkResult context::submit_frame(const frame_slot& frame, u32 image_index, u64 signal_value) const
	{
		return submit(frame.image_available, frame.cmd, m_rendering_finished[image_index], signal_value);
	}

} // namespace deckard::vulkan
