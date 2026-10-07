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
import deckard.types;
import deckard.vec;
import deckard.random;
import deckard.debug;
import deckard.assert;

#ifndef _DEBUG
import deckard_build;
#endif

namespace fs  = std::filesystem;
namespace vec = deckard::vec;

namespace deckard::vulkan
{
	static constexpr u32 frames_in_flight = 3;

	inline constexpr std::array<f32, 3> default_clear_rgb{0.0f, 0.5f, 0.75f};

	static constexpr VkImageSubresourceRange color_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	export struct buffer
	{
		VkBuffer       handle{};
		VkDeviceMemory memory{};
		VkDeviceSize   size{};
		void*          mapped{}; // non-null only for HOST_VISIBLE memory
	};

	struct frame_slot
	{
		VkCommandBuffer cmd{};
		VkSemaphore     image_available{};
		buffer          vertex{}; // animated triangle2 vertices
		buffer          batch{};  // 2D batch vertices
		u32             batch_count{0};
		u64             timeline_value{0};
	};

	export struct vertex2d
	{
		vec::vec2 v;
		u32       rgba; // r | g<<8 | b<<16 | a<<24 -> bytes R,G,B,A in memory (little-endian)
	};

	static_assert(sizeof(vec::vec2) == 2 * sizeof(f32));
	static_assert(std::is_standard_layout_v<vertex2d>);           // offsetof needs this

	export inline constexpr u32 max_batch_vertices = 3u * 32768u; // multiple of 3, ~1.1 MiB per frame slot

	struct pipeline_desc
	{
		VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP}; // old defaults, so the
		VkCullModeFlags     cull_mode{VK_CULL_MODE_BACK_BIT};               // triangle pipelines are unchanged
		VkShaderStageFlags  push_stages{0};
		u32                 push_size{0};
	};

	struct pipeline_state
	{
		VkPipelineLayout layout{nullptr};
		VkPipeline       pipeline{nullptr};
		VkPipelineCache  cache{nullptr};
		fs::path         cache_file{};
	};

	struct triangle2_vertex
	{
		vec::vec2 pos;
		vec::vec3 color;
	};

	// Debug utils
	PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT{nullptr};
	PFN_vkSubmitDebugUtilsMessageEXT    vkSubmitDebugUtilsMessageEXT{nullptr};
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};

	// ################################################################
	// Helpers ########################################################
	template<typename T>
	concept VulkanStruct = requires(T t) {
		t.sType;
		t.pNext;
	};

	template<VulkanStruct... Ts>
	void vulkan_pnext_chain(Ts&... structs) noexcept
	{
		VkBaseOutStructure* previous{nullptr};
		const auto          link = [&](auto& s)
		{
			auto* current = reinterpret_cast<VkBaseOutStructure*>(&s);
			if (previous)
				previous->pNext = current;

			while (current->pNext)
				current = current->pNext;

			previous = current;
		};
		(link(structs), ...);
	}

	// Vulkan's two-call enumeration idiom. `fn(u32* count, T* items)` may return VkResult or void.
	// Retries on VK_INCOMPLETE.
	template<typename T, typename Fn>
	[[nodiscard]] std::expected<std::vector<T>, VkResult> enumerate(Fn&& fn)
	{
		const auto call = [&](u32* count, T* items) -> VkResult
		{
			if constexpr (std::is_void_v<std::invoke_result_t<Fn&, u32*, T*>>)
			{
				fn(count, items);
				return VK_SUCCESS;
			}
			else
				return fn(count, items);
		};

		std::vector<T> items;
		u32            count{0};
		VkResult       result{VK_SUCCESS};

		do
		{
			if (result = call(&count, nullptr); result != VK_SUCCESS)
				return std::unexpected(result);

			items.resize(count);
			result = call(&count, items.data());
		} while (result == VK_INCOMPLETE);

		if (result != VK_SUCCESS)
			return std::unexpected(result);

		items.resize(count);
		return items;
	}

	constexpr auto extension_name = [](const VkExtensionProperties& e) -> const char* { return e.extensionName; };
	constexpr auto layer_name     = [](const VkLayerProperties& l) -> const char* { return l.layerName; };

	[[nodiscard]] bool has_name(const auto& available, std::string_view wanted, auto name_of)
	{
		return std::ranges::any_of(available, [&](const auto& item) { return std::string_view{name_of(item)} == wanted; });
	}

	void transition_image(
	  VkCommandBuffer cmd, VkImage image, VkPipelineStageFlags2 src_stage, VkAccessFlags2 src_access,
	  VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access, VkImageLayout old_layout,
	  VkImageLayout new_layout) noexcept
	{
		const VkImageMemoryBarrier2 barrier{
		  .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		  .srcStageMask        = src_stage,
		  .srcAccessMask       = src_access,
		  .dstStageMask        = dst_stage,
		  .dstAccessMask       = dst_access,
		  .oldLayout           = old_layout,
		  .newLayout           = new_layout,
		  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		  .image               = image,
		  .subresourceRange    = color_range,
		};
		const VkDependencyInfo dependency{
		  .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		  .imageMemoryBarrierCount = 1,
		  .pImageMemoryBarriers    = &barrier,
		};
		vkCmdPipelineBarrier2(cmd, &dependency);
	}
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

	export class context
	{
	private:
		VkInstance               m_instance{nullptr};
		VkDevice                 m_device{nullptr};
		VkPhysicalDevice         m_physical_device{nullptr};
		VkQueue                  m_graphics_queue{nullptr};
		u32                      m_graphics_queue_family_index{0};
		VkSurfaceKHR             m_surface{nullptr};
		VkSurfaceCapabilitiesKHR m_surface_capabilities{};
		VkSwapchainKHR           m_swapchain{nullptr};
		VkSurfaceFormatKHR       m_surface_format{};
		VkExtent2D               m_swapchain_extent{};
		u32                      swapchain_image_count{0};


		u32 minimum_api_version{VK_API_VERSION_1_3};

		std::array<frame_slot, frames_in_flight> m_frames;

		bool m_needs_resize{false};

namespace fs = std::filesystem;

namespace deckard::vulkan
{
	// Vulkan 1.3: https://developer.nvidia.com/blog/advanced-api-performance-vulkan-clearing-and-presenting/

	//
	// VertexBuffer vb;
	// vb->vertex(x,y,z, color, uv);
	//

	private:
	public:
		[[nodiscard]] VkInstance instance() const noexcept { return m_instance; }

	struct triangle2_vertex
		[[nodiscard]] static constexpr bool version_at_least(u32 version, u32 major, u32 minor) noexcept
	{
			return VK_API_VERSION_MAJOR(version) > major
				   or (VK_API_VERSION_MAJOR(version) == major and VK_API_VERSION_MINOR(version) >= minor);
		}

		using debug_callback_t = void(std::string_view, std::string_view, std::string_view);

		void set_debug_callback(std::move_only_function<debug_callback_t> callback) { debug_callback = std::move(callback); }

		std::expected<void, std::string> initialize(HWND hWnd, u32 apiversion = VK_API_VERSION_1_3)
		{
			if (not version_at_least(apiversion, 1, 3))
				return std::unexpected(std::format(
				  "Vulkan 1.3 or newer is required, requested {}.{}",
				  VK_API_VERSION_MAJOR(apiversion),
				  VK_API_VERSION_MINOR(apiversion)));

			u32 instance_version{0};
			if (vkEnumerateInstanceVersion(&instance_version) != VK_SUCCESS)
				return std::unexpected("Failed to enumerate Vulkan instance version");

			if (not version_at_least(instance_version, 1, 3))
				return std::unexpected(std::format(
				  "Vulkan loader/runtime reports {}.{}.{}, 1.3 is required",
				  VK_API_VERSION_MAJOR(instance_version),
				  VK_API_VERSION_MINOR(instance_version),
				  VK_API_VERSION_PATCH(instance_version)));

			dbg::println("Vulkan instance version: {}.{}.{}",
						 VK_API_VERSION_MAJOR(instance_version),
						 VK_API_VERSION_MINOR(instance_version),
						 VK_API_VERSION_PATCH(instance_version));


			minimum_api_version = apiversion;
	};

	export class vulkan
		void deinitialize()
	{
	public:
		vulkan() = default;
		}

		vulkan(HWND handle, bool vsync, u32 apiversion) { initialize(handle, vsync, apiversion); }

		~vulkan() { deinitialize(); };

		// Copy
		vulkan(const vulkan&)            = delete;
		vulkan& operator=(const vulkan&) = delete;

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
		// ############################################################
		// Instance ###################################################

		std::expected<void, std::string> initialize_instance(u32 minimum_apiversion)
		{
			auto extensions = enumerate<VkExtensionProperties>(
			  [](u32* count, VkExtensionProperties* items)
			  { return vkEnumerateInstanceExtensionProperties(nullptr, count, items); });
			if (not extensions)
				return std::unexpected("Failed to enumerate instance extensions");
			m_available_extensions = std::move(*extensions);

			auto layers = enumerate<VkLayerProperties>(
			  [](u32* count, VkLayerProperties* items) { return vkEnumerateInstanceLayerProperties(count, items); });
			if (not layers)
				return std::unexpected("Failed to enumerate validator layers");
			m_available_layers = std::move(*layers);

			const VkApplicationInfo app_info{
			  .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			  .pApplicationName   = "Deckard",
			  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			  .pEngineName        = "Deckard",
#ifndef _DEBUG
			  .engineVersion = VK_MAKE_VERSION(
				deckard_build::build::major, deckard_build::build::minor, deckard_build::build::patch),
#endif
			  .apiVersion = minimum_apiversion,
			};

			const std::array<const char*, 5> required_extensions{
			  VK_KHR_SURFACE_EXTENSION_NAME,
			  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
			  // VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
			  // VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
			  VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			  VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

			std::vector<const char*> required_layers;
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

		std::array<vec::vec2, 3> m_triangle2_origin;
		std::array<vec::vec3, 3> m_triangle2_color;
		std::array<f32, 3>       m_triangle2_angle;
		std::array<f32, 3>       m_triangle2_angular_speed;

		std::chrono::steady_clock::time_point m_last_frame_time{};

		void update_triangle2(f32 dt);


		// one per frame-in-flight slot, indexed by current_frame
		std::vector<semaphore> image_available;

		// one per swapchain image, avoids signaling a semaphore still in use by the swapchain
		std::vector<semaphore> rendering_finished;

		// single semaphore; each frame slot just remembers which counter value to wait for
		timeline_semaphore in_flight_timeline;

		std::vector<u64> in_flight_values;    // per frame slot: last value submitted for that slot
		u64              timeline_counter{0}; // next value to signal on submit
		u32              current_frame{0};    // rotates over frame slots [0, frame count)


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
			  {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = sizeof(vec2)},
			}};

			is_initialized &= m_pipeline2.initialize(
			  m_device, vert2, frag2, m_swapchain.desired_format().format, bindings, attributes);

			vert2.deinitialize(m_device);
			frag2.deinitialize(m_device);

			m_triangle2_origin = {
			  vec2{0.55f, -0.75f},
			  vec2{0.80f, -0.25f},
			  vec2{0.30f, -0.25f},
			};

			m_triangle2_color = {
			  // math::vec3{1.0f, 1.0f, 0.0f}, // yellow
			  // math::vec3{1.0f, 0.0f, 1.0f}, // magenta
			  // math::vec3{0.0f, 1.0f, 1.0f}, // cyan

			  vec3{0.0f, 1.0f, 0.0f},
			  vec3{0.0f, 0.0f, 1.0f},
			  vec3{1.0f, 0.0f, 1.0f},
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

			VkImageMemoryBarrier2 top_image_memory_barrier{
			  .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			  .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			  .srcAccessMask       = VK_ACCESS_2_NONE,
			  .dstStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			  .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			  .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
			  .newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

			const VkDependencyInfo top_dependency_info{
			  .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			  .imageMemoryBarrierCount = 1,
			  .pImageMemoryBarriers    = &top_image_memory_barrier,
			};

			vkCmdPipelineBarrier2(m_command_buffer[i], &top_dependency_info);


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

			VkImageMemoryBarrier2 bottom_image_memory_barrier{
			  .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			  .srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			  .srcAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			  .dstStageMask        = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
			  .dstAccessMask       = VK_ACCESS_2_NONE,
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

			const VkDependencyInfo bottom_dependency_info{
			  .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			  .imageMemoryBarrierCount = 1,
			  .pImageMemoryBarriers    = &bottom_image_memory_barrier,
			};

			vkCmdPipelineBarrier2(m_command_buffer[i], &bottom_dependency_info);

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

			const vec2 offset = vec2{orbit_radius, 0.0f}.rotate(m_triangle2_angle[i], vec2::zero());

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
		VkResult result = vkAcquireNextImageKHR(
		  m_device, m_swapchain, UINT64_MAX, image_available[current_frame], nullptr, &image_index);
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


	};
} // namespace deckard::vulkan
