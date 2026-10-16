module;
#include <windows.h>

#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_win32.h>

export module deckard.vulkan;

export module deckard.vulkan;
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
import deckard.scope_exit;
import deckard.image;

#ifndef _DEBUG
import deckard_build;
#endif

namespace fs  = std::filesystem;
namespace vec = deckard::vec;

namespace deckard::vulkan
{
	static constexpr u32 frames_in_flight = 3;

	inline constexpr std::array<f32, 3> default_clear_rgb{0.0f, 0.5f, 0.75f};

	static constexpr VkImageSubresourceRange  color_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	static constexpr std::array<vec::vec2, 3> triangle2_uvs{
	  vec::vec2{0.5f, 0.0f}, vec::vec2{1.0f, 1.0f}, vec::vec2{0.0f, 1.0f}};

	export struct buffer
	{
		VkBuffer       handle{};
		VkDeviceMemory memory{};
		VkDeviceSize   size{};
		VkMemoryPropertyFlags memory_properties{};
		void*          mapped{}; // non-null only for HOST_VISIBLE memory
	};

	struct image_resource
	{
		VkImage        handle{};
		VkDeviceMemory memory{};
		VkImageView    view{};
		VkSampler      sampler{};
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
		VkPrimitiveTopology   topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP}; // old defaults, so the
		VkCullModeFlags       cull_mode{VK_CULL_MODE_BACK_BIT};               // triangle pipelines are unchanged
		VkShaderStageFlags    push_stages{0};
		u32                   push_size{0};
		VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
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
		vec::vec2          pos;
		vec::vec2          uv;
		std::array<f32, 4> color;
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
		bool                     m_unified_image_layouts{false};
		buffer*                  m_screenshot_buffer{nullptr};
		buffer                   m_screenshot_readback{};
		VkQueue                  m_graphics_queue{nullptr};
		u32                      m_graphics_queue_family_index{0};
		VkSurfaceKHR             m_surface{nullptr};
		VkSurfaceCapabilitiesKHR m_surface_capabilities{};
		VkSwapchainKHR           m_swapchain{nullptr};
		VkSurfaceFormatKHR       m_surface_format{};
		VkExtent2D               m_swapchain_extent{};
		bool                     m_swapchain_transfer_src{false};
		u32                      swapchain_image_count{0};


		u32 minimum_api_version{VK_API_VERSION_1_3};

		std::array<frame_slot, frames_in_flight> m_frames;

		bool m_needs_resize{false};

		// command buffer
		VkCommandPool                m_command_pool{nullptr};
		std::vector<VkCommandBuffer> m_command_buffers;
		u32                          m_command_buffer_index{0};

		// synchronization
		std::vector<VkSemaphore> m_image_available{};    // one per swapchain image
		std::vector<VkSemaphore> m_rendering_finished{}; // one per swapchain image

		VkSemaphore      m_in_flight_timeline{nullptr};
		std::vector<u64> m_in_flight_timeline_values;

		// images
		std::vector<VkImage>     m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		image_resource           m_uploaded_image{};
		u64                      m_signal_value{0};     // timeline semaphore signal value for the current frame in flight
		u64                      m_timeline_counter{0}; // timeline semaphore counter for the current frame in flight
		u32                      m_currentframe{0};     // frame index for the current frame in flight

		// pipelines: [0] = triangle, [1] = triangle2
		std::vector<pipeline_state> m_pipelines;
		pipeline_state              m_pipeline_2d{};
		VkDescriptorSetLayout       m_texture_descriptor_layout{VK_NULL_HANDLE};
		VkDescriptorPool            m_texture_descriptor_pool{VK_NULL_HANDLE};
		VkDescriptorSet             m_texture_descriptor_set{VK_NULL_HANDLE};
		std::array<f32, 3>          m_clear_rgb{default_clear_rgb};

		// triangle2
		buffer                                m_triangle2{}; // static copy, only used by record_clear_commands()
		std::array<triangle2_vertex, 3>       m_triangle2_initial_vertices{};
		std::array<vec::vec2, 3>              m_triangle2_origin{};
		std::array<std::array<f32, 4>, 3>     m_triangle2_color{};
		std::array<f32, 3>                    m_triangle2_angle{};
		std::array<f32, 3>                    m_triangle2_angular_speed{};
		f32                                   m_time_seconds{0.0f};
		std::chrono::steady_clock::time_point m_last_frame_time{};
		bool                                  vsync{true};


		std::vector<VkLayerProperties>     m_available_layers;
		std::vector<VkExtensionProperties> m_available_extensions;
		std::vector<VkExtensionProperties> m_device_extensions;

		using debug_callback_t = void(std::string_view, std::string_view, std::string_view);

		VkDebugUtilsMessengerEXT                  debug_messenger{nullptr};
		std::move_only_function<debug_callback_t> debug_callback;

		// Device-level handles with a (device, handle, allocator) destroy function: destroy and reset.
		template<typename Handle, typename Fn>
		void destroy(Handle& handle, Fn destroy_fn) const noexcept
{
			if (handle and m_device)
			{
				destroy_fn(m_device, handle, nullptr);
				handle = {};
			}
		}

		template<typename Pfn>
		[[nodiscard]] Pfn load_instance_proc(const char* name) const noexcept
		{
			return reinterpret_cast<Pfn>(vkGetInstanceProcAddr(m_instance, name));
		}

	private:
	public:
		[[nodiscard]] VkInstance instance() const noexcept { return m_instance; }

		[[nodiscard]] VkDevice device() const noexcept { return m_device; }

		[[nodiscard]] VkPhysicalDevice physical_device() const noexcept { return m_physical_device; }

		[[nodiscard]] VkQueue graphics_queue() const noexcept { return m_graphics_queue; }

		void set_vsync(bool enabled) noexcept { vsync = enabled; }

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

			auto result
			  = initialize_instance(minimum_api_version)
				  .and_then([this] { return initialize_debug(); })
				  .and_then([this, hWnd] { return initialize_surface(hWnd); })
				  .and_then([this] { return initialize_device(); })
				  .and_then([this] { return initialize_swapchain(); })
				  .and_then([this] { return initialize_swapchain_images(); })
				  .and_then([this] { return initialize_command_buffers(); })
				  .and_then([this] { return initialize_frames(); })
				  .and_then([this] { return initialize_synchronization(); })
				  .and_then([this] { return initialize_triangle_renderer(); })
				  .and_then([this] { return initialize_renderer2d(); })
				  .and_then([this] { return record_clear_commands(); });


			return result;
		}

		void deinitialize()
		{
			if (m_device != VK_NULL_HANDLE)
				vkDeviceWaitIdle(m_device);

			deinitialize_synchronization();
			deinitialize_renderer2d();
			deinitialize_frames();
			deinitialize_triangle_renderer();
			deinitialize_command_buffers();
			deinitialize_swapchain_images();
			deinitialize_uploaded_image();
			if (m_screenshot_readback.handle != VK_NULL_HANDLE)
				destroy_buffer(m_screenshot_readback);
			deinitialize_swapchain();
			deinitialize_device();
			deinitialize_surface();
			deinitialize_debug();
			deinitialize_instance();
		}

		[[nodiscard]] bool resize()
		{
			const auto caps = query_surface_capabilities();
			if (not caps or caps->currentExtent.width == 0 or caps->currentExtent.height == 0)
				return false;

			vkDeviceWaitIdle(m_device);

			if (auto result = resize_swapchain()
								.and_then([this] { return resize_swapchain_images(); })
								.and_then([this] { return resize_synchronization(); });
				not result)
			{
				dbg::println("resize failed: {}", result.error());
				return false;
			}


			for (auto& frame : m_frames)
			{
				destroy(frame.image_available, vkDestroySemaphore);
				frame.image_available = create_semaphore();
				if (frame.image_available == VK_NULL_HANDLE)
					return false;
				frame.timeline_value = 0;
			}

			m_timeline_counter = 0;
			m_currentframe     = 0;
			m_needs_resize     = false;


			return true;
		}

		[[nodiscard]] bool needs_resize() const noexcept { return m_needs_resize; }

		bool draw(std::span<const vertex2d> batch = {}, std::array<f32, 3> clear_rgb = default_clear_rgb)
		{

			if (m_swapchain_images.empty())
				return true;

			auto& frame = m_frames[m_currentframe];

			// One host wait covers command buffer, vertex buffer and image_available reuse.
			if (frame.timeline_value != 0)
				wait_for_timeline_semaphore(m_in_flight_timeline, frame.timeline_value);

			upload_batch(frame, batch);
			m_clear_rgb = clear_rgb;

			const auto now    = std::chrono::steady_clock::now();
			const f32  dt     = std::chrono::duration<f32>(now - m_last_frame_time).count();
			m_last_frame_time = now;
			m_time_seconds += dt;
			update_triangle2(frame, dt);

			u32      image_index{0};
			VkResult result = vkAcquireNextImageKHR(
			  m_device, m_swapchain, UINT64_MAX, frame.image_available, VK_NULL_HANDLE, &image_index);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				m_needs_resize = true;
				return true;
			}
			if (result == VK_SUBOPTIMAL_KHR)
				m_needs_resize = true;
			else if (result != VK_SUCCESS)
				return false;

			vkResetCommandBuffer(frame.cmd, 0);
			if (auto recorded = record_frame(frame, image_index); not recorded)
			{
				dbg::println("{}", recorded.error());
				m_needs_resize = true;
				return false;
			}

			const u64 signal_value = m_timeline_counter + 1;
			if (result = submit_frame(frame, image_index, signal_value); result != VK_SUCCESS)
			{
				dbg::println("Failed to submit frame: {}", string_VkResult(result));
				m_needs_resize = true;
				return false;
			}

			// only after a successful submit, so a failed submit can't leave a never-signaled value behind
			m_timeline_counter   = signal_value;
			frame.timeline_value = signal_value;

			const VkPresentInfoKHR present_info{
			  .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			  .waitSemaphoreCount = 1,
			  .pWaitSemaphores    = &m_rendering_finished[image_index], // per image
			  .swapchainCount     = 1,
			  .pSwapchains        = &m_swapchain,
			  .pImageIndices      = &image_index,
	};

			result = vkQueuePresentKHR(m_graphics_queue, &present_info);
			if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR)
				m_needs_resize = true;
			else if (result != VK_SUCCESS)
	{
				dbg::println("Failed to present: {}", string_VkResult(result));
				return false;
		}

			// Advance even if present was out of date: the slot's value is already recorded.
			m_currentframe = (m_currentframe + 1) % frames_in_flight;
			return true;
		}

		[[nodiscard]] std::expected<image_rgb, std::string> screenshot(
		  std::span<const vertex2d> batch = {}, std::array<f32, 3> clear_rgb = default_clear_rgb)
		{
			if (m_device == VK_NULL_HANDLE or m_swapchain_images.empty())
				return std::unexpected("Vulkan swapchain must be initialized before taking a screenshot");
			if (not m_swapchain_transfer_src)
				return std::unexpected("Swapchain images do not support transfer-source usage");
			if (m_screenshot_buffer != nullptr)
				return std::unexpected("A Vulkan screenshot is already in progress");

			const u32 width  = m_swapchain_extent.width;
			const u32 height = m_swapchain_extent.height;
			if (width == 0 or height == 0 or width > image_rgb::MAX_WIDTH or height > image_rgb::MAX_HEIGHT)
				return std::unexpected("Swapchain extent is not supported for an RGB screenshot");

			if (m_surface_format.format != VK_FORMAT_R8G8B8A8_UNORM and m_surface_format.format != VK_FORMAT_R8G8B8A8_SRGB
				and m_surface_format.format != VK_FORMAT_B8G8R8A8_UNORM
				and m_surface_format.format != VK_FORMAT_B8G8R8A8_SRGB)
				return std::unexpected(std::format(
				  "Swapchain format {} is not supported for RGB screenshots", string_VkFormat(m_surface_format.format)));

			const VkDeviceSize readback_size = static_cast<VkDeviceSize>(width) * height * 4;
			if (m_screenshot_readback.size < readback_size)
			{
				buffer readback{};
				if (auto result = create_buffer(
					  readback_size,
					  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					  readback,
					  "screenshot readback",
					  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
					  VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
					not result)
					return std::unexpected(result.error());

				if (m_screenshot_readback.handle != VK_NULL_HANDLE)
					destroy_buffer(m_screenshot_readback);
				m_screenshot_readback = std::exchange(readback, buffer{});
				dbg::println(
				  "Screenshot readback memory: cached={}, coherent={}",
				  (m_screenshot_readback.memory_properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0,
				  (m_screenshot_readback.memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0);
			}

			m_screenshot_buffer = &m_screenshot_readback;
			scope_exit reset_screenshot_buffer{[this] { m_screenshot_buffer = nullptr; }};

			const u32 frame_index             = m_currentframe;
			const u64 previous_timeline_value = m_timeline_counter;
			if (not draw(batch, clear_rgb))
			{
				vkDeviceWaitIdle(m_device);
				return std::unexpected("Failed to render frame for Vulkan screenshot");
			}
			if (m_timeline_counter == previous_timeline_value)
				return std::unexpected("Vulkan did not render a frame for the screenshot");
			const u64                 frame_value = m_frames[frame_index].timeline_value;
			const VkSemaphoreWaitInfo wait_info{
			  .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			  .semaphoreCount = 1,
			  .pSemaphores    = &m_in_flight_timeline,
			  .pValues        = &frame_value,
			};
			if (VkResult result = vkWaitSemaphores(m_device, &wait_info, UINT64_MAX); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to wait for screenshot readback: {}", string_VkResult(result)));
			if ((m_screenshot_readback.memory_properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				const VkMappedMemoryRange mapped_range{
				  .sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				  .memory = m_screenshot_readback.memory,
				  .size   = VK_WHOLE_SIZE,
				};
				if (VkResult result = vkInvalidateMappedMemoryRanges(m_device, 1, &mapped_range); result != VK_SUCCESS)
					return std::unexpected(
					  std::format("Failed to invalidate screenshot readback memory: {}", string_VkResult(result)));
			}

			image_rgb   image{width, height};
			const auto* pixels = static_cast<const u8*>(m_screenshot_readback.mapped);
			const bool  bgra   = m_surface_format.format == VK_FORMAT_B8G8R8A8_UNORM
								 or m_surface_format.format == VK_FORMAT_B8G8R8A8_SRGB;
			auto        output = image.data();
			for (std::size_t i = 0; i < output.size(); ++i)
			{
				const std::size_t offset = i * 4;
				output[i].r              = pixels[offset + (bgra ? 2 : 0)];
				output[i].g              = pixels[offset + 1];
				output[i].b              = pixels[offset + (bgra ? 0 : 2)];
			}

			return image;
		}

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
			required_layers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_crash_diagnostic"};
#endif
		instance             m_instance;
		device               m_device;
		presentation_surface m_surface;
		command_buffer       m_command_buffer;
		swapchain            m_swapchain;
		images               m_images;
		graphics_pipeline    m_pipeline;

			for (const char* extension : required_extensions)
				if (not has_name(m_available_extensions, extension, extension_name))
					return std::unexpected(std::format("Required extension not found: {}", extension));

			for (const char* layer : required_layers)
				if (not has_name(m_available_layers, layer, layer_name))
					return std::unexpected(std::format("Required layer not found: {}", layer));

			const VkInstanceCreateInfo instance_info{
			  .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			  .pApplicationInfo        = &app_info,
			  .enabledLayerCount       = as<u32>(required_layers.size()),
			  .ppEnabledLayerNames     = required_layers.data(),
			  .enabledExtensionCount   = as<u32>(required_extensions.size()),
			  .ppEnabledExtensionNames = required_extensions.data(),
			};

			switch (const VkResult result = vkCreateInstance(&instance_info, nullptr, &m_instance))
			{
				case VK_SUCCESS: return {};
				case VK_ERROR_INCOMPATIBLE_DRIVER: return std::unexpected("Vulkan driver is not compatible.");
				case VK_ERROR_EXTENSION_NOT_PRESENT: return std::unexpected("Vulkan extension not present.");
				case VK_ERROR_LAYER_NOT_PRESENT: return std::unexpected("Vulkan layer not present.");
				default:
					return std::unexpected(std::format("Failed to create Vulkan instance: {}", string_VkResult(result)));
			}
		}

		void deinitialize_instance()
		{
			if (m_instance)
			{
				vkDestroyInstance(m_instance, nullptr);
				m_instance = VK_NULL_HANDLE;
			}
		}

		// ############################################################
		// Debug ######################################################
		template<typename Handle>
		void set_debug_name([[maybe_unused]] VkDevice dev, Handle handle, VkObjectType type, std::string_view name) noexcept
		{
		#ifdef _DEBUG
			const VkDebugUtilsObjectNameInfoEXT info{
			  .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			  .objectType   = type,
			  .objectHandle = reinterpret_cast<std::uint64_t>(handle),
			  .pObjectName  = name.data(),
			};
			// Load once via vkGetInstanceProcAddr in real code
			static auto fn = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
			  vkGetDeviceProcAddr(dev, "vkSetDebugUtilsObjectNameEXT"));
			if (fn)
				fn(dev, &info);
			#endif
		}

		// one per swapchain image, avoids signaling a semaphore still in use by the swapchain
		std::vector<semaphore> rendering_finished;

		static VKAPI_ATTR VkBool32 VKAPI_CALL static_debug_callback(
		  [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
		  [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT             message_type,
		  [[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* callback_data, [[maybe_unused]] void* userdata)
		{
#ifdef _DEBUG
			auto* self = static_cast<context*>(userdata);

			const std::string_view severity = [&]() -> std::string_view
			{
				switch (message_severity)
				{
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "Verbose";
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "Info";
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "Warning";
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "Error";
					default: return "Unknown";
				}
			}();

			const std::string_view type = [&]() -> std::string_view
			{
				switch (message_type)
				{
					case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "General";
					case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "Validation";
					case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "Performance";
					case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: return "Device address binding";
					default: return "Unknown";
				}
			}();

			if (callback_data->pMessage and self->debug_callback)
				self->debug_callback(severity, type, std::string_view(callback_data->pMessage));
#endif
			return VK_FALSE;
		}

		std::expected<void, std::string> initialize_debug_functions()
	{
#ifdef _DEBUG
			assert::check(
			  m_instance != VK_NULL_HANDLE, "Vulkan instance must be created before initializing debug functions");

			vkCreateDebugUtilsMessengerEXT = load_instance_proc<PFN_vkCreateDebugUtilsMessengerEXT>(
			  "vkCreateDebugUtilsMessengerEXT");
			vkSubmitDebugUtilsMessageEXT = load_instance_proc<PFN_vkSubmitDebugUtilsMessageEXT>(
			  "vkSubmitDebugUtilsMessageEXT");
			vkDestroyDebugUtilsMessengerEXT = load_instance_proc<PFN_vkDestroyDebugUtilsMessengerEXT>(
			  "vkDestroyDebugUtilsMessengerEXT");

			if (not vkCreateDebugUtilsMessengerEXT or not vkSubmitDebugUtilsMessageEXT
				or not vkDestroyDebugUtilsMessengerEXT)
				return std::unexpected("Failed to load debug functions");
#endif
			return {};
		}

		std::expected<void, std::string> initialize_debug()
		{
#ifdef _DEBUG
			if (auto result = initialize_debug_functions(); not result)
				return result;

			constexpr VkDebugUtilsMessageSeverityFlagsEXT severity
			  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

			const VkDebugUtilsMessengerCreateInfoEXT create_info{
			  .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			  .messageSeverity = severity,
			  .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
								 | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			  .pfnUserCallback = static_debug_callback,
			  .pUserData       = this,
			};

			assert::check(
			  m_instance != VK_NULL_HANDLE, "Vulkan instance must be created before initializing debug messenger");

			if (VkResult result = vkCreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &debug_messenger);
				result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create debug messenger: {}", string_VkResult(result)));
#endif
			return {};
		}

		void deinitialize_debug()
		{
#ifdef _DEBUG
			if (m_instance == VK_NULL_HANDLE)
				return;

			if (debug_messenger != VK_NULL_HANDLE)
			{
				vkDestroyDebugUtilsMessengerEXT(m_instance, debug_messenger, nullptr);
				debug_messenger = VK_NULL_HANDLE;
			}
#endif
		}

		// ############################################################
		// Surface ####################################################

		std::expected<void, std::string> initialize_surface(HWND hwnd)
		{
			const VkWin32SurfaceCreateInfoKHR create_info{
			  .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			  .hinstance = GetModuleHandle(nullptr),
			  .hwnd      = hwnd,
			};

			if (VkResult result = vkCreateWin32SurfaceKHR(m_instance, &create_info, nullptr, &m_surface);
				result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create Win32 surface: {}", string_VkResult(result)));

			return {};
		}

		[[nodiscard]] std::expected<VkSurfaceCapabilitiesKHR, VkResult> query_surface_capabilities() const noexcept
		{
			VkSurfaceCapabilitiesKHR capabilities{};
			if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &capabilities);
				result != VK_SUCCESS)
				return std::unexpected(result);

			return capabilities;
		}

		std::expected<void, std::string> update_surface_capabilities()
		{
			if (m_surface == VK_NULL_HANDLE)
				return std::unexpected("Vulkan surface must be created before updating surface capabilities");

			if (m_physical_device == VK_NULL_HANDLE)
				return std::unexpected("Vulkan physical device must be created before updating surface capabilities");

			auto capabilities = query_surface_capabilities();
			if (not capabilities)
				return std::unexpected(
				  std::format("Failed to get surface capabilities: {}", string_VkResult(capabilities.error())));

			m_surface_capabilities = *capabilities;
			return {};
		}

		void deinitialize_surface()
		{
			if (m_surface != VK_NULL_HANDLE)
			{
				vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
				m_surface = VK_NULL_HANDLE;
			}
		}

		[[nodiscard]] bool can_present_win32(u32 queue_family_index) const
		{
			VkBool32 present_support = VK_FALSE;
			if (vkGetPhysicalDeviceSurfaceSupportKHR(m_physical_device, queue_family_index, m_surface, &present_support)
				!= VK_SUCCESS)
			return false;

			return present_support == VK_TRUE;
		}
		is_initialized &= m_surface.initialize(m_instance, m_device, handle);

		std::expected<u32, std::string> select_queue_family_index()
		{
			auto families = enumerate<VkQueueFamilyProperties>(
			  [this](u32* count, VkQueueFamilyProperties* items)
			  { vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, count, items); });
			if (not families or families->empty())
				return std::unexpected("Failed to get queue family properties");

			for (u32 i = 0; i < families->size(); ++i)
			{
				const auto& family = (*families)[i];
				if (family.queueCount != 0 and (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) and can_present_win32(i))
					return i;
			}

			return std::unexpected("Failed to find a suitable queue family");
		}

		// ############################################################
		// Device #####################################################

		// Prefers discrete, then integrated, then the first device. Devices missing the required features or
		// API version are skipped while looking for discrete/integrated.
		std::expected<void, std::string> select_physical_device()
		{
			auto devices = enumerate<VkPhysicalDevice>(
			  [this](u32* count, VkPhysicalDevice* items) { return vkEnumeratePhysicalDevices(m_instance, count, items); });
			if (not devices)
				return std::unexpected(
				  std::format("Failed to enumerate physical devices: {}", string_VkResult(devices.error())));

			// TODO: Implement scoring
			std::optional<VkPhysicalDevice> discrete;
			std::optional<VkPhysicalDevice> integrated;

			for (const VkPhysicalDevice device : *devices)
			{
				VkPhysicalDeviceProperties props{};
				vkGetPhysicalDeviceProperties(device, &props);

				VkPhysicalDeviceFeatures2        features2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
				VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
				VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
				VkPhysicalDeviceVulkan14Features features14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};

				vulkan_pnext_chain(features2, features12, features13, features14);

				vkGetPhysicalDeviceFeatures2(device, &features2);

				if (not features13.dynamicRendering or not features13.synchronization2 or not features12.timelineSemaphore)
		{
					dbg::println("Skipping device {}: need dynamic rendering, synchronization2 and timelineSemaphore",
								 props.deviceName);
					continue;
		}

				if (
				  not version_at_least(
					props.apiVersion, VK_API_VERSION_MAJOR(minimum_api_version), VK_API_VERSION_MINOR(minimum_api_version)))
		{
					dbg::println(
					  "Skipping device {}: {}.{}.{}, requires at least {}.{}",
					  props.deviceName,
					  VK_API_VERSION_MAJOR(props.apiVersion),
					  VK_API_VERSION_MINOR(props.apiVersion),
					  VK_API_VERSION_PATCH(props.apiVersion),
					  VK_API_VERSION_MAJOR(minimum_api_version),
					  VK_API_VERSION_MINOR(minimum_api_version));
					continue;
		}

				if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU and not discrete)
					discrete = device;
				else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU and not integrated)
					integrated = device;
			}

			if (discrete)
				m_physical_device = *discrete;
			else if (integrated)
				m_physical_device = *integrated;
			else if (not devices->empty())
				m_physical_device = devices->front();
			else
				return std::unexpected("No suitable Vulkan devices found");

			return {};
		}

		void log_device_info() const
			{
			VkPhysicalDeviceVulkan12Properties vulkan12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
			VkPhysicalDeviceVulkan13Properties vulkan13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
			VkPhysicalDeviceVulkan14Properties vulkan14{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES};
			VkPhysicalDeviceProperties2        properties2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

			vulkan_pnext_chain(properties2, vulkan12, vulkan13, vulkan14);

			vkGetPhysicalDeviceProperties2(m_physical_device, &properties2);

			dbg::println(
			  "Conformance version: {}.{}.{}.{}",
			  vulkan12.conformanceVersion.major,
			  vulkan12.conformanceVersion.minor,
			  vulkan12.conformanceVersion.subminor,
			  vulkan12.conformanceVersion.patch);
			dbg::println("Device name: {}", properties2.properties.deviceName);
			dbg::println("Driver name: {}", vulkan12.driverName);
			dbg::println("Driver version: {}.0.{}",
						 VK_VERSION_MAJOR(properties2.properties.driverVersion),
						 VK_VERSION_PATCH(properties2.properties.driverVersion));
			}

		// Extensions that are enabled when the device offers them.
		static constexpr std::array<const char*, 8> wanted_device_extensions{
		  VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
		  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		  VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		  VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME,
		  VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
		  VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
		  VK_KHR_MAINTENANCE2_EXTENSION_NAME,
		  VK_EXT_MEMORY_BUDGET_EXTENSION_NAME};

		std::expected<void, std::string> create_device()
			{
			auto extensions = enumerate<VkExtensionProperties>(
			  [this](u32* count, VkExtensionProperties* items)
			  { return vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, count, items); });

			if (not extensions)
			{
				return std::unexpected(
				  std::format("Failed to enumerate device extensions: {}", string_VkResult(extensions.error())));
			}

			m_device_extensions = std::move(*extensions);

			log_device_info();

			auto queue_family = select_queue_family_index();
			if (not queue_family)
				return std::unexpected(queue_family.error());
			m_graphics_queue_family_index = *queue_family;

			const f32                     queue_priority = 1.0f;
			const VkDeviceQueueCreateInfo queue_info{
			  .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			  .queueFamilyIndex = m_graphics_queue_family_index,
			  .queueCount       = 1,
			  .pQueuePriorities = &queue_priority,
			};

			std::vector<const char*> enabled_extensions;
			for (const char* name : wanted_device_extensions)
				if (has_name(m_device_extensions, name, extension_name))
					enabled_extensions.push_back(name);

			bool                                           enable_unified_image_layouts{false};
			VkPhysicalDeviceUnifiedImageLayoutsFeaturesKHR unified_image_layouts{
			  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFIED_IMAGE_LAYOUTS_FEATURES_KHR};
			if (has_name(m_device_extensions, VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME, extension_name))
			{
				VkPhysicalDeviceFeatures2 features2{
				  .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &unified_image_layouts};
				vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);

				if (unified_image_layouts.unifiedImageLayouts)
				{
					enabled_extensions.push_back(VK_KHR_UNIFIED_IMAGE_LAYOUTS_EXTENSION_NAME);
					unified_image_layouts.unifiedImageLayouts = VK_TRUE;
					enable_unified_image_layouts              = true;
				}
			}

			// required features
			VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
			features13.synchronization2 = VK_TRUE;    // Barrier synchronization2
			features13.dynamicRendering = VK_TRUE;    // Dynamic rendering

			VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
			features12.timelineSemaphore   = VK_TRUE; // Timeline semaphores
			features12.descriptorIndexing  = VK_TRUE; // Descriptor indexing
			features12.bufferDeviceAddress = VK_TRUE; // Buffer device address

			VkPhysicalDeviceShaderObjectFeaturesEXT shader_object{
			  .sType        = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
			  .shaderObject = VK_TRUE, // Shader object
			};

			if (unified_image_layouts.unifiedImageLayouts)
				vulkan_pnext_chain(features13, features12, shader_object, unified_image_layouts);
			else
			vulkan_pnext_chain(features13, features12, shader_object);

			const VkDeviceCreateInfo device_info{
			  .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			  .pNext                   = &features13,
			  .queueCreateInfoCount    = 1,
			  .pQueueCreateInfos       = &queue_info,
			  .enabledExtensionCount   = as<u32>(enabled_extensions.size()),
			  .ppEnabledExtensionNames = enabled_extensions.data(),
			};

			if (VkResult result = vkCreateDevice(m_physical_device, &device_info, nullptr, &m_device); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create Vulkan device: {}", string_VkResult(result)));

			m_unified_image_layouts = enable_unified_image_layouts;
			vkGetDeviceQueue(m_device, m_graphics_queue_family_index, 0, &m_graphics_queue);
			if (m_graphics_queue == VK_NULL_HANDLE)
				return std::unexpected("Failed to get graphics queue from device");

			return {};
		}

		std::expected<void, std::string> initialize_device()
			{
			assert::check(m_instance != VK_NULL_HANDLE, "Vulkan instance must be created before initializing device");

			return select_physical_device()
			  .and_then([this] { return create_device(); })
			  .and_then([this] { return update_surface_capabilities(); });
			}

		void deinitialize_device()
		{
			if (m_device != VK_NULL_HANDLE)
			{
				vkDeviceWaitIdle(m_device);
				vkDestroyDevice(m_device, nullptr);
				m_device = VK_NULL_HANDLE;
			}
		}

		// ############################################################
		// Swapchain ##################################################

		[[nodiscard]] VkSurfaceFormatKHR select_surface_format() const
		{
			constexpr VkSurfaceFormatKHR default_format{VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
			assert::check(m_surface != VK_NULL_HANDLE, "Vulkan surface must be created before selecting surface format");

			auto formats = enumerate<VkSurfaceFormatKHR>(
			  [this](u32* count, VkSurfaceFormatKHR* items)
			  { return vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, count, items); });

			if (not formats or formats->empty() or formats->front().format == VK_FORMAT_UNDEFINED)
				return default_format;

			constexpr std::array<VkSurfaceFormatKHR, 2> preferred{{
			  {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
			  {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
			}};

			for (const auto& want : preferred)
				if (auto have = std::ranges::find_if(
					  *formats, [&](const auto& f) { return f.format == want.format and f.colorSpace == want.colorSpace; });
					have != formats->end())
					return *have;

			// Otherwise, first supported format wins
			return formats->front();
		}

		[[nodiscard]] bool surface_extent_changed() const
		{
			const auto capabilities = query_surface_capabilities();
			if (not capabilities)
				return false;

			const VkExtent2D current = capabilities->currentExtent;
			if (current.width == 0 or current.height == 0)
				return false;

			return current.width != m_swapchain_extent.width or current.height != m_swapchain_extent.height;
	}

		std::expected<void, std::string> initialize_swapchain()
	{
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing swapchain");

			if (auto result = update_surface_capabilities(); not result)
				return result;

			u32 min_image_count = m_surface_capabilities.minImageCount + 1;
			if (m_surface_capabilities.maxImageCount != 0)
				min_image_count = std::min(min_image_count, m_surface_capabilities.maxImageCount);

			m_surface_format = select_surface_format();

			// usage: transfer src/dst only when the surface supports them
			const VkImageUsageFlags usage
			  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
				| (m_surface_capabilities.supportedUsageFlags
				   & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT));

			// present mode: first one from the try-order that the surface offers, FIFO is always available
			auto present_modes = enumerate<VkPresentModeKHR>(
			  [this](u32* count, VkPresentModeKHR* items)
			  { return vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, count, items); });
			if (not present_modes)
				return std::unexpected(
				  std::format("Failed to get surface present modes: {}", string_VkResult(present_modes.error())));

			static constexpr std::array vsync_order{
			  VK_PRESENT_MODE_FIFO_KHR,
			  VK_PRESENT_MODE_FIFO_RELAXED_KHR,
			  VK_PRESENT_MODE_MAILBOX_KHR,
			  VK_PRESENT_MODE_IMMEDIATE_KHR};

			static constexpr std::array no_vsync_order{
			  VK_PRESENT_MODE_IMMEDIATE_KHR,
			  VK_PRESENT_MODE_FIFO_KHR,
			  VK_PRESENT_MODE_FIFO_RELAXED_KHR,
			  VK_PRESENT_MODE_MAILBOX_KHR};

			const auto& try_order    = vsync ? vsync_order : no_vsync_order;
			const auto  preferred    = std::ranges::find_first_of(try_order, *present_modes);
			const auto  present_mode = preferred != try_order.end() ? *preferred : VK_PRESENT_MODE_FIFO_KHR;

			const VkSwapchainKHR old_swapchain = m_swapchain;

			const VkSwapchainCreateInfoKHR create_info{
			  .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			  .surface               = m_surface,
			  .minImageCount         = min_image_count,
			  .imageFormat           = m_surface_format.format,
			  .imageColorSpace       = m_surface_format.colorSpace,
			  .imageExtent           = m_surface_capabilities.currentExtent,
			  .imageArrayLayers      = 1,
			  .imageUsage            = usage,
			  .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
			  .queueFamilyIndexCount = 0,
			  .pQueueFamilyIndices   = nullptr,
			  .preTransform          = m_surface_capabilities.currentTransform,
			  .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			  .presentMode           = present_mode,
			  .clipped               = VK_TRUE,
			  .oldSwapchain          = old_swapchain,
			};

			if (VkResult result = vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create swapchain: {}", string_VkResult(result)));

			m_swapchain_transfer_src = (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;
			if (old_swapchain != VK_NULL_HANDLE)
				vkDestroySwapchainKHR(m_device, old_swapchain, nullptr);

			// refresh the extent/capabilities of the new swapchain
			return update_surface_capabilities();
	}

		std::expected<void, std::string> resize_swapchain() { return initialize_swapchain(); }

		std::expected<void, std::string> resize_swapchain_images()
	{
			deinitialize_swapchain_images();
			return initialize_swapchain_images();
		}

		void deinitialize_swapchain() { destroy(m_swapchain, vkDestroySwapchainKHR); }

		// ############################################################
		// Swapchain images ###########################################

		std::expected<void, std::string> initialize_swapchain_images()
		{
			if (m_swapchain == VK_NULL_HANDLE)
				return std::unexpected("Vulkan swapchain must be created before initializing swapchain images");

			auto images = enumerate<VkImage>(
			  [this](u32* count, VkImage* items) { return vkGetSwapchainImagesKHR(m_device, m_swapchain, count, items); });
			if (not images)
				return std::unexpected(std::format("Failed to get swapchain images: {}", string_VkResult(images.error())));

			m_swapchain_images    = std::move(*images);
			swapchain_image_count = as<u32>(m_swapchain_images.size());

			// image views
			m_swapchain_image_views.assign(swapchain_image_count, VK_NULL_HANDLE);

			for (std::size_t i = 0; i < m_swapchain_images.size(); ++i)
			{
				const VkImageViewCreateInfo view_info{
				  .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				  .image            = m_swapchain_images[i],
				  .viewType         = VK_IMAGE_VIEW_TYPE_2D,
				  .format           = m_surface_format.format, // TODO: better format selection
				  .subresourceRange = color_range,             // components default to IDENTITY
				};

				if (VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &m_swapchain_image_views[i]);
					result != VK_SUCCESS)
					return std::unexpected(std::format("Failed to create image view: {}", string_VkResult(result)));
			}

			m_swapchain_extent = m_surface_capabilities.currentExtent;

			return {};
	}

		void deinitialize_swapchain_images()
	{
			for (auto& view : m_swapchain_image_views)
				destroy(view, vkDestroyImageView);

			m_swapchain_image_views.clear();
			m_swapchain_images.clear();
	}

		void destroy_image(image_resource& image) const noexcept
	{
			if (image.sampler != VK_NULL_HANDLE)
				vkDestroySampler(m_device, image.sampler, nullptr);
			if (image.view != VK_NULL_HANDLE)
				vkDestroyImageView(m_device, image.view, nullptr);
			if (image.handle != VK_NULL_HANDLE)
				vkDestroyImage(m_device, image.handle, nullptr);
			if (image.memory != VK_NULL_HANDLE)
				vkFreeMemory(m_device, image.memory, nullptr);
			image = {};
		}

		void deinitialize_uploaded_image() { destroy_image(m_uploaded_image); }

		[[nodiscard]] VkImage get_swapchain_image(u32 index) const
		{
			assert::check(index < m_swapchain_images.size(), "Swapchain image index out of range");
			return m_swapchain_images[index];
		}

		[[nodiscard]] VkImageView get_swapchain_image_view(u32 index) const
		{
			assert::check(index < m_swapchain_image_views.size(), "Swapchain image view index out of range");
			return m_swapchain_image_views[index];
		}

		// ############################################################
		// Command buffers ############################################

		std::expected<void, std::string> allocate_command_buffers(std::span<VkCommandBuffer> out) const
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

		std::expected<void, std::string> initialize_command_buffers()
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

		void deinitialize_command_buffers()
		{
			if (not m_command_buffers.empty())
		{
				vkFreeCommandBuffers(m_device, m_command_pool, as<u32>(m_command_buffers.size()), m_command_buffers.data());
				m_command_buffers.clear();
			}

			destroy(m_command_pool, vkDestroyCommandPool);
		}

		[[nodiscard]] VkCommandBuffer& get_command_buffer(u32 index)
			{
			assert::check(index < m_command_buffers.size(), "Command buffer index out of range");
			return m_command_buffers[index];
			}

		// Barrier (acquire -> attachment), dynamic rendering with a clear and `draw`, barrier (attachment -> present).
		template<std::invocable<VkCommandBuffer> Draw>
		std::expected<void, std::string> record_commands(
		  VkCommandBuffer cmd, VkCommandBufferUsageFlags usage, u32 image_index, VkExtent2D extent,
		  const std::array<f32, 3>& clear_rgb, Draw&& draw)
		{
			const VkCommandBufferBeginInfo begin_info{
			  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			  .flags = usage,
			};
			if (auto result = vkBeginCommandBuffer(cmd, &begin_info); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to begin command buffer: {}", string_VkResult(result)));

			// srcStage matches the acquire semaphore's wait stage
			transition_image(
			  cmd,
			  m_swapchain_images[image_index],
			  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			  VK_ACCESS_2_NONE,
			  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkClearValue clear_value{};
			clear_value.color = {{clear_rgb[0], clear_rgb[1], clear_rgb[2], 1.0f}};

			const VkRenderingAttachmentInfo color_attachment{
			  .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			  .imageView   = m_swapchain_image_views[image_index],
			  .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			  .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
			  .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
			  .clearValue  = clear_value,
			};

			const VkRect2D        render_area{{0, 0}, extent};
			const VkRenderingInfo render_info{
			  .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
			  .renderArea           = render_area,
			  .layerCount           = 1,
			  .colorAttachmentCount = 1,
			  .pColorAttachments    = &color_attachment,
			};

			vkCmdBeginRendering(cmd, &render_info);

			const VkViewport viewport{
			  0.0f, 0.0f, static_cast<f32>(extent.width), static_cast<f32>(extent.height), 0.0f, 1.0f};
			vkCmdSetViewport(cmd, 0, 1, &viewport);
			vkCmdSetScissor(cmd, 0, 1, &render_area);

			std::invoke(std::forward<Draw>(draw), cmd);

			vkCmdEndRendering(cmd);

			if (m_screenshot_buffer)
			{
				transition_image(
				  cmd,
				  m_swapchain_images[image_index],
				  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				  VK_PIPELINE_STAGE_2_COPY_BIT,
				  VK_ACCESS_2_TRANSFER_READ_BIT,
				  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				const VkBufferImageCopy copy_region{
				  .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
				  .imageExtent      = {extent.width, extent.height, 1},
				};
				vkCmdCopyImageToBuffer(
				  cmd,
				  m_swapchain_images[image_index],
				  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				  m_screenshot_buffer->handle,
				  1,
				  &copy_region);

				transition_image(
				  cmd,
				  m_swapchain_images[image_index],
				  VK_PIPELINE_STAGE_2_COPY_BIT,
				  VK_ACCESS_2_TRANSFER_READ_BIT,
				  VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
				  VK_ACCESS_2_NONE,
				  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}
			else
			{
				// Render writes become visible to presentation through the semaphore.
				transition_image(
				  cmd,
				  m_swapchain_images[image_index],
				  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				  VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
				  VK_ACCESS_2_NONE,
				  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
			}

			if (auto result = vkEndCommandBuffer(cmd); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to end command buffer: {}", string_VkResult(result)));

			return {};
		}

		// The two triangle pipelines: [0] has no vertex input, [1] reads `vertex_buffer`.
		void draw_triangles(VkCommandBuffer cmd, VkBuffer vertex_buffer) const
		{
			if (m_pipelines.size() > 0 and m_pipelines[0].pipeline != VK_NULL_HANDLE)
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0].pipeline);
				vkCmdDraw(cmd, 3, 1, 0, 0);
			}

			if (m_pipelines.size() > 1 and m_pipelines[1].pipeline != VK_NULL_HANDLE and vertex_buffer != VK_NULL_HANDLE
				and m_uploaded_image.view != VK_NULL_HANDLE and m_texture_descriptor_set != VK_NULL_HANDLE)
			{
				const VkDeviceSize offset{0};
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[1].pipeline);
				vkCmdBindDescriptorSets(
				  cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[1].layout, 0, 1, &m_texture_descriptor_set, 0, nullptr);

				// push time
				vkCmdPushConstants(
				  cmd,
				  m_pipelines[1].layout,
				  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				  0,
				  sizeof(m_time_seconds),
				  &m_time_seconds);


				vkCmdBindVertexBuffers(cmd, 0, 1, &vertex_buffer, &offset);
				vkCmdDraw(cmd, 3, 1, 0, 0);
			}
		}

		// Pre-records the static command buffers (not used by draw(), which records per frame).
		std::expected<void, std::string> record_clear_commands()
		{
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before recording commands");
			assert::check(m_command_pool != VK_NULL_HANDLE, "Vulkan command pool must be created before recording commands");

			const VkExtent2D extent = m_surface_capabilities.currentExtent;

			for (std::size_t i = 0; i < m_command_buffers.size(); ++i)
				if (auto result = record_commands(
					  m_command_buffers[i],
					  0,
					  as<u32>(i),
					  extent,
					  default_clear_rgb,
					  [this](VkCommandBuffer cmd) { draw_triangles(cmd, m_triangle2.handle); });
					not result)
					return result;

			return {};
		}

		std::expected<void, std::string> record_frame(const frame_slot& frame, u32 image_index)
		{
			const VkExtent2D extent = m_swapchain_extent;

			return record_commands(
			  frame.cmd,
			  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // re-recorded every frame
			  image_index,
			  extent,
			  m_clear_rgb,
			  [&](VkCommandBuffer cmd)
			  {
				  draw_triangles(cmd, frame.vertex.handle);

				  if (frame.batch_count > 0 and m_pipeline_2d.pipeline != VK_NULL_HANDLE)
				  {
					  const std::array<f32, 2> scale{
						2.0f / static_cast<f32>(extent.width), 2.0f / static_cast<f32>(extent.height)};
					  const VkDeviceSize offset{0};

					  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_2d.pipeline);
					  vkCmdPushConstants(
						cmd, m_pipeline_2d.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(scale), scale.data());
					  vkCmdBindVertexBuffers(cmd, 0, 1, &frame.batch.handle, &offset);
					  vkCmdDraw(cmd, frame.batch_count, 1, 0, 0);
				  }
			  });
		}

		VkResult submit(VkSemaphore wait, VkCommandBuffer cmd, VkSemaphore rendering_finished, u64 signal_value) const
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

		VkResult submit_command() const
		{
			return submit(
			  m_image_available[m_currentframe],
			  m_command_buffers[m_command_buffer_index],
			  m_rendering_finished[m_command_buffer_index],
			  m_signal_value);
		}

		VkResult submit_frame(const frame_slot& frame, u32 image_index, u64 signal_value) const
		{
			return submit(frame.image_available, frame.cmd, m_rendering_finished[image_index], signal_value);
		}

		// ############################################################
		// Semaphores #################################################

		VkSemaphore create_semaphore(VkSemaphoreType type = VK_SEMAPHORE_TYPE_BINARY) const
		{
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be valid to create semaphore");

			const VkSemaphoreTypeCreateInfo type_info{
			  .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			  .semaphoreType = type,
			  .initialValue  = 0,
			};
			const VkSemaphoreCreateInfo semaphore_info{
			  .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			  .pNext = type == VK_SEMAPHORE_TYPE_TIMELINE ? &type_info : nullptr,
			};

			VkSemaphore semaphore{VK_NULL_HANDLE};
			if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &semaphore) != VK_SUCCESS)
				return VK_NULL_HANDLE;
			return semaphore;
		}

		void assert_timeline(VkSemaphore semaphore) const
		{
			assert::check(semaphore != VK_NULL_HANDLE, "Timeline semaphore must be valid");
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be valid to use timeline semaphore");
		}

		void wait_for_timeline_semaphore(VkSemaphore semaphore, u64 value, u64 timeout = UINT64_MAX) const
		{
			assert_timeline(semaphore);

			const VkSemaphoreWaitInfo wait_info{
			  .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			  .semaphoreCount = 1,
			  .pSemaphores    = &semaphore,
			  .pValues        = &value,
			};
			const auto result = vkWaitSemaphores(m_device, &wait_info, timeout);
			assert::check(result == VK_SUCCESS, "Failed to wait for timeline semaphore");
		}

		[[nodiscard]] u64 get_timeline_semaphore_value(VkSemaphore semaphore) const
		{
			assert_timeline(semaphore);

			u64        value  = 0;
			const auto result = vkGetSemaphoreCounterValue(m_device, semaphore, &value);
			assert::check(result == VK_SUCCESS, "Failed to get timeline semaphore value");
			return value;
		}

		void signal_timeline_semaphore(VkSemaphore semaphore, u64 value) const
		{
			assert_timeline(semaphore);

			const VkSemaphoreSignalInfo signal_info{
			  .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			  .semaphore = semaphore,
			  .value     = value,
			};
			const auto result = vkSignalSemaphore(m_device, &signal_info);
			assert::check(result == VK_SUCCESS, "Failed to signal timeline semaphore");
		}

		// ############################################################
		// Synchronization ############################################

		std::expected<void, std::string> initialize_synchronization()
		{
			if (m_device == VK_NULL_HANDLE)
				return std::unexpected("Vulkan device must be created before initializing synchronization");


			m_in_flight_timeline = create_semaphore(VK_SEMAPHORE_TYPE_TIMELINE);
			m_in_flight_timeline_values.assign(swapchain_image_count, 0);

			m_timeline_counter = 0;
			m_currentframe     = 0;

			for (auto* semaphores : {&m_image_available, &m_rendering_finished})
			{
				semaphores->resize(swapchain_image_count);
				std::ranges::generate(*semaphores, [this] { return create_semaphore(); });
			}

			if (m_in_flight_timeline == VK_NULL_HANDLE
				or std::ranges::any_of(m_image_available, [](VkSemaphore s) { return s == VK_NULL_HANDLE; })
				or std::ranges::any_of(m_rendering_finished, [](VkSemaphore s) { return s == VK_NULL_HANDLE; }))
				return std::unexpected("Failed to create synchronization semaphores");


			return {};
		}

		void destroy_synchronization()
		{
			destroy(m_in_flight_timeline, vkDestroySemaphore);

			for (auto* semaphores : {&m_rendering_finished, &m_image_available})
				for (auto& semaphore : *semaphores)
					destroy(semaphore, vkDestroySemaphore);
		}

		std::expected<void, std::string> resize_synchronization()
			{
			if (m_device == VK_NULL_HANDLE)
				return std::unexpected("Vulkan device must be created before resizing synchronization");
			destroy_synchronization();
			return initialize_synchronization();
			}

		void deinitialize_synchronization()
			{
			if (m_device == VK_NULL_HANDLE)
				return;
			vkDeviceWaitIdle(m_device);
			destroy_synchronization();
		}

		// ############################################################
		// Pipelines ##################################################

		void destroy_pipeline(pipeline_state& state) const noexcept
		{
			destroy(state.pipeline, vkDestroyPipeline);
			destroy(state.layout, vkDestroyPipelineLayout);
			destroy(state.cache, vkDestroyPipelineCache);
			}

		std::expected<pipeline_state, std::string> create_pipeline(
		  VkShaderModule vertex, VkShaderModule fragment, std::span<const VkVertexInputBindingDescription> bindings = {},
		  std::span<const VkVertexInputAttributeDescription> attributes = {}, const pipeline_desc& desc = {}) const
		{
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing pipeline");

			// TODO: cache
			pipeline_state state{};

			const auto fail = [&](std::string message)
			{
				destroy_pipeline(state);
				return std::unexpected(std::move(message));
			};

			const VkPipelineCacheCreateInfo cache_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
			if (vkCreatePipelineCache(m_device, &cache_info, nullptr, &state.cache) != VK_SUCCESS)
			{
				dbg::println("Failed to create pipeline cache, continuing without cache");
				state.cache = VK_NULL_HANDLE;
			}

			// layout
			const VkPushConstantRange push_range{
			  .stageFlags = desc.push_stages,
			  .offset     = 0,
			  .size       = desc.push_size,
			};
			const VkPipelineLayoutCreateInfo layout_info{
			  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			  .setLayoutCount         = desc.descriptor_set_layout != VK_NULL_HANDLE ? 1u : 0u,
			  .pSetLayouts            = desc.descriptor_set_layout != VK_NULL_HANDLE ? &desc.descriptor_set_layout : nullptr,
			  .pushConstantRangeCount = desc.push_size != 0 ? 1u : 0u,
			  .pPushConstantRanges    = desc.push_size != 0 ? &push_range : nullptr,
			};
			if (VkResult result = vkCreatePipelineLayout(m_device, &layout_info, nullptr, &state.layout);
				result != VK_SUCCESS)
				return fail(std::format("Failed to create pipeline layout: {}", string_VkResult(result)));

			const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{{
			  {
				.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage  = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vertex,
				.pName  = "main",
			  },
			  {
				.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = fragment,
				.pName  = "main",
			  },
			  }};

			const VkPipelineVertexInputStateCreateInfo vertex_input_info{
			  .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			  .vertexBindingDescriptionCount   = as<u32>(bindings.size()),
			  .pVertexBindingDescriptions      = bindings.empty() ? nullptr : bindings.data(),
			  .vertexAttributeDescriptionCount = as<u32>(attributes.size()),
			  .pVertexAttributeDescriptions    = attributes.empty() ? nullptr : attributes.data(),
			};

			const VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
			  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			  .topology               = desc.topology,
			  .primitiveRestartEnable = VK_FALSE,
			};

			// dynamic viewport and scissor
			const VkPipelineViewportStateCreateInfo viewport_state_info{
			  .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			  .viewportCount = 1,
			  .scissorCount  = 1,
			};

			const VkPipelineRasterizationStateCreateInfo rasterizer_info{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			  .depthClampEnable        = VK_FALSE,
			  .rasterizerDiscardEnable = VK_FALSE,
			  .polygonMode             = VK_POLYGON_MODE_FILL,
			  .cullMode                = desc.cull_mode,
			  .frontFace               = VK_FRONT_FACE_CLOCKWISE,
			  .depthBiasEnable         = VK_FALSE,
			  .lineWidth               = 1.0f,
			};

			const VkPipelineMultisampleStateCreateInfo multisample_info{
			  .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			  .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			};

			const VkPipelineColorBlendAttachmentState color_blend_attachment{
			  .blendEnable    = VK_FALSE,
			  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
								| VK_COLOR_COMPONENT_A_BIT,
			};

			const VkPipelineColorBlendStateCreateInfo color_blend_info{
			  .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			  .logicOpEnable   = VK_FALSE,
			  .attachmentCount = 1,
			  .pAttachments    = &color_blend_attachment,
			};

			constexpr std::array<VkDynamicState, 2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

			const VkPipelineDynamicStateCreateInfo dynamic_state_info{
			  .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			  .dynamicStateCount = as<u32>(dynamic_states.size()),
			  .pDynamicStates    = dynamic_states.data(),
			};

			// creation feedback
			std::array<VkPipelineCreationFeedback, 2> stage_feedback{};
			VkPipelineCreationFeedback                creation_feedback{};

			const VkPipelineCreationFeedbackCreateInfo feedback_info{
			  .sType                              = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO,
			  .pPipelineCreationFeedback          = &creation_feedback,
			  .pipelineStageCreationFeedbackCount = as<u32>(stage_feedback.size()),
			  .pPipelineStageCreationFeedbacks    = stage_feedback.data(),
			};

			// dynamic rendering
			const VkPipelineRenderingCreateInfo rendering_info{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			  .pNext                   = &feedback_info,
			  .colorAttachmentCount    = 1,
			  .pColorAttachmentFormats = &m_surface_format.format,
			  .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
			  .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
			};

			const VkGraphicsPipelineCreateInfo pipeline_info{
			  .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			  .pNext               = &rendering_info,
			  .stageCount          = as<u32>(shader_stages.size()),
			  .pStages             = shader_stages.data(),
			  .pVertexInputState   = &vertex_input_info,
			  .pInputAssemblyState = &input_assembly_info,
			  .pViewportState      = &viewport_state_info,
			  .pRasterizationState = &rasterizer_info,
			  .pMultisampleState   = &multisample_info,
			  .pColorBlendState    = &color_blend_info,
			  .pDynamicState       = &dynamic_state_info,
			  .layout              = state.layout,
			  .renderPass          = VK_NULL_HANDLE,
			  .subpass             = 0,
			};

			if (VkResult result = vkCreateGraphicsPipelines(
				  m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &state.pipeline);
				result != VK_SUCCESS)
				return fail(std::format("Failed to create graphics pipeline: {}", string_VkResult(result)));

			return state;
		}

		void deinitialize_pipeline()
		{
			for (auto& pipeline : m_pipelines)
				destroy_pipeline(pipeline);

			m_pipelines.clear();
		}

		// ############################################################
		// Buffers ####################################################


		// ############################################################
		// Triangle renderer ##########################################

		std::expected<void, std::string> initialize_triangle_buffer(std::span<const triangle2_vertex> vertices)
		{
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing triangle buffer");

			if (vertices.empty())
				return std::unexpected("Triangle vertex data must not be empty");

			if (auto result = create_buffer(
				  vertices.size_bytes(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_triangle2, "triangle vertex");
				not result)
				return result;

			std::memcpy(m_triangle2.mapped, vertices.data(), vertices.size_bytes());
			return {};
		}

		void deinitialize_triangle_buffer() { destroy_buffer(m_triangle2); }

		void update_texture_descriptor(const image_resource& image) const
		{
			if (m_texture_descriptor_set == VK_NULL_HANDLE or image.view == VK_NULL_HANDLE)
				return;

			const VkDescriptorImageInfo image_info{
			  .sampler     = image.sampler,
			  .imageView   = image.view,
			  .imageLayout = m_unified_image_layouts ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};
			const VkWriteDescriptorSet write{
			  .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			  .dstSet          = m_texture_descriptor_set,
			  .dstBinding      = 0,
			  .descriptorCount = 1,
			  .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			  .pImageInfo      = &image_info,
			};
			vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
		}

		std::expected<void, std::string> initialize_texture_descriptor()
		{
			const VkDescriptorSetLayoutBinding binding{
			  .binding         = 0,
			  .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			  .descriptorCount = 1,
			  .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
			};
			const VkDescriptorSetLayoutCreateInfo layout_info{
			  .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			  .bindingCount = 1,
			  .pBindings    = &binding,
			};
			if (VkResult result = vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_texture_descriptor_layout);
				result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to create texture descriptor set layout: {}", string_VkResult(result)));

			const VkDescriptorPoolSize       pool_size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
			const VkDescriptorPoolCreateInfo pool_info{
			  .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			  .maxSets       = 1,
			  .poolSizeCount = 1,
			  .pPoolSizes    = &pool_size,
			};
			if (VkResult result = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_texture_descriptor_pool);
				result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create texture descriptor pool: {}", string_VkResult(result)));

			const VkDescriptorSetAllocateInfo allocate_info{
			  .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			  .descriptorPool     = m_texture_descriptor_pool,
			  .descriptorSetCount = 1,
			  .pSetLayouts        = &m_texture_descriptor_layout,
			};
			if (VkResult result = vkAllocateDescriptorSets(m_device, &allocate_info, &m_texture_descriptor_set);
				result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to allocate texture descriptor set: {}", string_VkResult(result)));

			update_texture_descriptor(m_uploaded_image);
			return {};
		}

		std::expected<void, std::string> build_triangle_renderer()
		{
			shader_set<4> shaders{m_device};
			if (auto result = shaders.load_all(
				  fs::current_path() / "shaders", {"triangle.vert", "triangle.frag", "triangle2.vert", "triangle2.frag"});
				not result)
				return result;
			if (auto result = initialize_texture_descriptor(); not result)
				return result;

			const auto add_pipeline = [this](auto&&... args) -> std::expected<void, std::string>
			{
				auto pipeline = create_pipeline(std::forward<decltype(args)>(args)...);
				if (not pipeline)
					return std::unexpected(std::move(pipeline.error()));

				m_pipelines.push_back(std::move(*pipeline));
				return {};
			};

			const std::array<VkVertexInputBindingDescription, 1> bindings{{
			  {.binding = 0, .stride = sizeof(triangle2_vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
			}};

			const std::array<VkVertexInputAttributeDescription, 3> attributes{{
			  {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 0},
			  {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = sizeof(vec::vec2)},
			  {.location = 2,
			   .binding  = 0,
			   .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
			   .offset   = offsetof(triangle2_vertex, color)},

			}};

			// triangle.frag is used by the untextured triangle.
			if (auto result = add_pipeline(shaders[0], shaders[1]); not result)
				return result;

			const pipeline_desc texture_desc{
			  //  .cull_mode           = VK_CULL_MODE_NONE,
			  .push_stages           = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			  .push_size             = sizeof(f32),
			  .descriptor_set_layout = m_texture_descriptor_layout,
			};
			if (auto result = add_pipeline(shaders[2], shaders[3], bindings, attributes, texture_desc); not result)
				return result;

			m_triangle2_origin = {
			  vec::vec2{0.55f, -0.75f},
			  vec::vec2{0.80f, -0.25f},
			  vec::vec2{0.30f, -0.25f},
			};
			m_triangle2_color = {{{0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}};

			for (u32 i = 0; i < 3; ++i)
			{
				m_triangle2_angle[i]            = random::rnd<f32>(0.0f, std::numbers::pi_v<f32> * 2.0f);
				const f32 speed                 = random::rnd<f32>(1.0f, 3.0f);
				m_triangle2_angular_speed[i]    = random::randbool() ? speed : -speed;
				m_triangle2_initial_vertices[i] = {m_triangle2_origin[i], triangle2_uvs[i], m_triangle2_color[i]};
			}

			if (auto result = initialize_triangle_buffer(m_triangle2_initial_vertices); not result)
				return result;

			m_last_frame_time = std::chrono::steady_clock::now();
			m_time_seconds    = 0.0f;
			return {};
		}

		std::expected<void, std::string> initialize_triangle_renderer()
		{
			deinitialize_triangle_renderer();

			auto result = build_triangle_renderer();
			if (not result)
				deinitialize_triangle_renderer();

			return result;
	}

		void deinitialize_triangle_renderer()
	{
			deinitialize_triangle_buffer();
			deinitialize_pipeline();
			if (m_texture_descriptor_pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_device, m_texture_descriptor_pool, nullptr);
			m_texture_descriptor_pool = VK_NULL_HANDLE;
			m_texture_descriptor_set  = VK_NULL_HANDLE;
			if (m_texture_descriptor_layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(m_device, m_texture_descriptor_layout, nullptr);
			m_texture_descriptor_layout = VK_NULL_HANDLE;
		}

		// Advances the three orbiting vertices by `dt` and writes them to `destination`.
		void write_triangle2(void* destination, f32 dt)
		{
		constexpr f32 orbit_radius = 0.06f;

		std::array<triangle2_vertex, 3> vertices{};
		for (u32 i = 0; i < 3; ++i)
		{
			m_triangle2_angle[i] += m_triangle2_angular_speed[i] * dt;

				const vec::vec2 offset = vec::vec2{orbit_radius, 0.0f}.rotate(m_triangle2_angle[i], vec::vec2::zero());
				vertices[i].pos        = m_triangle2_origin[i] + offset;
				vertices[i].uv         = triangle2_uvs[i];
				vertices[i].color      = m_triangle2_color[i];
			}

			std::memcpy(destination, vertices.data(), sizeof(vertices)); // HOST_COHERENT, mapped once
		}

		void update_triangle2(f32 dt)
		{
			if (m_triangle2.mapped != nullptr)
				write_triangle2(m_triangle2.mapped, dt);
	}

		void update_triangle2(frame_slot& frame, f32 dt)
	{
			assert::check(frame.vertex.mapped != nullptr, "Frame vertex buffer must be mapped");
			write_triangle2(frame.vertex.mapped, dt);
		}

		// ############################################################
		// Frames #####################################################

		std::expected<void, std::string> initialize_frames()
		{
			assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing frames");
			assert::check(m_command_pool != VK_NULL_HANDLE, "Command pool must be created before initializing frames");

			// Allocate every command buffer first and store them immediately, so a later
			// failure can always free them through deinitialize_frames().
			std::array<VkCommandBuffer, frames_in_flight> cmds{};
			if (auto result = allocate_command_buffers(cmds); not result)
				return result;

			for (u32 i = 0; i < frames_in_flight; ++i)
				m_frames[i] = frame_slot{.cmd = cmds[i]};

			for (auto& frame : m_frames)
			{
				frame.image_available = create_semaphore();
				if (frame.image_available == VK_NULL_HANDLE)
		{
					deinitialize_frames();
					return std::unexpected("Failed to create image_available semaphore");
		}

				auto result = create_buffer(
				  sizeof(triangle2_vertex) * 3, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, frame.vertex, "frame vertex");
				if (result)
					result = create_buffer(
					  sizeof(vertex2d) * max_batch_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, frame.batch, "frame batch");

				if (not result)
		{
					deinitialize_frames();
					return result;
				}
		}

			return {};
		}

		void deinitialize_frames()
		{
			if (m_device == VK_NULL_HANDLE)
				return;

			vkDeviceWaitIdle(m_device);

			std::array<VkCommandBuffer, frames_in_flight> cmds{};
			u32                                           cmd_count{0};

			for (auto& frame : m_frames)
		{
				if (frame.cmd != VK_NULL_HANDLE)
					cmds[cmd_count++] = frame.cmd;

				destroy(frame.image_available, vkDestroySemaphore);
				destroy_buffer(frame.vertex);
				destroy_buffer(frame.batch);

				frame = {};
			}

			if (cmd_count > 0 and m_command_pool != VK_NULL_HANDLE)
				vkFreeCommandBuffers(m_device, m_command_pool, cmd_count, cmds.data());
		}

		void upload_batch(frame_slot& frame, std::span<const vertex2d> batch) const noexcept
		{
			u32 count = static_cast<u32>(std::min<std::size_t>(batch.size(), max_batch_vertices));
			count -= count % 3; // whole triangles only
			if (count != 0)
				std::memcpy(frame.batch.mapped, batch.data(), count * sizeof(vertex2d));
			frame.batch_count = count;
		}

		// ############################################################
		// Renderer 2D ################################################

		std::expected<void, std::string> initialize_renderer2d()
		{
			shader_set<2> shaders{m_device};
			if (auto result = shaders.load_all(fs::current_path() / "shaders", {"batch2d.vert", "batch2d.frag"}); not result)
				return result;

			const std::array<VkVertexInputBindingDescription, 1> bindings{{
			  {.binding = 0, .stride = sizeof(vertex2d), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
			}};

			const std::array<VkVertexInputAttributeDescription, 2> attributes{{
			  {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex2d, v)},
			  {.location = 1, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(vertex2d, rgba)},
			}};

			const pipeline_desc desc{
			  .topology    = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			  .cull_mode   = VK_CULL_MODE_NONE,
			  .push_stages = VK_SHADER_STAGE_VERTEX_BIT,
			  .push_size   = sizeof(f32) * 2,
			};

			auto pipeline = create_pipeline(shaders[0], shaders[1], bindings, attributes, desc);
			if (not pipeline)
				return std::unexpected(std::move(pipeline.error()));

			m_pipeline_2d = std::move(*pipeline);
			return {};
	}

		void deinitialize_renderer2d() { destroy_pipeline(m_pipeline_2d); }

	};
} // namespace deckard::vulkan
