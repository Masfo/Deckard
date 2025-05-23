module;
#include <windows.h>

#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

export module deckard.vulkan;

export import :instance;
export import :device;
export import :debug;
export import :surface;
export import :swapchain;
export import :command_buffer;
export import :semaphore;
export import :fence;
export import :images;
export import :core;
export import :texture;

/*
module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:queue;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class queue
	{
	public:
		bool initialize(VkInstance instance, HINSTANCE window_instance, HWND window_handle)
		{
			//
			return true;
		}

		void deinitialize(VkInstance instance)
		{
			//
		}

	private:
	};

} // namespace deckard::vulkan

*/

import std;
import deckard.vulkan_helpers;
import deckard.types;

namespace deckard::vulkan
{
	// Vulkan 1.3: https://developer.nvidia.com/blog/advanced-api-performance-vulkan-clearing-and-presenting/


	export class vulkan
	{
	public:
		vulkan() = default;

		vulkan(HWND handle, bool vsync) { initialize(handle, vsync); }

		~vulkan() { deinitialize(); };

		// Copy
		vulkan(vulkan const&)            = delete;
		vulkan& operator=(vulkan const&) = delete;

		// Move
		vulkan(vulkan&&)            = delete;
		vulkan& operator=(vulkan&&) = delete;


		bool initialize(HWND handle, bool vsync);
		void deinitialize();

		void resize();
		void resize_images();


		bool draw();
		void record_commands();

		void wait() { m_device.wait(); }

		void vsync(bool v) { m_vsync = v; }

	private:
#ifdef _DEBUG
		debug m_debug;
#endif
		instance             m_instance;
		device               m_device;
		presentation_surface m_surface;
		command_buffer       m_command_buffer;
		swapchain            m_swapchain;
		images               m_images;

		semaphore image_available;
		semaphore rendering_finished;
		fence     in_flight;

		bool is_initialized{false};
		bool m_vsync{true};
	};

	bool vulkan::initialize(HWND handle, bool vsync)
	{
		m_vsync = vsync;

		is_initialized = m_instance.initialize();
#ifdef _DEBUG
		is_initialized &= m_debug.initialize(m_instance, nullptr);
#endif

		is_initialized &= m_device.initialize(m_instance);
		is_initialized &= m_surface.initialize(m_instance, m_device, handle);

		is_initialized &= m_swapchain.initialize(m_device, m_surface, vsync);
		is_initialized &= m_command_buffer.initialize(m_device, m_swapchain);
		is_initialized &= m_images.initialize(m_device, m_swapchain);

		is_initialized &= image_available.initialize(m_device);
		is_initialized &= rendering_finished.initialize(m_device);
		is_initialized &= in_flight.initialize(m_device);

		record_commands();

		return is_initialized;
	}

	void vulkan::deinitialize()
	{
		if (not is_initialized)
			return;

		vkDeviceWaitIdle(m_device);


		in_flight.deinitialize(m_device);
		rendering_finished.deinitialize(m_device);
		image_available.deinitialize(m_device);

		m_images.deinitialize(m_device);
		m_command_buffer.deinitialize(m_device);
		m_swapchain.deinitialize(m_device);

		m_device.deinitialize();
		m_surface.deinitialize(m_instance);

#ifdef _DEBUG
		m_debug.deinitialize(m_instance);
#endif
		m_instance.deinitialize();
		is_initialized = false;
	}

	void vulkan::resize()
	{
		m_swapchain.resize(m_device, m_surface, m_vsync);

		resize_images();

		record_commands();
	}

	void vulkan::resize_images()
	{
		m_images.deinitialize(m_device);
		m_images.initialize(m_device, m_swapchain);
	}

	void vulkan::record_commands()
	{
		m_device.wait();

		m_surface.update(m_device);

		m_command_buffer.deinitialize(m_device);

		if (not m_command_buffer.initialize(m_device, m_swapchain))
			return;


		// TODO: no reuse of command yet, record per frame
		// render pass, framebuffer
		// viewport, vkCmdDraw

		for (u32 i = 0; i < m_command_buffer.size(); ++i)
		{
			const VkCommandBuffer& command_buffer = m_command_buffer[i];
			assert::check(command_buffer != nullptr);


			// VkCommandBufferBeginInfo cmd_buffer_begin{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = 0};
			// result = vkBeginCommandBuffer(command_buffers[i], &cmd_buffer_begin);

			VkResult result = m_command_buffer.begin(i);
			if (result != VK_SUCCESS)
			{
				dbg::println("Command buffer begin failed");
				return;
			}

			VkImageMemoryBarrier image_barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};

			image_barrier.srcAccessMask = 0;
			image_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;


			image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

			image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			image_barrier.image            = m_images.image(i);
			image_barrier.subresourceRange = {
			  .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, //
			  .baseMipLevel   = 0,
			  .levelCount     = 1,
			  .baseArrayLayer = 0,
			  .layerCount     = 1};

			// image barrier begin: LAYOUT_UNDEFINED -> LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			//               before end: LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> LAYOUT_PRESENT_SRC_KHR
			// https://github.com/emeiri/ogldev/blob/master/Vulkan/VulkanCore/Source/wrapper.cpp#L181

			// image layout
			vkCmdPipelineBarrier(
			  command_buffer,
			  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // src
			  VK_PIPELINE_STAGE_TRANSFER_BIT,    // dst
			  0,
			  0,
			  nullptr,                           // memory barrier
			  0,
			  nullptr,                           // buffer memory barrier
			  1,
			  &image_barrier);                   // image memory barrier

			// #0080c4
			VkClearColorValue clear_color{0.0f, 0.5f, 0.75f, 1.0f};
			VkClearValue      clear_depth = {.depthStencil = {.depth = 1.0f, .stencil = 0}};


			const VkRenderingAttachmentInfo color_attachment_info{
			  .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			  .imageView   = m_images.imageview(i),
			  .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,

			  .resolveMode        = VK_RESOLVE_MODE_NONE,
			  .resolveImageView   = VK_NULL_HANDLE,
			  .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,

			  .loadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR,
			  .storeOp    = VK_ATTACHMENT_STORE_OP_STORE,
			  .clearValue = clear_color,
			};

			VkClearValue depth_value{.depthStencil = {.depth = 1.0f, .stencil = 0}};

			const VkRenderingAttachmentInfo depth_attachment_info{
			  .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			  .imageView          = VK_NULL_HANDLE,
			  .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			  .resolveMode        = VK_RESOLVE_MODE_NONE,
			  .resolveImageView   = VK_NULL_HANDLE,
			  .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			  .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
			  .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
			  .clearValue         = clear_depth,
			};

			const VkExtent2D      current_extent = m_surface.extent();
			VkRect2D              render_area{{0, 0}, {(uint32_t)current_extent.width, (uint32_t)current_extent.height}};
			const VkRenderingInfo render_info{
			  .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			  // TODO: update commands when resized
			  .renderArea           = render_area,
			  .layerCount           = 1,
			  .colorAttachmentCount = 1,
			  .pColorAttachments    = &color_attachment_info,
			  .pDepthAttachment     = &depth_attachment_info,
			  .pStencilAttachment   = nullptr,
			};

			vkCmdBeginRendering(command_buffer, &render_info);


			//
			const VkViewport viewport{
			  .x        = 0.0f, //
			  .y        = 0.0f,
			  .width    = (f32)current_extent.width,
			  .height   = (f32)current_extent.height,
			  .minDepth = 0.0f,
			  .maxDepth = 1.0f};

			vkCmdSetViewport(command_buffer, 0, 1, &viewport);

			VkRect2D scissor = render_area;
			vkCmdSetScissor(command_buffer, 0, 1, &scissor);


			// render pass

			vkCmdEndRendering(command_buffer);

			// image layout present

			image_barrier.srcAccessMask = 0;
			image_barrier.dstAccessMask = 0;

			image_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			vkCmdPipelineBarrier(
			  command_buffer,
			  VK_PIPELINE_STAGE_TRANSFER_BIT,       // src
			  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dst
			  0,
			  0,
			  nullptr,                              // memory barrier
			  0,
			  nullptr,                              // buffer memory barrier
			  1,
			  &image_barrier);                      // image memory barrier


			result = m_command_buffer.end(i);
			if (result != VK_SUCCESS)
			{
				dbg::println("cmd buffer failed");
			}
		}
	}

	bool vulkan::draw()
	{

		in_flight.wait(m_device);
		bool     resized{false};
		u32      image_index{0};
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, image_available, nullptr, &image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			resize();
			resized = true;
		}
		else if (result != VK_SUCCESS)
		{
			dbg::println("Acquire swapchain image failed: {}", string_VkResult(result));
			return false;
		}

		in_flight.reset(m_device);


		m_command_buffer.submit(m_device, image_available, rendering_finished, in_flight, image_index);


		result = m_device.present(rendering_finished, image_index, m_swapchain);
		if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR or resized)
		{
			resize();
			resized = false;
		}
		else if (result != VK_SUCCESS)
		{
			dbg::println("Vulkan present failed: {}", string_VkResult(result));
			return false;
		}

		return true;
	}


} // namespace deckard::vulkan
