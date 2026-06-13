module;
#include <windows.h>

#include <vulkan/vk_enum_string_helper.h>
#define VK_ONLY_EXPORTED_PROTOTYPES

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
export import :shaders;

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

		vulkan(HWND handle, bool vsync, u32 apiversion) { initialize(handle, vsync, apiversion); }

		~vulkan() { deinitialize(); };

		// Copy
		vulkan(vulkan const&)            = delete;
		vulkan& operator=(vulkan const&) = delete;

		// Move
		vulkan(vulkan&&)            = delete;
		vulkan& operator=(vulkan&&) = delete;


		bool initialize(HWND handle, bool vsync, u32 apiversion);
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

	bool vulkan::initialize(HWND handle, bool vsync, u32 apiversion)
	{
		dbg::println(
		  "Compiled against Vulkan Header Version: {}.{}.{}.{}",
		  VK_API_VERSION_VARIANT(VK_HEADER_VERSION_COMPLETE),
		  VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE),
		  VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE),
		  VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));
		m_vsync = vsync;

		is_initialized = m_instance.initialize(apiversion);
#ifdef _DEBUG
		is_initialized &= m_debug.initialize(m_instance, nullptr);
#endif

		is_initialized &= m_device.initialize(m_instance, apiversion);

		if (m_device == nullptr)
		{
			dbg::println("Failed to create Vulkan device");
			return false;
		}
		is_initialized &= m_surface.initialize(m_instance, m_device, handle);

		is_initialized &= m_swapchain.initialize(m_device, m_surface, vsync);
		is_initialized &= m_command_buffer.initialize(m_device, m_swapchain);
		is_initialized &= m_images.initialize(m_device, m_swapchain);

		is_initialized &= image_available.initialize(m_device);
		is_initialized &= rendering_finished.initialize(m_device);
		is_initialized &= in_flight.initialize(m_device);

		record_commands();

		// Test shader
		shader vert;
		shader frag;

		if (auto result = vert.load(m_device, "data01/vert.spv"); not result)
			dbg::println(result.error());

		if (auto result = frag.load(m_device, "data01/frag.spv"); not result)
			dbg::println(result.error());

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


			// VkCommandBufferBeginInfo cmd_buffer_begin{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = 0};
			// result = vkBeginCommandBuffer(command_buffers[i], &cmd_buffer_begin);

			VkResult result = m_command_buffer.begin(i);
			if (result != VK_SUCCESS)
			{
				dbg::println("Command buffer begin failed");
				return;
			}

			assert::check(m_command_buffer[i] != nullptr);

			// #0080c4
			VkClearColorValue clear_color{0.0f, 0.5f, 0.75f, 1.0f};


			const VkRenderingAttachmentInfo color_attachment_info{
			  .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			  .imageView   = m_images.imageview(i),
			  .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,

			  .loadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR,
			  .storeOp    = VK_ATTACHMENT_STORE_OP_STORE,
			  .clearValue = clear_color,
			};


			const VkExtent2D      current_extent = m_surface.extent();
			VkRect2D              render_area{{0, 0}, {current_extent.width, current_extent.height}};
			const VkRenderingInfo render_info{
			  .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			  // TODO: update commands when resized
			  .renderArea           = render_area,
			  .layerCount           = 1,
			  .colorAttachmentCount = 1,
			  .pColorAttachments    = &color_attachment_info,
			};

			VkImageMemoryBarrier top_image_memory_barrier{
			  .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			  .dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			  .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
			  .newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			  .image            = m_images.image(i),
			  .subresourceRange = {
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1,
			  }};

			vkCmdPipelineBarrier(
			  m_command_buffer[i],
			  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // srcStageMask
			  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
			  0,
			  0,
			  nullptr,
			  0,
			  nullptr,
			  1,                        // imageMemoryBarrierCount
			  &top_image_memory_barrier // pImageMemoryBarriers
			);


			vkCmdBeginRenderingKHR(m_command_buffer[i], &render_info);

			// draw here

			vkCmdEndRenderingKHR(m_command_buffer[i]);


#if 0
			const VkViewport viewport{
			  .x        = 0.0f, //
			  .y        = 0.0f,
			  .width    = (f32)current_extent.width,
			  .height   = (f32)current_extent.height,
			  .minDepth = 0.0f,
			  .maxDepth = 1.0f};

			vkCmdSetViewport(m_command_buffer[i], 0, 1, &viewport);

			VkRect2D scissor = render_area;
			vkCmdSetScissor(m_command_buffer[i], 0, 1, &scissor);
#endif


			//


#if 1
			// image barrier begin: LAYOUT_UNDEFINED -> LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			//               before end: LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> LAYOUT_PRESENT_SRC_KHR
			// https://github.com/emeiri/ogldev/blob/master/Vulkan/VulkanCore/Source/wrapper.cpp#L181

			VkImageMemoryBarrier bottom_image_memory_barrier{
			  .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			  .srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			  .oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			  .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			  .image               = m_images.image(i),
			  .subresourceRange    = {
				   .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				   .baseMipLevel   = 0,
				   .levelCount     = 1,
				   .baseArrayLayer = 0,
				   .layerCount     = 1,
              }};

			vkCmdPipelineBarrier(
			  m_command_buffer[i],
			  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
			  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // dstStageMask
			  0,
			  0,
			  nullptr,
			  0,
			  nullptr,
			  1,                           // imageMemoryBarrierCount
			  &bottom_image_memory_barrier // pImageMemoryBarriers
			);

#endif

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
		bool resized{false};
		u32  image_index{0};

		// image_available multiple?
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
