module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:command_buffer;

import :device;
import :swapchain;
import :semaphore;

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
		bool initialize(device device, swapchain swapchain)
		{
			assert::check(device != nullptr);

			// command buffers
			VkCommandPoolCreateInfo cmd_pool_create{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
			cmd_pool_create.queueFamilyIndex = device.select_queue();
			cmd_pool_create.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VkResult result = vkCreateCommandPool(device, &cmd_pool_create, nullptr, &m_command_pool);
			if (result != VK_SUCCESS)
			{
				dbg::println("Command pool creation failed: {}", string_VkResult(result));
				return false;
			}


			m_command_buffers.resize(swapchain.count(device));

			VkCommandBufferAllocateInfo command_buffer_allocate{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
			command_buffer_allocate.commandPool        = m_command_pool;
			command_buffer_allocate.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			command_buffer_allocate.commandBufferCount = as<u32>(m_command_buffers.size());

			result = vkAllocateCommandBuffers(device, &command_buffer_allocate, m_command_buffers.data());
			if (result != VK_SUCCESS)
			{
				dbg::println("Failed to allocate command buffers: {}", string_VkResult(result));
				return false;
			}


			return true;
		}

		VkResult begin(u32 index)
		{
			VkCommandBufferBeginInfo begin{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
			begin.flags = 0;
			// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			return vkBeginCommandBuffer(m_command_buffers[index], &begin);
		}

		VkResult end(u32 index)
		{
			//
			return vkEndCommandBuffer(m_command_buffers[index]);
		}

		void deinitialize(VkDevice device)
		{
			if (not m_command_buffers.empty())
			{
				vkFreeCommandBuffers(device, m_command_pool, as<u32>(m_command_buffers.size()), m_command_buffers.data());
				m_command_buffers.clear();
			}

			if (m_command_pool != nullptr)
			{
				vkDestroyCommandPool(device, m_command_pool, nullptr);
				m_command_pool = nullptr;
			}
		}

		VkResult submit(device device, VkSemaphore image_available, VkSemaphore rendering_finished,
						VkSemaphore in_flight_timeline, u64 signal_value, u32 index)
		{
			VkPipelineStageFlags             wait_dest_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			const std::array<VkSemaphore, 2> signal_semaphores{rendering_finished, in_flight_timeline};
			const std::array<u64, 2>         signal_values{0, signal_value}; // entry for the binary semaphore is ignored


			VkTimelineSemaphoreSubmitInfo timeline_info{
			  .sType                     = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			  .signalSemaphoreValueCount = as<u32>(signal_values.size()),
			  .pSignalSemaphoreValues    = signal_values.data(),
			};

			VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .pNext = &timeline_info};

			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores    = &image_available;
			submit_info.pWaitDstStageMask  = &wait_dest_stage_mask;

			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers    = &m_command_buffers[index];

			submit_info.signalSemaphoreCount = as<u32>(signal_semaphores.size());
			submit_info.pSignalSemaphores    = signal_semaphores.data();

			return vkQueueSubmit(device.queue(), 1, &submit_info, VK_NULL_HANDLE);
		}

		VkCommandBuffer& operator[](size_t index) { return m_command_buffers[index]; }

		const VkCommandBuffer current() const { return m_command_buffers[m_index]; }

		VkCommandBuffer& current() { return m_command_buffers[m_index]; }

		const VkCommandBuffer current(u32 index) const { return m_command_buffers[index]; }

		size_t size() const { return m_command_buffers.size(); }

	private:
		VkCommandPool                m_command_pool{nullptr};
		std::vector<VkCommandBuffer> m_command_buffers;
		u32                          m_index{0};
	};

} // namespace deckard::vulkan
