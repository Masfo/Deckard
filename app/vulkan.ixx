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
export import :images;
export import :core;
export import :texture;
export import :shaders;
export import :pipeline;
export import :buffer;

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
import deckard.vec;
import deckard.random;

namespace fs = std::filesystem;

namespace deckard::vulkan
{
	// Vulkan 1.3: https://developer.nvidia.com/blog/advanced-api-performance-vulkan-clearing-and-presenting/

	struct triangle2_vertex
	{
		math::vec2 pos;
		math::vec3 color;
	};

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

		bool is_vsync() const { return m_vsync; }

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
		graphics_pipeline    m_pipeline;

		graphics_pipeline m_pipeline2;
		vertex_buffer     m_triangle2_buffer;

		std::array<math::vec2, 3> m_triangle2_origin;
		std::array<math::vec3, 3> m_triangle2_color;
		std::array<f32, 3>        m_triangle2_angle;
		std::array<f32, 3>        m_triangle2_angular_speed;

		std::chrono::steady_clock::time_point m_last_frame_time{};

		void update_triangle2(f32 dt);


		// one per frame-in-flight slot, indexed by current_frame
		std::vector<semaphore> image_available;

		// one per swapchain image, avoids signaling a semaphore still in use by the swapchain
		std::vector<semaphore> rendering_finished;

		// single semaphore; each frame slot just remembers which counter value to wait for
		timeline_semaphore in_flight_timeline;

		std::vector<u64>   in_flight_values;    // per frame slot: last value submitted for that slot
		u64                timeline_counter{0}; // next value to signal on submit
		u32                current_frame{0};    // rotates over frame slots [0, frame count)


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

		is_initialized &= in_flight_timeline.initialize(m_device, 0);

		image_available.resize(m_swapchain.count(m_device));
		for (auto& image : image_available)
			is_initialized &= image.initialize(m_device);



		in_flight_values.assign(m_swapchain.count(m_device), 0);
		timeline_counter = 0;
		current_frame    = 0;

		rendering_finished.resize(m_swapchain.count(m_device));
		for (auto& sem : rendering_finished)
			is_initialized &= sem.initialize(m_device);



		// Compile the hardcoded triangle shaders at runtime and build the pipeline
		const fs::path shader_dir = fs::current_path() / "shaders";
		shader         vert;
		shader         frag;

		if (auto spirv = compile_spirv13(shader_dir / "triangle.vert"); not spirv.empty())
		{
			if (auto result = vert.load_from_memory(m_device, spirv); not result)
				dbg::println(result.error());
		}

		if (auto spirv = compile_spirv13(shader_dir / "triangle.frag"); not spirv.empty())
		{
			if (auto result = frag.load_from_memory(m_device, spirv); not result)
				dbg::println(result.error());
		}

		is_initialized &= m_pipeline.initialize(m_device, vert, frag, m_swapchain.desired_format().format);

		vert.deinitialize(m_device);
		frag.deinitialize(m_device);

		// Second triangle: vertices pushed as init data (not hardcoded in the vertex shader),
		// each corner then orbits its original position at a random speed/direction.
		{
			shader vert2;
			shader frag2;

			if (auto spirv = compile_spirv13(shader_dir / "triangle2.vert"); not spirv.empty())
			{
				if (auto result = vert2.load_from_memory(m_device, spirv); not result)
					dbg::println(result.error());
			}

			if (auto spirv = compile_spirv13(shader_dir / "triangle.frag"); not spirv.empty())
			{
				if (auto result = frag2.load_from_memory(m_device, spirv); not result)
					dbg::println(result.error());
			}

			const std::array<VkVertexInputBindingDescription, 1> bindings{{
			  {.binding = 0, .stride = sizeof(triangle2_vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
			}};

			const std::array<VkVertexInputAttributeDescription, 2> attributes{{
			  {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0},
			  {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(math::vec2)},
			}};

			is_initialized &=
			  m_pipeline2.initialize(m_device, vert2, frag2, m_swapchain.desired_format().format, bindings, attributes);

			vert2.deinitialize(m_device);
			frag2.deinitialize(m_device);

			m_triangle2_origin = {
			  math::vec2{0.55f, -0.75f},
			  math::vec2{0.80f, -0.25f},
			  math::vec2{0.30f, -0.25f},
			};

			m_triangle2_color = {
			  // math::vec3{1.0f, 1.0f, 0.0f}, // yellow
			  // math::vec3{1.0f, 0.0f, 1.0f}, // magenta
			  // math::vec3{0.0f, 1.0f, 1.0f}, // cyan

			  math::vec3{0.0f, 1.0f, 0.0f},
			  math::vec3{0.0f, 0.0f, 1.0f},
			  math::vec3{1.0f, 0.0f, 1.0f},
			};

			for (u32 i = 0; i < 3; ++i)
			{
				m_triangle2_angle[i] = random::rnd<f32>(0.0f, std::numbers::pi_v<f32> * 2.0f);

				const f32 speed              = random::rnd<f32>(1.0f, 3.0f);
				m_triangle2_angular_speed[i] = random::randbool() ? speed : -speed;
			}

			std::array<triangle2_vertex, 3> initial_vertices{};
			for (u32 i = 0; i < 3; ++i)
				initial_vertices[i] = {m_triangle2_origin[i], m_triangle2_color[i]};

			is_initialized &= m_triangle2_buffer.initialize(m_device, std::as_bytes(std::span(initial_vertices)));
		}

		m_last_frame_time = std::chrono::steady_clock::now();

		record_commands();

		return is_initialized;
	}

	void vulkan::deinitialize()
	{
		if (not is_initialized)
			return;

		vkDeviceWaitIdle(m_device);


		in_flight_timeline.deinitialize(m_device);

		for (auto& sem : rendering_finished)
			sem.deinitialize(m_device);
		rendering_finished.clear();


		for (auto& image : image_available)
			image.deinitialize(m_device);
		image_available.clear();

		m_pipeline.deinitialize(m_device);
		m_pipeline2.deinitialize(m_device);
		m_triangle2_buffer.deinitialize(m_device);
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

		for (auto& sem : rendering_finished)
			sem.deinitialize(m_device);

		for (auto& image : image_available)
			image.deinitialize(m_device);


		const u32 frame_count = m_swapchain.count(m_device);

		rendering_finished.resize(frame_count);
		for (auto& sem : rendering_finished)
			sem.initialize(m_device);

		image_available.resize(frame_count);
		for (auto& image : image_available)
			image.initialize(m_device);

		in_flight_values.assign(frame_count, 0);
		current_frame = 0;
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


		// vkCmdExecuteCommands
		// record static commands, then add them to the command buffer with vkCmdExecuteCommands

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
			  .srcAccessMask    = 0,
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
			  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
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

			if (m_pipeline.valid())
			{
				vkCmdBindPipeline(m_command_buffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
				vkCmdDraw(m_command_buffer[i], 3, 1, 0, 0);
			}

			if (m_pipeline2.valid())
			{
				vkCmdBindPipeline(m_command_buffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline2);

				const VkBuffer     vertex_buffers[]{m_triangle2_buffer};
				const VkDeviceSize offsets[]{0};
				vkCmdBindVertexBuffers(m_command_buffer[i], 0, 1, vertex_buffers, offsets);

				vkCmdDraw(m_command_buffer[i], 3, 1, 0, 0);
			}

			vkCmdEndRenderingKHR(m_command_buffer[i]);


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

	void vulkan::update_triangle2(f32 dt)
	{
		constexpr f32 orbit_radius = 0.06f;

		std::array<triangle2_vertex, 3> vertices{};
		for (u32 i = 0; i < 3; ++i)
		{
			m_triangle2_angle[i] += m_triangle2_angular_speed[i] * dt;

			const math::vec2 offset = math::vec2{orbit_radius, 0.0f}.rotate(m_triangle2_angle[i], math::vec2::zero());

			vertices[i].pos   = m_triangle2_origin[i] + offset;
			vertices[i].color = m_triangle2_color[i];
		}

		m_triangle2_buffer.update(m_device, std::as_bytes(std::span(vertices)));
	}

	bool vulkan::draw()
	{

		in_flight_timeline.wait(m_device, in_flight_values[current_frame]);

		const auto now    = std::chrono::steady_clock::now();
		const f32  dt     = std::chrono::duration<f32>(now - m_last_frame_time).count();
		m_last_frame_time = now;
		update_triangle2(dt);

		bool resized{false};
		u32  image_index{0};

		// image_available multiple?
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, image_available[current_frame], nullptr, &image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			resize();
			resized = true;
		}
		else if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR)
		{
			dbg::println("Acquire swapchain image failed: {}", string_VkResult(result));
			return false;
		}


		++timeline_counter;
		in_flight_values[current_frame] = timeline_counter;

		m_command_buffer.submit(
		  m_device,
		  image_available[current_frame],
		  rendering_finished[image_index],
		  in_flight_timeline,
		  timeline_counter,
		  image_index);


		result = m_device.present(rendering_finished[image_index], image_index, m_swapchain);
		if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR or resized)
		{
			resize();
		}
		else if (result != VK_SUCCESS)
		{
			dbg::println("Vulkan present failed: {}", string_VkResult(result));
			return false;
		}

		current_frame = (current_frame + 1) % as<u32>(image_available.size());

		return true;
	}


} // namespace deckard::vulkan
