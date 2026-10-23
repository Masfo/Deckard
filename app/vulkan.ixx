module;
#include <windows.h>

#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan;
export import :shaders;

import std;
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

	constexpr VkImageSubresourceRange color_range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	inline constexpr std::array<f32, 3> default_clear_rgb{0.0f, 0.5f, 0.75f};


	static constexpr std::array<vec::vec2, 3> triangle2_uvs{
	  vec::vec2{0.5f, 0.0f}, vec::vec2{1.0f, 1.0f}, vec::vec2{0.0f, 1.0f}};

	export struct buffer
	{
		VkBuffer              handle{};
		VkDeviceMemory        memory{};
		VkDeviceSize          size{};
		VkMemoryPropertyFlags memory_properties{};
		void*                 mapped{}; // non-null only for HOST_VISIBLE memory
	};

	struct image_resource
	{
		VkImage        handle{};
		VkDeviceMemory memory{};
		VkImageView    view{};
		VkSampler      sampler{};
	};

	export struct texture_id
	{
		u32 value{std::numeric_limits<u32>::max()};

		[[nodiscard]] explicit operator bool() const noexcept { return value != std::numeric_limits<u32>::max(); }

		[[nodiscard]] friend bool operator==(texture_id, texture_id) noexcept = default;
	};

	struct texture_resource
	{
		image_resource  image{};
		VkDescriptorSet descriptor_set{VK_NULL_HANDLE};
	};

	struct texture_batch
	{
		texture_id id{};
		u32        first_vertex{};
		u32        vertex_count{};
	};

	struct frame_slot
	{
		VkCommandBuffer            cmd{};
		VkSemaphore                image_available{};
		buffer                     vertex{}; // animated triangle2 vertices
		buffer                     batch{};  // 2D batch vertices
		u32                        batch_count{0};
		std::vector<texture_batch> texture_batches;
		u64                        timeline_value{0};
	};

	export struct vertex2d
	{
		vec::vec2 v;
		vec::vec2 uv;
		u32       rgba;       // r | g<<8 | b<<16 | a<<24 -> bytes R,G,B,A in memory (little-endian)
		u32       texture_id; // Index into the context's stable texture table
	};

	static_assert(sizeof(vec::vec2) == 2 * sizeof(f32));
	static_assert(std::is_standard_layout_v<vertex2d>);           // offsetof needs this

	export inline constexpr u32 max_batch_vertices = 3u * 32768u; // multiple of 3, ~1.1 MiB per frame slot

	export struct pipeline_desc
	{
		VkPrimitiveTopology   topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP};
		VkCullModeFlags       cull_mode{VK_CULL_MODE_BACK_BIT};
		VkShaderStageFlags    push_stages{0};
		u32                   push_size{0};
		VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
		bool                  alpha_blend{false};
	};

	export struct pipeline_state
	{
		VkPipelineLayout layout{VK_NULL_HANDLE};
		VkPipeline       pipeline{VK_NULL_HANDLE};
		VkPipelineCache  cache{VK_NULL_HANDLE};
		fs::path         cache_file{};
	};

	// Configuration for dynamic-rendering graphics pipelines. The defaults match the
	// renderer's existing triangle pipelines; adjust only the state a pipeline needs.
	export struct graphics_shader_stage
	{
		VkShaderStageFlagBits stage{VK_SHADER_STAGE_VERTEX_BIT};
		VkShaderModule        module{VK_NULL_HANDLE};
		std::string           entry_point{"main"};
	};

	export struct graphics_pipeline_config
	{
		std::vector<graphics_shader_stage>             shader_stages;
		std::vector<VkVertexInputBindingDescription>   vertex_bindings;
		std::vector<VkVertexInputAttributeDescription> vertex_attributes;
		std::vector<VkDescriptorSetLayout>             descriptor_set_layouts;
		std::vector<VkPushConstantRange>               push_constant_ranges;

		VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP};
		VkBool32            primitive_restart_enable{VK_FALSE};

		VkBool32        depth_clamp_enable{VK_FALSE};
		VkBool32        rasterizer_discard_enable{VK_FALSE};
		VkPolygonMode   polygon_mode{VK_POLYGON_MODE_FILL};
		VkCullModeFlags cull_mode{VK_CULL_MODE_BACK_BIT};
		VkFrontFace     front_face{VK_FRONT_FACE_CLOCKWISE};
		VkBool32        depth_bias_enable{VK_FALSE};
		f32             depth_bias_constant_factor{0.0f};
		f32             depth_bias_clamp{0.0f};
		f32             depth_bias_slope_factor{0.0f};
		f32             line_width{1.0f};

		VkSampleCountFlagBits rasterization_samples{VK_SAMPLE_COUNT_1_BIT};
		VkBool32              sample_shading_enable{VK_FALSE};
		f32                   min_sample_shading{1.0f};
		VkBool32              alpha_to_coverage_enable{VK_FALSE};
		VkBool32              alpha_to_one_enable{VK_FALSE};

		VkBool32         depth_test_enable{VK_FALSE};
		VkBool32         depth_write_enable{VK_FALSE};
		VkCompareOp      depth_compare_op{VK_COMPARE_OP_LESS};
		VkBool32         depth_bounds_test_enable{VK_FALSE};
		VkBool32         stencil_test_enable{VK_FALSE};
		VkStencilOpState stencil_front{};
		VkStencilOpState stencil_back{};
		f32              min_depth_bounds{0.0f};
		f32              max_depth_bounds{1.0f};
		VkFormat         depth_attachment_format{VK_FORMAT_UNDEFINED};
		VkFormat         stencil_attachment_format{VK_FORMAT_UNDEFINED};

		std::vector<VkFormat>                            color_attachment_formats;
		std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments{VkPipelineColorBlendAttachmentState{
		  .blendEnable    = VK_FALSE,
		  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
							| VK_COLOR_COMPONENT_A_BIT,
		}};
		VkBool32                                         logic_op_enable{VK_FALSE};
		VkLogicOp                                        logic_op{VK_LOGIC_OP_COPY};
		std::array<f32, 4>                               blend_constants{};

		std::vector<VkDynamicState> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		std::vector<VkViewport>     viewports;
		std::vector<VkRect2D>       scissors;
	};

	export class pipeline_creator
	{
	public:
		[[nodiscard]] static std::expected<pipeline_state, std::string> create(
		  VkDevice device, VkShaderModule vertex, VkShaderModule fragment, VkFormat color_format,
		  const graphics_pipeline_config& config = {})
		{
			if (device == VK_NULL_HANDLE)
				return std::unexpected("Vulkan device must be initialized before creating a graphics pipeline");

			if (config.shader_stages.empty() and (vertex == VK_NULL_HANDLE or fragment == VK_NULL_HANDLE))
				return std::unexpected("Graphics pipeline requires valid vertex and fragment shader modules");

			if (config.shader_stages.size() > std::numeric_limits<u32>::max())
				return std::unexpected("Graphics pipeline configuration has too many shader stages");

			std::vector<VkFormat> color_formats = config.color_attachment_formats;
			if (color_formats.empty() and color_format != VK_FORMAT_UNDEFINED)
				color_formats.push_back(color_format);

			if (color_formats.size() != config.color_blend_attachments.size())
				return std::unexpected("Color attachment formats and blend states must have matching counts");

			if (config.descriptor_set_layouts.size() > std::numeric_limits<u32>::max()
				or config.push_constant_ranges.size() > std::numeric_limits<u32>::max()
				or config.vertex_bindings.size() > std::numeric_limits<u32>::max()
				or config.vertex_attributes.size() > std::numeric_limits<u32>::max()
				or config.dynamic_states.size() > std::numeric_limits<u32>::max()
				or config.viewports.size() > std::numeric_limits<u32>::max()
				or config.scissors.size() > std::numeric_limits<u32>::max()
				or color_formats.size() > std::numeric_limits<u32>::max())
				return std::unexpected("Graphics pipeline configuration has too many state entries");

			pipeline_state state{};
			const auto     fail = [&](std::string message)
			{
				destroy(device, state);
				return std::expected<pipeline_state, std::string>{std::unexpected(std::move(message))};
			};

			const VkPipelineCacheCreateInfo cache_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
			if (VkResult result = vkCreatePipelineCache(device, &cache_info, nullptr, &state.cache); result != VK_SUCCESS)
				return fail(std::format("Failed to create pipeline cache: {}", string_VkResult(result)));

			const VkPipelineLayoutCreateInfo layout_info{
			  .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			  .setLayoutCount = static_cast<u32>(config.descriptor_set_layouts.size()),
			  .pSetLayouts    = config.descriptor_set_layouts.empty() ? nullptr : config.descriptor_set_layouts.data(),
			  .pushConstantRangeCount = static_cast<u32>(config.push_constant_ranges.size()),
			  .pPushConstantRanges    = config.push_constant_ranges.empty() ? nullptr : config.push_constant_ranges.data(),
			};
			if (VkResult result = vkCreatePipelineLayout(device, &layout_info, nullptr, &state.layout); result != VK_SUCCESS)
				return fail(std::format("Failed to create pipeline layout: {}", string_VkResult(result)));

			std::vector<VkPipelineShaderStageCreateInfo> stages;
			if (config.shader_stages.empty())
			{
				stages = {
				  VkPipelineShaderStageCreateInfo{
					.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage  = VK_SHADER_STAGE_VERTEX_BIT,
					.module = vertex,
					.pName  = "main",
				  },
				  VkPipelineShaderStageCreateInfo{
					.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = fragment,
					.pName  = "main",
				  },
				};
			}
			else
			{
				stages.reserve(config.shader_stages.size());
				VkShaderStageFlags enabled_stages{0};
				for (const auto& shader_stage : config.shader_stages)
				{
					if (shader_stage.module == VK_NULL_HANDLE or shader_stage.entry_point.empty())
						return fail("Graphics pipeline shader stages require valid modules and entry points");
					if ((enabled_stages & shader_stage.stage) != 0)
						return fail("Graphics pipeline shader stages must not contain duplicate stages");
					enabled_stages |= shader_stage.stage;
					stages.push_back({
					  .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					  .stage  = shader_stage.stage,
					  .module = shader_stage.module,
					  .pName  = shader_stage.entry_point.c_str(),
					});
				}
				if ((enabled_stages & VK_SHADER_STAGE_VERTEX_BIT) == 0
					or (enabled_stages & VK_SHADER_STAGE_FRAGMENT_BIT) == 0)
					return fail("Graphics pipeline requires vertex and fragment shader stages");
			}

			const VkPipelineVertexInputStateCreateInfo vertex_input{
			  .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			  .vertexBindingDescriptionCount   = static_cast<u32>(config.vertex_bindings.size()),
			  .pVertexBindingDescriptions      = config.vertex_bindings.empty() ? nullptr : config.vertex_bindings.data(),
			  .vertexAttributeDescriptionCount = static_cast<u32>(config.vertex_attributes.size()),
			  .pVertexAttributeDescriptions = config.vertex_attributes.empty() ? nullptr : config.vertex_attributes.data(),
			};
			const VkPipelineInputAssemblyStateCreateInfo input_assembly{
			  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			  .topology               = config.topology,
			  .primitiveRestartEnable = config.primitive_restart_enable,
			};
			const bool dynamic_viewport = std::ranges::contains(config.dynamic_states, VK_DYNAMIC_STATE_VIEWPORT);
			const bool dynamic_scissor  = std::ranges::contains(config.dynamic_states, VK_DYNAMIC_STATE_SCISSOR);
			if (not dynamic_viewport and config.viewports.empty())
				return fail("A static viewport must be supplied when viewport is not dynamic");
			if (not dynamic_scissor and config.scissors.empty())
				return fail("A static scissor must be supplied when scissor is not dynamic");
			if (not dynamic_viewport and not dynamic_scissor and config.viewports.size() != config.scissors.size())
				return fail("Static viewport and scissor counts must match");
			const VkPipelineViewportStateCreateInfo viewport_state{
			  .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			  .viewportCount = dynamic_viewport ? 1u : static_cast<u32>(config.viewports.size()),
			  .pViewports    = dynamic_viewport ? nullptr : config.viewports.data(),
			  .scissorCount  = dynamic_scissor ? 1u : static_cast<u32>(config.scissors.size()),
			  .pScissors     = dynamic_scissor ? nullptr : config.scissors.data(),
			};
			const VkPipelineRasterizationStateCreateInfo rasterization{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			  .depthClampEnable        = config.depth_clamp_enable,
			  .rasterizerDiscardEnable = config.rasterizer_discard_enable,
			  .polygonMode             = config.polygon_mode,
			  .cullMode                = config.cull_mode,
			  .frontFace               = config.front_face,
			  .depthBiasEnable         = config.depth_bias_enable,
			  .depthBiasConstantFactor = config.depth_bias_constant_factor,
			  .depthBiasClamp          = config.depth_bias_clamp,
			  .depthBiasSlopeFactor    = config.depth_bias_slope_factor,
			  .lineWidth               = config.line_width,
			};
			const VkPipelineMultisampleStateCreateInfo multisample{
			  .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			  .rasterizationSamples  = config.rasterization_samples,
			  .sampleShadingEnable   = config.sample_shading_enable,
			  .minSampleShading      = config.min_sample_shading,
			  .alphaToCoverageEnable = config.alpha_to_coverage_enable,
			  .alphaToOneEnable      = config.alpha_to_one_enable,
			};
			const VkPipelineDepthStencilStateCreateInfo depth_stencil{
			  .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			  .depthTestEnable       = config.depth_test_enable,
			  .depthWriteEnable      = config.depth_write_enable,
			  .depthCompareOp        = config.depth_compare_op,
			  .depthBoundsTestEnable = config.depth_bounds_test_enable,
			  .stencilTestEnable     = config.stencil_test_enable,
			  .front                 = config.stencil_front,
			  .back                  = config.stencil_back,
			  .minDepthBounds        = config.min_depth_bounds,
			  .maxDepthBounds        = config.max_depth_bounds,
			};
			const VkPipelineColorBlendStateCreateInfo blend{
			  .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			  .logicOpEnable   = config.logic_op_enable,
			  .logicOp         = config.logic_op,
			  .attachmentCount = static_cast<u32>(config.color_blend_attachments.size()),
			  .pAttachments    = config.color_blend_attachments.empty() ? nullptr : config.color_blend_attachments.data(),
			  .blendConstants
			  = {config.blend_constants[0], config.blend_constants[1], config.blend_constants[2], config.blend_constants[3]},
			};
			const VkPipelineDynamicStateCreateInfo dynamic_state{
			  .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			  .dynamicStateCount = static_cast<u32>(config.dynamic_states.size()),
			  .pDynamicStates    = config.dynamic_states.empty() ? nullptr : config.dynamic_states.data(),
			};
			std::vector<VkPipelineCreationFeedback>    stage_feedback(stages.size());
			VkPipelineCreationFeedback                 creation_feedback{};
			const VkPipelineCreationFeedbackCreateInfo feedback{
			  .sType                              = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO,
			  .pPipelineCreationFeedback          = &creation_feedback,
			  .pipelineStageCreationFeedbackCount = static_cast<u32>(stage_feedback.size()),
			  .pPipelineStageCreationFeedbacks    = stage_feedback.empty() ? nullptr : stage_feedback.data(),
			};
			const VkPipelineRenderingCreateInfo rendering{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			  .pNext                   = &feedback,
			  .colorAttachmentCount    = static_cast<u32>(color_formats.size()),
			  .pColorAttachmentFormats = color_formats.empty() ? nullptr : color_formats.data(),
			  .depthAttachmentFormat   = config.depth_attachment_format,
			  .stencilAttachmentFormat = config.stencil_attachment_format,
			};
			const VkGraphicsPipelineCreateInfo pipeline_info{
			  .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			  .pNext               = &rendering,
			  .stageCount          = static_cast<u32>(stages.size()),
			  .pStages             = stages.data(),
			  .pVertexInputState   = &vertex_input,
			  .pInputAssemblyState = &input_assembly,
			  .pViewportState      = &viewport_state,
			  .pRasterizationState = &rasterization,
			  .pMultisampleState   = &multisample,
			  .pDepthStencilState  = &depth_stencil,
			  .pColorBlendState    = &blend,
			  .pDynamicState       = &dynamic_state,
			  .layout              = state.layout,
			  .renderPass          = VK_NULL_HANDLE,
			  .subpass             = 0,
			};

			if (
			  VkResult result = vkCreateGraphicsPipelines(device, state.cache, 1, &pipeline_info, nullptr, &state.pipeline);
			  result != VK_SUCCESS)
				return fail(std::format("Failed to create graphics pipeline: {}", string_VkResult(result)));

			return state;
		}

		[[nodiscard]] static std::expected<pipeline_state, std::string> create(
		  VkDevice device, VkFormat color_format, const graphics_pipeline_config& config)
		{
			return create(device, VK_NULL_HANDLE, VK_NULL_HANDLE, color_format, config);
		}

		static void destroy(VkDevice device, pipeline_state& state) noexcept
		{
			if (device != VK_NULL_HANDLE)
			{
				if (state.pipeline != VK_NULL_HANDLE)
					vkDestroyPipeline(device, state.pipeline, nullptr);
				if (state.layout != VK_NULL_HANDLE)
					vkDestroyPipelineLayout(device, state.layout, nullptr);
				if (state.cache != VK_NULL_HANDLE)
					vkDestroyPipelineCache(device, state.cache, nullptr);
			}
			state = {};
		}
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

	// Compiles and loads N shaders; everything loaded is released again when the set goes out of scope.
	template<std::size_t N>
	class shader_set
	{
	public:
		explicit shader_set(VkDevice device) noexcept
			: m_device{device}
		{
		}

		shader_set(const shader_set&)            = delete;
		shader_set& operator=(const shader_set&) = delete;

		~shader_set()
		{
			for (std::size_t i = 0; i < N; ++i)
				if (m_loaded[i])
					m_shaders[i].deinitialize(m_device);
		}

		[[nodiscard]] std::expected<void, std::string> load(std::size_t index, const fs::path& file)
		{
			auto spirv = compile_spirv13(file);
			if (spirv.empty())
				return std::unexpected(std::format("Failed to compile shader: {}", file.string()));

			if (auto result = m_shaders[index].load_from_memory(m_device, spirv); not result)
				return std::unexpected(result.error());

			m_loaded[index] = true;
			return {};
		}

		[[nodiscard]] std::expected<void, std::string> load_all(
		  const fs::path& directory, const std::array<std::string_view, N>& names)
		{
			for (std::size_t i = 0; i < N; ++i)
				if (auto result = load(i, directory / names[i]); not result)
					return result;

			return {};
		}

		[[nodiscard]] shader& operator[](std::size_t index) noexcept { return m_shaders[index]; }

	private:
		VkDevice              m_device{nullptr};
		std::array<shader, N> m_shaders{};
		std::array<bool, N>   m_loaded{};
	};

	// ################################################################
	// Context ########################################################

	export class context
	{
	private:
		static constexpr u32 max_texture_count = 4096;

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
		std::vector<VkImage>                            m_swapchain_images;
		std::vector<VkImageView>                        m_swapchain_image_views;
		std::array<texture_resource, max_texture_count> m_textures{};
		u32                                             m_texture_count{0};
		texture_id                                      m_white_texture{0};
		u64 m_signal_value{0};     // timeline semaphore signal value for the current frame in flight
		u64 m_timeline_counter{0}; // timeline semaphore counter for the current frame in flight
		u32 m_currentframe{0};     // frame index for the current frame in flight

		// pipelines: [0] = triangle, [1] = triangle2
		std::vector<pipeline_state> m_pipelines;
		pipeline_state              m_pipeline_2d{};
		VkDescriptorSetLayout       m_texture_descriptor_layout{VK_NULL_HANDLE};
		VkDescriptorPool            m_texture_descriptor_pool{VK_NULL_HANDLE};
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


	public:
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
			deinitialize_textures();
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

			m_clear_rgb = clear_rgb;

			const auto now    = std::chrono::steady_clock::now();
			const f32  dt     = std::chrono::duration<f32>(now - m_last_frame_time).count();
			m_last_frame_time = now;
			m_time_seconds += dt;
			update_triangle2(frame, dt);

			upload_batch(frame, batch);

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
				dbg::println("Screenshot readback memory: cached={}, coherent={}",
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

	private:
	private:
		[[nodiscard]] std::optional<u32> find_memory_type(
		  u32 type_filter, VkMemoryPropertyFlags want, VkMemoryPropertyFlags preferred = 0) const
		{
			VkPhysicalDeviceMemoryProperties mem_properties{};
			vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);

			std::optional<u32> fallback;
			for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i)
				if ((type_filter & (1 << i)) and (mem_properties.memoryTypes[i].propertyFlags & want) == want)
				{
					if (preferred != 0 and (mem_properties.memoryTypes[i].propertyFlags & preferred) == preferred)
						return i;
					if (not fallback)
						fallback = i;
				}
			return fallback;
		}

	private:
		// ############################################################
		// Buffer #####################################################

		// Creates a persistently mapped host-visible buffer. Cleans up after itself on failure.
		std::expected<void, std::string> create_buffer(
		  VkDeviceSize size, VkBufferUsageFlags usage, buffer& out, std::string_view label,
		  VkMemoryPropertyFlags required  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		  VkMemoryPropertyFlags preferred = 0)
		{

			const VkBufferCreateInfo buffer_info{
			  .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			  .size        = size,
			  .usage       = usage,
			  .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			};

			if (auto r = vkCreateBuffer(m_device, &buffer_info, nullptr, &out.handle); r != VK_SUCCESS)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("Failed to create {} buffer: {}", label, string_VkResult(r)));
			}

			VkMemoryRequirements requirements{};
			vkGetBufferMemoryRequirements(m_device, out.handle, &requirements);

			auto memory_type = find_memory_type(requirements.memoryTypeBits, required, preferred);
			if (not memory_type)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("Failed to find suitable memory type for {} buffer", label));
			}

			VkPhysicalDeviceMemoryProperties mem_properties{};
			vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties);
			out.memory_properties = mem_properties.memoryTypes[*memory_type].propertyFlags;

			const VkMemoryAllocateInfo memory_info{
			  .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			  .allocationSize  = requirements.size,
			  .memoryTypeIndex = *memory_type,
			};

			if (auto r = vkAllocateMemory(m_device, &memory_info, nullptr, &out.memory); r != VK_SUCCESS)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("Failed to allocate {} memory: {}", label, string_VkResult(r)));
			}

			if (auto r = vkBindBufferMemory(m_device, out.handle, out.memory, 0); r != VK_SUCCESS)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("Failed to bind {} memory: {}", label, string_VkResult(r)));
			}

			if (auto r = vkMapMemory(m_device, out.memory, 0, requirements.size, 0, &out.mapped); r != VK_SUCCESS)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("Failed to map {} memory: {}", label, string_VkResult(r)));
			}

			out.size = size;
			set_debug_name(
			  m_device, reinterpret_cast<u64>(out.handle), VK_OBJECT_TYPE_BUFFER,
			  std::format("buffer_alloc: {}", label).c_str());
			return {};
		}

		void destroy_buffer(buffer& buf) noexcept
		{
			if (buf.mapped)
				vkUnmapMemory(m_device, buf.memory);

			vkDestroyBuffer(m_device, buf.handle, nullptr);
			vkFreeMemory(m_device, buf.memory, nullptr);
			buf = {};
		}

		auto map_memory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, void*& out) noexcept -> VkResult
		{
			auto result = vkMapMemory(m_device, memory, offset, size, 0, &out);
			if (result != VK_SUCCESS)
				return result;
			return VK_SUCCESS;
		}

		void unmap_memory(VkDeviceMemory memory) noexcept { vkUnmapMemory(m_device, memory); }

		[[nodiscard]] std::expected<buffer, std::string> upload(
		  std::span<const std::byte> data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
		{
			buffer buf{};
			if (auto result = create_buffer(data.size(), usage, buf, "upload buffer"); not result)
				return std::unexpected(std::format("Failed to create buffer: {}", result.error()));
			if (buf.mapped)
				std::memcpy(buf.mapped, data.data(), data.size());
			else
			{
				void* mapped{nullptr};
				if (auto result = map_memory(buf.memory, 0, data.size(), mapped); result != VK_SUCCESS)
				{
					destroy_buffer(buf);
					return std::unexpected(std::format("Failed to map buffer memory: {}", string_VkResult(result)));
				}
				std::memcpy(mapped, data.data(), data.size());
				unmap_memory(buf.memory);
			}
			return buf;
		}

		[[nodiscard]] std::expected<buffer, std::string> upload(
		  std::span<const f32> data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
		{
			return upload(std::as_bytes(data), usage, properties);
		}

		[[nodiscard]]

		// ############################################################
		// Instance ###################################################
		// impl/instance.cxx

		[[nodiscard]] VkInstance         instance() const noexcept;
		std::expected<void, std::string> initialize_instance(u32 minimum_apiversion);
		void                             deinitialize_instance();

		// ############################################################
		// Debug ######################################################
		// impl/debug.cxx

		void set_debug_name([[maybe_unused]] VkDevice dev, u64 handle, VkObjectType type, std::string_view name) noexcept;

		static VKAPI_ATTR VkBool32 VKAPI_CALL static_debug_callback(
		  [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
		  [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT             message_type,
		  [[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* callback_data, [[maybe_unused]] void* userdata);

		std::expected<void, std::string> initialize_debug_functions();

		std::expected<void, std::string> initialize_debug();

		void deinitialize_debug();

		// ############################################################
		// Surface ####################################################
		// impl/surface.cxx

		[[nodiscard]] VkSurfaceFormatKHR select_surface_format() const;
		std::expected<void, std::string> initialize_surface(HWND hwnd);

		[[nodiscard]] std::expected<VkSurfaceCapabilitiesKHR, VkResult> query_surface_capabilities() const noexcept;
		std::expected<void, std::string>                                update_surface_capabilities();

		void deinitialize_surface();

		[[nodiscard]] bool can_present_win32(u32 queue_family_index) const;

		[[nodiscard]] bool surface_extent_changed() const;

		std::expected<u32, std::string> select_queue_family_index();

		// ############################################################
		// Device #####################################################
		// impl/device.cxx

		// Prefers discrete, then integrated, then the first device. Devices missing the required features or
		// API version are skipped while looking for discrete/integrated.
		std::expected<void, std::string> select_physical_device();
		void                             log_device_info() const;

		std::expected<void, std::string> create_device();
		[[nodiscard]] VkDevice           device() const noexcept;
		std::expected<void, std::string> initialize_device();
		void                             deinitialize_device();

		// ############################################################
		// Swapchain ##################################################
		// impl/swapchain.cxx

		std::expected<void, std::string> initialize_swapchain();

		std::expected<void, std::string> resize_swapchain();

		void deinitialize_swapchain();

		std::expected<void, std::string> initialize_swapchain_images();
		std::expected<void, std::string> resize_swapchain_images();
		void                             deinitialize_swapchain_images();

		[[nodiscard]] VkImage     get_swapchain_image(u32 index) const;
		[[nodiscard]] VkImageView get_swapchain_image_view(u32 index) const;

		// ############################################################
		// Swapchain images ###########################################


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

		void destroy_texture_resources() noexcept
		{
			for (u32 i = 0; i < m_texture_count; ++i)
			{
				destroy_image(m_textures[i].image);
				m_textures[i].descriptor_set = VK_NULL_HANDLE;
			}
			m_texture_count = 0;
			m_white_texture = {0};
		}

		void deinitialize_textures() { destroy_texture_resources(); }

		[[nodiscard]] const texture_resource* find_texture(texture_id id) const noexcept
		{
			if (not id or id.value >= m_texture_count)
				return nullptr;
			return &m_textures[id.value];
		}

		// ############################################################
		// Command buffers ############################################
		// impl/command_buffers.cxx

		std::expected<void, std::string> allocate_command_buffers(std::span<VkCommandBuffer> out) const;
		std::expected<void, std::string> initialize_command_buffers();
		void                             deinitialize_command_buffers();
		[[nodiscard]] VkCommandBuffer&   get_command_buffer(u32 index);

		VkResult submit(VkSemaphore wait, VkCommandBuffer cmd, VkSemaphore rendering_finished, u64 signal_value) const;
		VkResult submit_command() const;
		VkResult submit_frame(const frame_slot& frame, u32 image_index, u64 signal_value) const;

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

		// The first triangle pipeline has no vertex input. The animated triangle is emitted
		// through the regular textured 2D batch so its texture is selected by vertex ID.
		void draw_triangles(VkCommandBuffer cmd, VkBuffer vertex_buffer) const
		{
			(void)vertex_buffer;
			if (m_pipelines.size() > 0 and m_pipelines[0].pipeline != VK_NULL_HANDLE)
			{
				vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0].pipeline);
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

				  if (frame.batch_count > 0 and m_pipeline_2d.pipeline != VK_NULL_HANDLE
					  and not frame.texture_batches.empty())
				  {
					  const std::array<f32, 2> scale{
						2.0f / static_cast<f32>(extent.width), 2.0f / static_cast<f32>(extent.height)};
					  const VkDeviceSize offset{0};

					  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_2d.pipeline);
					  vkCmdPushConstants(
						cmd, m_pipeline_2d.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(scale), scale.data());
					  vkCmdBindVertexBuffers(cmd, 0, 1, &frame.batch.handle, &offset);
					  for (const auto& batch : frame.texture_batches)
					  {
						  if (batch.vertex_count == 0)
							  continue;
						  const auto* texture = find_texture(batch.id);
						  if (texture == nullptr or texture->descriptor_set == VK_NULL_HANDLE)
							  continue;
						  vkCmdBindDescriptorSets(
							cmd,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							m_pipeline_2d.layout,
							0,
							1,
							&texture->descriptor_set,
							0,
							nullptr);
						  vkCmdDraw(cmd, batch.vertex_count, 1, batch.first_vertex, 0);
					  }
				  }
			  });
		}

		// ############################################################
		// Semaphores #################################################
		// impl/synchronization.cxx

		VkSemaphore create_semaphore(VkSemaphoreType type = VK_SEMAPHORE_TYPE_BINARY) const;
		void        assert_timeline(VkSemaphore semaphore) const;

		void              wait_for_timeline_semaphore(VkSemaphore semaphore, u64 value, u64 timeout = UINT64_MAX) const;
		[[nodiscard]] u64 get_timeline_semaphore_value(VkSemaphore semaphore) const;
		void              signal_timeline_semaphore(VkSemaphore semaphore, u64 value) const;

		// ############################################################
		// Synchronization ############################################
		// impl/synchronization.cxx

		std::expected<void, std::string> initialize_synchronization();
		void                             destroy_synchronization();

		std::expected<void, std::string> resize_synchronization();
		void                             deinitialize_synchronization();

		// ############################################################
		// Pipelines ##################################################
		// impl/pipeline.cxx

		std::expected<pipeline_state, std::string> create_pipeline(
		  VkShaderModule vertex, VkShaderModule fragment, std::span<const VkVertexInputBindingDescription> bindings = {},
		  std::span<const VkVertexInputAttributeDescription> attributes = {}, const pipeline_desc& desc = {}) const;

		void destroy_pipeline(pipeline_state& state) const noexcept;
		void deinitialize_pipelines();

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

		void update_texture_descriptor(texture_resource& texture) const
		{
			if (texture.descriptor_set == VK_NULL_HANDLE or texture.image.view == VK_NULL_HANDLE)
				return;

			const VkDescriptorImageInfo image_info{
			  .sampler     = texture.image.sampler,
			  .imageView   = texture.image.view,
			  .imageLayout = m_unified_image_layouts ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};
			const VkWriteDescriptorSet write{
			  .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			  .dstSet          = texture.descriptor_set,
			  .dstBinding      = 0,
			  .descriptorCount = 1,
			  .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			  .pImageInfo      = &image_info,
			};
			vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
		}

		[[nodiscard]] std::expected<texture_id, std::string> register_texture(image_resource& image)
		{
			if (image.view == VK_NULL_HANDLE or image.sampler == VK_NULL_HANDLE)
				return std::unexpected("Texture image view and sampler must be valid");
			if (m_texture_descriptor_pool == VK_NULL_HANDLE or m_texture_descriptor_layout == VK_NULL_HANDLE)
				return std::unexpected("Texture descriptors must be initialized before loading textures");
			if (m_texture_count >= max_texture_count)
				return std::unexpected("Texture table is full");

			const VkDescriptorSetAllocateInfo allocate_info{
			  .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			  .descriptorPool     = m_texture_descriptor_pool,
			  .descriptorSetCount = 1,
			  .pSetLayouts        = &m_texture_descriptor_layout,
			};
			texture_resource texture{};
			if (VkResult result = vkAllocateDescriptorSets(m_device, &allocate_info, &texture.descriptor_set);
				result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to allocate texture descriptor set: {}", string_VkResult(result)));

			const texture_id id{m_texture_count};
			texture.image               = std::exchange(image, image_resource{});
			m_textures[m_texture_count] = texture;
			++m_texture_count;
			update_texture_descriptor(m_textures[id.value]);
			return id;
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

			const VkDescriptorPoolSize       pool_size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_texture_count};
			const VkDescriptorPoolCreateInfo pool_info{
			  .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			  .maxSets       = max_texture_count,
			  .poolSizeCount = 1,
			  .pPoolSizes    = &pool_size,
			};
			if (VkResult result = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_texture_descriptor_pool);
				result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create texture descriptor pool: {}", string_VkResult(result)));

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
			image_rgba white_image{1, 1};
			white_image.at(0, 0) = rgba{255, 255, 255, 255};
			auto white_texture   = upload_image(white_image);
			if (not white_texture)
				return std::unexpected(std::format("Failed to create white fallback texture: {}", white_texture.error()));
			m_white_texture = *white_texture;

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
			deinitialize_pipelines();
			destroy_texture_resources();
			if (m_texture_descriptor_pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_device, m_texture_descriptor_pool, nullptr);
			m_texture_descriptor_pool = VK_NULL_HANDLE;
			if (m_texture_descriptor_layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(m_device, m_texture_descriptor_layout, nullptr);
			m_texture_descriptor_layout = VK_NULL_HANDLE;
			m_white_texture             = {0};
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
			count -= count % 3;
			if (count != 0)
				std::memcpy(frame.batch.mapped, batch.data(), count * sizeof(vertex2d));
			frame.batch_count = count;
			frame.texture_batches.clear();
			for (u32 first = 0; first < count;)
			{
				const texture_id id{batch[first].texture_id};
				u32              end = first + 3;
				while (end < count and batch[end].texture_id == id.value)
					end += 3;
				frame.texture_batches.push_back({id, first, end - first});
				first = end;
			}
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

			const std::array<VkVertexInputAttributeDescription, 3> attributes{{
			  {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex2d, v)},
			  {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(vertex2d, uv)},
			  {.location = 2, .binding = 0, .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(vertex2d, rgba)},
			}};

			const pipeline_desc desc{
			  .topology              = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			  .cull_mode             = VK_CULL_MODE_NONE,
			  .push_stages           = VK_SHADER_STAGE_VERTEX_BIT,
			  .push_size             = sizeof(f32) * 2,
			  .descriptor_set_layout = m_texture_descriptor_layout,
			  .alpha_blend           = true,
			};

			auto pipeline = create_pipeline(shaders[0], shaders[1], bindings, attributes, desc);
			if (not pipeline)
				return std::unexpected(std::move(pipeline.error()));

			m_pipeline_2d = std::move(*pipeline);
			return {};
		}

		void deinitialize_renderer2d() { destroy_pipeline(m_pipeline_2d); }

		// ############################################################
		// image upload ###############################################

	public:
		std::expected<texture_id, std::string> upload_image(const image_rgba& image)
		{
			if (m_device == VK_NULL_HANDLE or m_graphics_queue == VK_NULL_HANDLE or m_command_pool == VK_NULL_HANDLE)
				return std::unexpected("Vulkan device, graphics queue, and command pool must be initialized");
			if (m_texture_descriptor_pool == VK_NULL_HANDLE or m_texture_descriptor_layout == VK_NULL_HANDLE)
				return std::unexpected("Texture descriptors must be initialized before uploading textures");

			const auto pixels = image.raw_data();
			if (pixels.empty())
				return std::unexpected("Image data must not be empty");

			const u32 width  = image.width();
			const u32 height = image.height();
			if (width == 0 or height == 0)
				return std::unexpected("Image dimensions must not be zero");

			const std::size_t expected_size = std::size_t{width} * height * 4;
			if (pixels.size_bytes() != expected_size)
				return std::unexpected("Image data size does not match width * height * 4");

			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(m_physical_device, &properties);
			if (width > properties.limits.maxImageDimension2D or height > properties.limits.maxImageDimension2D)
				return std::unexpected("Image dimensions exceed the device's maximum 2D image dimension");

			buffer          staging{};
			image_resource  uploaded{};
			VkCommandBuffer command_buffer{VK_NULL_HANDLE};
			VkFence         fence{VK_NULL_HANDLE};

			scope_exit guard{[&]
							 {
								 if (fence != VK_NULL_HANDLE)
									 vkDestroyFence(m_device, fence, nullptr);
								 if (command_buffer != VK_NULL_HANDLE)
									 vkFreeCommandBuffers(m_device, m_command_pool, 1, &command_buffer);
								 if (staging.handle != VK_NULL_HANDLE)
									 destroy_buffer(staging);
								 destroy_image(uploaded);
							 }};

			// Staging buffer
			if (auto result = create_buffer(pixels.size_bytes(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging, "image staging");
				not result)
				return std::unexpected(std::move(result.error()));
			std::memcpy(staging.mapped, pixels.data(), pixels.size_bytes());
			// If create_buffer can return non-coherent memory, add vkFlushMappedMemoryRanges here.

			// Device-local image
			const VkImageCreateInfo image_info{
			  .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			  .imageType     = VK_IMAGE_TYPE_2D,
			  .format        = VK_FORMAT_R8G8B8A8_UNORM,
			  .extent        = {width, height, 1},
			  .mipLevels     = 1,
			  .arrayLayers   = 1,
			  .samples       = VK_SAMPLE_COUNT_1_BIT,
			  .tiling        = VK_IMAGE_TILING_OPTIMAL,
			  .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			  .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
			  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};
			if (VkResult result = vkCreateImage(m_device, &image_info, nullptr, &uploaded.handle); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create image: {}", string_VkResult(result)));

			VkMemoryRequirements requirements{};
			vkGetImageMemoryRequirements(m_device, uploaded.handle, &requirements);
			auto memory_type = find_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (not memory_type)
				return std::unexpected("Failed to find suitable device-local memory for image");

			const VkMemoryAllocateInfo memory_info{
			  .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			  .allocationSize  = requirements.size,
			  .memoryTypeIndex = *memory_type,
			};
			if (VkResult result = vkAllocateMemory(m_device, &memory_info, nullptr, &uploaded.memory); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to allocate image memory: {}", string_VkResult(result)));
			if (VkResult result = vkBindImageMemory(m_device, uploaded.handle, uploaded.memory, 0); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to bind image memory: {}", string_VkResult(result)));

			const VkImageViewCreateInfo view_info{
			  .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			  .image            = uploaded.handle,
			  .viewType         = VK_IMAGE_VIEW_TYPE_2D,
			  .format           = VK_FORMAT_R8G8B8A8_UNORM,
			  .subresourceRange = color_range,
			};
			if (VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &uploaded.view); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create uploaded image view: {}", string_VkResult(result)));

			const VkSamplerCreateInfo sampler_info{
			  .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			  .magFilter               = VK_FILTER_LINEAR,
			  .minFilter               = VK_FILTER_LINEAR,
			  .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			  .addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			  .addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			  .addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			  .mipLodBias              = 0.0f,
			  .anisotropyEnable        = VK_FALSE,
			  .maxAnisotropy           = 1.0f,
			  .compareEnable           = VK_FALSE,
			  .minLod                  = 0.0f,
			  .maxLod                  = 0.0f,
			  .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			  .unnormalizedCoordinates = VK_FALSE,
			};
			if (VkResult result = vkCreateSampler(m_device, &sampler_info, nullptr, &uploaded.sampler); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create uploaded image sampler: {}", string_VkResult(result)));

			// Command buffer
			const VkCommandBufferAllocateInfo allocate_info{
			  .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			  .commandPool        = m_command_pool,
			  .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			  .commandBufferCount = 1,
			};
			if (VkResult result = vkAllocateCommandBuffers(m_device, &allocate_info, &command_buffer); result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to allocate image upload command buffer: {}", string_VkResult(result)));

			const VkCommandBufferBeginInfo begin_info{
			  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			};
			if (VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info); result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to begin image upload command buffer: {}", string_VkResult(result)));

			const VkImageLayout upload_layout
			  = m_unified_image_layouts ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			const VkImageLayout sampled_layout
			  = m_unified_image_layouts ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			transition_image(
			  command_buffer,
			  uploaded.handle,
			  VK_PIPELINE_STAGE_2_NONE,
			  VK_ACCESS_2_NONE,
			  VK_PIPELINE_STAGE_2_COPY_BIT,
			  VK_ACCESS_2_TRANSFER_WRITE_BIT,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  upload_layout);

			const VkBufferImageCopy copy_region{
			  .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
			  .imageExtent      = {width, height, 1},
			};
			vkCmdCopyBufferToImage(command_buffer, staging.handle, uploaded.handle, upload_layout, 1, &copy_region);
			transition_image(
			  command_buffer,
			  uploaded.handle,
			  VK_PIPELINE_STAGE_2_COPY_BIT,
			  VK_ACCESS_2_TRANSFER_WRITE_BIT,
			  VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			  VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			  upload_layout,
			  sampled_layout);

			if (VkResult result = vkEndCommandBuffer(command_buffer); result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to end image upload command buffer: {}", string_VkResult(result)));

			// Submit with fence
			const VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
			if (VkResult result = vkCreateFence(m_device, &fence_info, nullptr, &fence); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create upload fence: {}", string_VkResult(result)));

			const VkCommandBufferSubmitInfo command_buffer_info{
			  .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			  .commandBuffer = command_buffer,
			};
			const VkSubmitInfo2 submit_info{
			  .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			  .commandBufferInfoCount = 1,
			  .pCommandBufferInfos    = &command_buffer_info,
			};
			if (VkResult result = vkQueueSubmit2(m_graphics_queue, 1, &submit_info, fence); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to submit image upload: {}", string_VkResult(result)));

			if (VkResult result = vkWaitForFences(m_device, 1, &fence, VK_TRUE, std::numeric_limits<u64>::max());
				result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to wait for image upload: {}", string_VkResult(result)));

			if (VkResult result = vkQueueWaitIdle(m_graphics_queue); result != VK_SUCCESS)
				return std::unexpected(std::format("Failed to wait for graphics queue: {}", string_VkResult(result)));

			// Success: tear down upload temporaries and register the uploaded image.
			vkDestroyFence(m_device, fence, nullptr);
			fence = VK_NULL_HANDLE;
			vkFreeCommandBuffers(m_device, m_command_pool, 1, &command_buffer);
			command_buffer = VK_NULL_HANDLE;
			destroy_buffer(staging);
			staging      = {};
			auto texture = register_texture(uploaded);
			if (not texture)
				return std::unexpected(std::move(texture.error()));
			guard.release();
			return texture;
		}

		[[nodiscard]] std::expected<texture_id, std::string> load_texture(const fs::path& path)
		{
			auto image = deckard::image::load_png(path);
			if (not image)
				return std::unexpected(image.error());
			return upload_image(*image);
		}

		[[nodiscard]] texture_id default_texture() const noexcept { return m_white_texture; }
	};

} // namespace deckard::vulkan
