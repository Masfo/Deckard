module;
#include <windows.h>

#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan;
export import deckard.vulkan.common;
export import deckard.vulkan.light;
export import deckard.mesh;
export import :shaders;

import std;
import deckard.as;
import deckard.types;
import deckard.vec;
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


	export struct buffer
	{
		VkBuffer              handle{};
		VkDeviceMemory        memory{};
		VkDeviceSize          memory_offset{};
		VkDeviceSize          allocation_size{};
		u32                   allocation_block{std::numeric_limits<u32>::max()};
		VkDeviceSize          size{};
		VkMemoryPropertyFlags memory_properties{};
		void*                 mapped{}; // non-null only for HOST_VISIBLE memory
	};

	struct image_resource
	{
		VkImage        handle{};
		VkDeviceMemory memory{};
		VkDeviceSize  memory_offset{};
		VkDeviceSize  allocation_size{};
		u32           allocation_block{std::numeric_limits<u32>::max()};
		VkImageView    view{};
		VkSampler      sampler{};
	};

	struct memory_range
	{
		VkDeviceSize offset{};
		VkDeviceSize size{};
	};

	struct memory_block
	{
		VkDeviceMemory        memory{};
		VkDeviceSize          size{};
		u32                   memory_type{};
		VkMemoryPropertyFlags properties{};
		bool                  image_memory{false};
		void*                 mapped{};
		std::vector<memory_range> free_ranges;
	};

	export struct texture_id
	{
		u32 value{std::numeric_limits<u32>::max()};

		[[nodiscard]] explicit operator bool() const noexcept { return value != std::numeric_limits<u32>::max(); }

		[[nodiscard]] friend bool operator==(texture_id, texture_id) noexcept = default;
	};

	export struct mesh_id
	{
		u32 value{std::numeric_limits<u32>::max()};

		[[nodiscard]] explicit operator bool() const noexcept { return value != std::numeric_limits<u32>::max(); }

		[[nodiscard]] friend bool operator==(mesh_id, mesh_id) noexcept = default;
	};

	// One 3D draw submitted for the current frame.
	export struct mesh_draw
	{
		mesh_id             id{};
		texture_id          texture{};
		std::array<f32, 16> model{};  // column-major model transform
		std::array<f32, 12> normal{}; // mat3 inverse-transpose of the model matrix, each column padded to 16 bytes
		std::array<f32, 4>  color{1.0f, 1.0f, 1.0f, 0.0f}; // RGB tint, W emissive strength
	};

	struct texture_resource
	{
		image_resource  image{};
		VkDescriptorSet descriptor_set{VK_NULL_HANDLE};
	};

	struct mesh_resource
	{
		buffer vertex{};
		buffer index{};
		u32    index_count{0};
	};

	// Matches the push constant block in mesh.vert / mesh.frag.
	struct mesh_push_constant
	{
		std::array<f32, 16> model;
		std::array<f32, 12> normal; // mat3 in std430: three columns, each padded to 16 bytes
		std::array<f32, 4>  color;  // RGB tint, W emissive strength
	};

	static_assert(sizeof(mesh_push_constant) == 128);

	struct scene_uniform
	{
		std::array<f32, 16> view_projection;
		std::array<f32, 4>  light_position_intensity;
		std::array<f32, 4>  light_color_radius;
	};

	static_assert(sizeof(scene_uniform) == 96);

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
		buffer                     batch{};  // 2D batch vertices
		buffer                     scene{};  // view-projection and point light
		VkDescriptorSet            scene_descriptor_set{VK_NULL_HANDLE};
		u32                        batch_count{0};
		std::vector<texture_batch> texture_batches;
		u64                        timeline_value{0};
	};


	static_assert(sizeof(vec::vec2) == 2 * sizeof(f32));
	static_assert(std::is_standard_layout_v<vertex2d>);           // offsetof needs this

	export inline constexpr u32 max_batch_vertices = 3u * 32768u; // multiple of 3, ~1.1 MiB per frame slot

	// Format of the depth attachment that is bound to every dynamic rendering pass.
	export inline constexpr VkFormat default_depth_format = VK_FORMAT_D32_SFLOAT;

	export struct pipeline_desc
	{
		VkPrimitiveTopology   topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP};
		VkCullModeFlags       cull_mode{VK_CULL_MODE_BACK_BIT};
		VkShaderStageFlags    push_stages{0};
		u32                   push_size{0};
		VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
		VkDescriptorSetLayout additional_descriptor_set_layout{VK_NULL_HANDLE};
		bool                  alpha_blend{false};
		bool                  depth_test{false};
		bool                  depth_write{false};
		// Test/write become dynamic state, so one pipeline can draw both depth-tested and overlay
		// geometry without a shader variant.
		bool                  dynamic_depth{false};
		VkCompareOp           depth_compare{VK_COMPARE_OP_LESS};
		// Dynamic rendering requires the pipeline to declare the attachment formats it is used with, even
		// when the pipeline does not read or write depth.
		VkFormat              depth_format{default_depth_format};
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
	  VkImageLayout new_layout, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT) noexcept
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
		  .subresourceRange    = {aspect, 0, 1, 0, 1},
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
		static constexpr u32 max_mesh_count    = 4096;

		VkInstance               m_instance{nullptr};
		VkDevice                 m_device{nullptr};
		VkPhysicalDevice         m_physical_device{nullptr};
		bool                     m_unified_image_layouts{false};
		buffer*                  m_screenshot_buffer{nullptr};
		buffer                   m_screenshot_readback{};
		std::vector<memory_block> m_memory_blocks{};
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
		std::vector<mesh_resource>                      m_meshes{};
		u64 m_signal_value{0};     // timeline semaphore signal value for the current frame in flight
		u64 m_timeline_counter{0}; // timeline semaphore counter for the current frame in flight
		u32 m_currentframe{0};     // frame index for the current frame in flight

		// pipelines: [0] = triangle
		std::vector<pipeline_state> m_pipelines;
		pipeline_state              m_pipeline_2d{};
		pipeline_state              m_pipeline_mesh{};
		std::vector<image_resource> m_depth_images{};
		VkFormat                    m_depth_format{default_depth_format};
		VkDescriptorSetLayout       m_texture_descriptor_layout{VK_NULL_HANDLE};
		VkDescriptorPool            m_texture_descriptor_pool{VK_NULL_HANDLE};
		VkDescriptorSetLayout       m_scene_descriptor_layout{VK_NULL_HANDLE};
		VkDescriptorPool            m_scene_descriptor_pool{VK_NULL_HANDLE};
		light                        m_scene_light{
		  .position = {-2.0f, 4.0f, 3.0f}, .color = {1.0f, 0.95f, 0.85f}, .intensity = 2.5f, .radius = 10.0f};
		std::array<f32, 3>          m_clear_rgb{default_clear_rgb};

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

		void set_light(const light& source) noexcept { m_scene_light = source; }

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
				  .and_then([this] { return initialize_depth(); })
				  .and_then([this] { return initialize_command_buffers(); })
				  .and_then([this] { return initialize_frames(); })
				  .and_then([this] { return initialize_synchronization(); })
				  .and_then([this] { return initialize_triangle_renderer(); })
				  .and_then([this] { return initialize_renderer2d(); })
				  .and_then([this] { return initialize_mesh_renderer(); })
				  .and_then([this] { return record_clear_commands(); });


			return result;
		}

		void deinitialize()
		{
			if (m_device != VK_NULL_HANDLE)
				vkDeviceWaitIdle(m_device);

			deinitialize_synchronization();
			deinitialize_renderer2d();
			deinitialize_mesh_renderer();
			deinitialize_meshes();
			deinitialize_frames();
			deinitialize_triangle_renderer();
			deinitialize_command_buffers();
			deinitialize_swapchain_images();
			deinitialize_depth();
			deinitialize_textures();
			if (m_screenshot_readback.handle != VK_NULL_HANDLE)
				destroy_buffer(m_screenshot_readback);
			deinitialize_memory_blocks();
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
								.and_then([this] { return resize_depth(); })
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

		[[nodiscard]] const light& get_light() const noexcept { return m_scene_light; }

		bool draw(
		  std::span<const vertex2d> batch = {}, std::span<const mesh_draw> meshes = {},
		  std::array<f32, 3> clear_rgb = default_clear_rgb,
		  std::array<f32, 16> view_projection = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1})
		{

			if (m_swapchain_images.empty())
				return true;

			auto& frame = m_frames[m_currentframe];

			// One host wait covers command buffer, vertex buffer and image_available reuse.
			if (frame.timeline_value != 0)
				wait_for_timeline_semaphore(m_in_flight_timeline, frame.timeline_value);

			m_clear_rgb = clear_rgb;

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
			if (auto recorded = record_frame(frame, image_index, meshes, view_projection); not recorded)
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
		  std::span<const vertex2d> batch = {}, std::span<const mesh_draw> meshes = {},
		  std::array<f32, 3> clear_rgb = default_clear_rgb,
		  std::array<f32, 16> view_projection = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1})
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
			if (not draw(batch, meshes, clear_rgb, view_projection))
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

		struct memory_allocation
		{
			VkDeviceMemory        memory{};
			VkDeviceSize          offset{};
			VkDeviceSize          size{};
			VkMemoryPropertyFlags properties{};
			void*                 mapped{};
			u32                   block{};
		};

		[[nodiscard]] static std::optional<VkDeviceSize> take_memory_range(
		  memory_block& block, VkDeviceSize size, VkDeviceSize alignment)
		{
			for (std::size_t i = 0; i < block.free_ranges.size(); ++i)
			{
				const memory_range range = block.free_ranges[i];
				const VkDeviceSize offset = (range.offset + alignment - 1) & ~(alignment - 1);
				if (offset < range.offset or offset - range.offset > range.size or size > range.size - (offset - range.offset))
					continue;

				block.free_ranges.erase(block.free_ranges.begin() + i);
				if (offset > range.offset)
					block.free_ranges.push_back({range.offset, offset - range.offset});
				const VkDeviceSize end = offset + size;
				if (end < range.offset + range.size)
					block.free_ranges.push_back({end, range.offset + range.size - end});
				return offset;
			}
			return std::nullopt;
		}

		[[nodiscard]] std::expected<memory_allocation, std::string> allocate_memory(
		  VkDeviceSize size, VkDeviceSize alignment, u32 type_filter, VkMemoryPropertyFlags required,
		  VkMemoryPropertyFlags preferred, bool image_memory, std::string_view label)
		{
			const auto memory_type = find_memory_type(type_filter, required, preferred);
			if (not memory_type)
				return std::unexpected(std::format("Failed to find suitable memory type for {}", label));

			VkPhysicalDeviceMemoryProperties memory_properties{};
			vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);
			const VkMemoryPropertyFlags properties = memory_properties.memoryTypes[*memory_type].propertyFlags;

			for (u32 i = 0; i < m_memory_blocks.size(); ++i)
			{
				auto& block = m_memory_blocks[i];
				if (block.memory_type != *memory_type or block.image_memory != image_memory)
					continue;
				if (auto offset = take_memory_range(block, size, alignment))
					return memory_allocation{
					  block.memory, *offset, size, block.properties,
					  block.mapped ? static_cast<std::byte*>(block.mapped) + *offset : nullptr, i};
			}

			// Keep small resources in reusable blocks; image and buffer pools stay separate for
			// bufferImageGranularity requirements.
			constexpr VkDeviceSize small_allocation_threshold = 1024 * 1024;
			const VkDeviceSize    block_size = std::max(size, small_allocation_threshold);
			const VkMemoryAllocateInfo allocate_info{
			  .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			  .allocationSize  = block_size,
			  .memoryTypeIndex = *memory_type,
			};

			memory_block block{
			  .size         = block_size,
			  .memory_type  = *memory_type,
			  .properties   = properties,
			  .image_memory = image_memory,
			  .free_ranges  = {{0, block_size}},
			};
			if (VkResult result = vkAllocateMemory(m_device, &allocate_info, nullptr, &block.memory); result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to allocate {} memory block: {}", label, string_VkResult(result)));

			if (not image_memory and (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
				if (VkResult result = vkMapMemory(m_device, block.memory, 0, VK_WHOLE_SIZE, 0, &block.mapped);
					result != VK_SUCCESS)
				{
					vkFreeMemory(m_device, block.memory, nullptr);
					return std::unexpected(
					  std::format("Failed to map {} memory block: {}", label, string_VkResult(result)));
				}

			m_memory_blocks.push_back(std::move(block));
			const u32 block_index = static_cast<u32>(m_memory_blocks.size() - 1);
			auto&    stored_block = m_memory_blocks.back();
			const auto offset = take_memory_range(stored_block, size, alignment);
			assert::check(offset.has_value(), "New Vulkan memory block could not satisfy its first allocation");
			return memory_allocation{
			  stored_block.memory, *offset, size, stored_block.properties,
			  stored_block.mapped ? static_cast<std::byte*>(stored_block.mapped) + *offset : nullptr, block_index};
		}

		void release_memory(u32 block_index, VkDeviceSize offset, VkDeviceSize size) noexcept
		{
			if (block_index >= m_memory_blocks.size() or size == 0)
				return;

			auto& block = m_memory_blocks[block_index];
			block.free_ranges.push_back({offset, size});
			std::ranges::sort(block.free_ranges, {}, &memory_range::offset);

			std::vector<memory_range> merged;
			merged.reserve(block.free_ranges.size());
			for (const auto& range : block.free_ranges)
			{
				if (not merged.empty() and merged.back().offset + merged.back().size >= range.offset)
					merged.back().size = std::max(
					  merged.back().offset + merged.back().size, range.offset + range.size) - merged.back().offset;
				else
					merged.push_back(range);
			}
			block.free_ranges = std::move(merged);
		}

		void deinitialize_memory_blocks() noexcept
		{
			for (auto& block : m_memory_blocks)
			{
				if (block.mapped != nullptr)
					vkUnmapMemory(m_device, block.memory);
				if (block.memory != VK_NULL_HANDLE)
					vkFreeMemory(m_device, block.memory, nullptr);
			}
			m_memory_blocks.clear();
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

			auto allocation = allocate_memory(
			  requirements.size, requirements.alignment, requirements.memoryTypeBits, required, preferred, false, label);
			if (not allocation)
			{
				destroy_buffer(out);
				return std::unexpected(std::move(allocation.error()));
			}

			out.memory            = allocation->memory;
			out.memory_offset     = allocation->offset;
			out.allocation_size   = allocation->size;
			out.allocation_block  = allocation->block;
			out.memory_properties = allocation->properties;
			out.mapped            = allocation->mapped;

			if (auto r = vkBindBufferMemory(m_device, out.handle, out.memory, out.memory_offset); r != VK_SUCCESS)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("Failed to bind {} memory: {}", label, string_VkResult(r)));
			}

			if ((required & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 and out.mapped == nullptr)
			{
				destroy_buffer(out);
				return std::unexpected(std::format("{} buffer memory was not mapped", label));
			}

			out.size = size;
			set_debug_name(
			  m_device, reinterpret_cast<u64>(out.handle), VK_OBJECT_TYPE_BUFFER,
			  std::format("buffer_alloc: {}", label).c_str());
			return {};
		}

		void destroy_buffer(buffer& buf) noexcept
		{
			if (buf.handle != VK_NULL_HANDLE)
				vkDestroyBuffer(m_device, buf.handle, nullptr);
			release_memory(buf.allocation_block, buf.memory_offset, buf.allocation_size);
			buf = {};
		}

		[[nodiscard]] std::expected<buffer, std::string> upload(
		  std::span<const std::byte> data, VkBufferUsageFlags usage, [[maybe_unused]] VkMemoryPropertyFlags properties)
		{
			buffer buf{};
			if (auto result = create_buffer(data.size(), usage, buf, "upload buffer"); not result)
				return std::unexpected(std::format("Failed to create buffer: {}", result.error()));
			if (buf.mapped == nullptr)
			{
				destroy_buffer(buf);
				return std::unexpected("Upload buffer allocation is not host-visible");
			}
			std::memcpy(buf.mapped, data.data(), data.size());
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


		void destroy_image(image_resource& image) noexcept
		{
			if (image.sampler != VK_NULL_HANDLE)
				vkDestroySampler(m_device, image.sampler, nullptr);
			if (image.view != VK_NULL_HANDLE)
				vkDestroyImageView(m_device, image.view, nullptr);
			if (image.handle != VK_NULL_HANDLE)
				vkDestroyImage(m_device, image.handle, nullptr);
			release_memory(image.allocation_block, image.memory_offset, image.allocation_size);
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

		[[nodiscard]] const mesh_resource* find_mesh(mesh_id id) const noexcept
		{
			if (not id or id.value >= m_meshes.size())
				return nullptr;
			return &m_meshes[id.value];
		}

		void deinitialize_meshes() noexcept
		{
			if (m_device == VK_NULL_HANDLE)
				return;

			for (auto& mesh : m_meshes)
			{
				destroy_buffer(mesh.vertex);
				destroy_buffer(mesh.index);
			}
			m_meshes.clear();
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

			image_resource* depth = image_index < m_depth_images.size() ? &m_depth_images[image_index] : nullptr;
			const bool has_depth = depth != nullptr and depth->handle != VK_NULL_HANDLE and depth->view != VK_NULL_HANDLE;

			VkRenderingAttachmentInfo depth_attachment{
			  .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			  .imageView   = has_depth ? depth->view : VK_NULL_HANDLE,
			  .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			  .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
			  .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
			  .clearValue  = {.depthStencil = {1.0f, 0}},
			};

			if (has_depth)
			{
				// Contents are always cleared, so discarding the previous frame's layout is fine.
				transition_image(
				  cmd,
				  depth->handle,
				  VK_PIPELINE_STAGE_2_NONE,
				  VK_ACCESS_2_NONE,
				  VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
				  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				  VK_IMAGE_LAYOUT_UNDEFINED,
				  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				  VK_IMAGE_ASPECT_DEPTH_BIT);
			}

			const VkRect2D        render_area{{0, 0}, extent};
			const VkRenderingInfo render_info{
			  .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
			  .renderArea           = render_area,
			  .layerCount           = 1,
			  .colorAttachmentCount = 1,
			  .pColorAttachments    = &color_attachment,
			  .pDepthAttachment     = has_depth ? &depth_attachment : nullptr,
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
		void draw_triangles(VkCommandBuffer cmd) const
		{
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
					  [this](VkCommandBuffer cmd) { draw_triangles(cmd); });
					not result)
					return result;

			return {};
		}

		std::expected<void, std::string> record_frame(
		  const frame_slot& frame, u32 image_index, std::span<const mesh_draw> meshes,
		  const std::array<f32, 16>& view_projection)
		{
			const VkExtent2D extent = m_swapchain_extent;
			const scene_uniform scene{
			  view_projection,
			  {m_scene_light.position.x, m_scene_light.position.y, m_scene_light.position.z, m_scene_light.intensity},
			  {m_scene_light.color.x, m_scene_light.color.y, m_scene_light.color.z, m_scene_light.radius},
			};
			std::memcpy(frame.scene.mapped, &scene, sizeof(scene));

			return record_commands(
			  frame.cmd,
			  VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, // re-recorded every frame
			  image_index,
			  extent,
			  m_clear_rgb,
			  [&](VkCommandBuffer cmd)
			  {
				  draw_triangles(cmd);

				  if (not meshes.empty() and m_pipeline_mesh.pipeline != VK_NULL_HANDLE)
				  {
					  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_mesh.pipeline);
					  vkCmdBindDescriptorSets(
						cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						m_pipeline_mesh.layout,
						1,
						1,
						&frame.scene_descriptor_set,
						0,
						nullptr);

					  for (const auto& item : meshes)
					  {
						  const auto* resource = find_mesh(item.id);
						  if (resource == nullptr or resource->index_count == 0)
							  continue;

						  // Emissive markers skip depth testing so they stay visible behind geometry.
						  const VkBool32 depth = item.color[3] > 0.0f ? VK_FALSE : VK_TRUE;
						  vkCmdSetDepthTestEnable(cmd, depth);
						  vkCmdSetDepthWriteEnable(cmd, depth);

						  const VkDeviceSize offset{0};
						  vkCmdBindVertexBuffers(cmd, 0, 1, &resource->vertex.handle, &offset);
						  vkCmdBindIndexBuffer(cmd, resource->index.handle, 0, VK_INDEX_TYPE_UINT32);

						  const texture_id texture_id = item.texture ? item.texture : m_white_texture;
						  const auto* texture = find_texture(texture_id);
						  if (texture == nullptr or texture->descriptor_set == VK_NULL_HANDLE)
							  continue;
						  vkCmdBindDescriptorSets(
							cmd,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							m_pipeline_mesh.layout,
							0,
							1,
							&texture->descriptor_set,
							0,
							nullptr);

						  const mesh_push_constant push{item.model, item.normal, item.color};
						  vkCmdPushConstants(
							cmd,
							m_pipeline_mesh.layout,
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							0,
							sizeof(push),
							&push);

						  vkCmdDrawIndexed(cmd, resource->index_count, 1, 0, 0, 0);
					  }
				  }

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
			shader_set<2> shaders{m_device};
			if (auto result = shaders.load_all(
				  fs::current_path() / "shaders", {"triangle.vert", "triangle.frag"});
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

			// The untextured triangle has no vertex input; it is positioned from gl_VertexIndex.
			return add_pipeline(shaders[0], shaders[1]);
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
				  sizeof(vertex2d) * max_batch_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, frame.batch, "frame batch");
				if (result)
					result = create_buffer(
					  sizeof(scene_uniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, frame.scene, "scene uniform");

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
				destroy_buffer(frame.batch);
				destroy_buffer(frame.scene);

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
		// Depth buffer ###############################################

		std::expected<void, std::string> initialize_depth()
		{
			if (m_device == VK_NULL_HANDLE)
				return std::unexpected("Vulkan device must be created before initializing the depth buffer");
			if (m_swapchain_extent.width == 0 or m_swapchain_extent.height == 0)
				return std::unexpected("Swapchain extent must be non-zero before initializing the depth buffer");

			deinitialize_depth();
			m_depth_images.resize(m_swapchain_images.size());

			const VkImageCreateInfo image_info{
			  .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			  .imageType     = VK_IMAGE_TYPE_2D,
			  .format        = m_depth_format,
			  .extent        = {m_swapchain_extent.width, m_swapchain_extent.height, 1},
			  .mipLevels     = 1,
			  .arrayLayers   = 1,
			  .samples       = VK_SAMPLE_COUNT_1_BIT,
			  .tiling        = VK_IMAGE_TILING_OPTIMAL,
			  .usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			  .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
			  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};
			for (auto& depth : m_depth_images)
			{
				if (VkResult result = vkCreateImage(m_device, &image_info, nullptr, &depth.handle); result != VK_SUCCESS)
				{
					deinitialize_depth();
					return std::unexpected(std::format("Failed to create depth image: {}", string_VkResult(result)));
				}

				VkMemoryRequirements requirements{};
				vkGetImageMemoryRequirements(m_device, depth.handle, &requirements);
				auto allocation = allocate_memory(
				  requirements.size, requirements.alignment, requirements.memoryTypeBits,
				  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, true, "depth image");
				if (not allocation)
				{
					deinitialize_depth();
					return std::unexpected(std::move(allocation.error()));
				}

				depth.memory           = allocation->memory;
				depth.memory_offset    = allocation->offset;
				depth.allocation_size  = allocation->size;
				depth.allocation_block = allocation->block;
				if (VkResult result = vkBindImageMemory(m_device, depth.handle, depth.memory, depth.memory_offset);
					result != VK_SUCCESS)
				{
					deinitialize_depth();
					return std::unexpected(std::format("Failed to bind depth image memory: {}", string_VkResult(result)));
				}

				const VkImageViewCreateInfo view_info{
				  .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				  .image            = depth.handle,
				  .viewType         = VK_IMAGE_VIEW_TYPE_2D,
				  .format           = m_depth_format,
				  .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
				};
				if (VkResult result = vkCreateImageView(m_device, &view_info, nullptr, &depth.view); result != VK_SUCCESS)
				{
					deinitialize_depth();
					return std::unexpected(std::format("Failed to create depth image view: {}", string_VkResult(result)));
				}
			}

			return {};
		}

		std::expected<void, std::string> resize_depth()
		{
			deinitialize_depth();
			return initialize_depth();
		}

		void deinitialize_depth() noexcept
		{
			if (m_device != VK_NULL_HANDLE)
				for (auto& depth : m_depth_images)
					destroy_image(depth);
			m_depth_images.clear();
		}

		// ############################################################
		// Renderer 3D ################################################

		std::expected<void, std::string> initialize_scene_descriptors()
		{
			const VkDescriptorSetLayoutBinding binding{
			  .binding         = 0,
			  .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			  .descriptorCount = 1,
			  .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			};
			const VkDescriptorSetLayoutCreateInfo layout_info{
			  .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			  .bindingCount = 1,
			  .pBindings    = &binding,
			};
			if (VkResult result = vkCreateDescriptorSetLayout(m_device, &layout_info, nullptr, &m_scene_descriptor_layout);
				result != VK_SUCCESS)
				return std::unexpected(
				  std::format("Failed to create scene descriptor set layout: {}", string_VkResult(result)));

			const VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frames_in_flight};
			const VkDescriptorPoolCreateInfo pool_info{
			  .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			  .maxSets       = frames_in_flight,
			  .poolSizeCount = 1,
			  .pPoolSizes    = &pool_size,
			};
			if (VkResult result = vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_scene_descriptor_pool);
				result != VK_SUCCESS)
			{
				vkDestroyDescriptorSetLayout(m_device, m_scene_descriptor_layout, nullptr);
				m_scene_descriptor_layout = VK_NULL_HANDLE;
				return std::unexpected(std::format("Failed to create scene descriptor pool: {}", string_VkResult(result)));
			}

			std::array<VkDescriptorSetLayout, frames_in_flight> layouts{};
			layouts.fill(m_scene_descriptor_layout);
			std::array<VkDescriptorSet, frames_in_flight> sets{};
			const VkDescriptorSetAllocateInfo allocate_info{
			  .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			  .descriptorPool     = m_scene_descriptor_pool,
			  .descriptorSetCount = frames_in_flight,
			  .pSetLayouts        = layouts.data(),
			};
			if (VkResult result = vkAllocateDescriptorSets(m_device, &allocate_info, sets.data()); result != VK_SUCCESS)
			{
				vkDestroyDescriptorPool(m_device, m_scene_descriptor_pool, nullptr);
				vkDestroyDescriptorSetLayout(m_device, m_scene_descriptor_layout, nullptr);
				m_scene_descriptor_pool  = VK_NULL_HANDLE;
				m_scene_descriptor_layout = VK_NULL_HANDLE;
				return std::unexpected(std::format("Failed to allocate scene descriptor sets: {}", string_VkResult(result)));
			}

			for (u32 i = 0; i < frames_in_flight; ++i)
			{
				m_frames[i].scene_descriptor_set = sets[i];
				const VkDescriptorBufferInfo buffer_info{
				  .buffer = m_frames[i].scene.handle,
				  .offset = 0,
				  .range  = sizeof(scene_uniform),
				};
				const VkWriteDescriptorSet write{
				  .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				  .dstSet          = sets[i],
				  .dstBinding      = 0,
				  .descriptorCount = 1,
				  .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				  .pBufferInfo     = &buffer_info,
				};
				vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
			}

			return {};
		}

		std::expected<void, std::string> initialize_mesh_renderer()
		{
			if (auto result = initialize_scene_descriptors(); not result)
				return result;

			shader_set<2> shaders{m_device};
			if (auto result = shaders.load_all(fs::current_path() / "shaders", {"mesh.vert", "mesh.frag"}); not result)
				return result;

			const std::array<VkVertexInputBindingDescription, 1> bindings{{
			  {.binding = 0, .stride = sizeof(mesh_vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
			}};

			const std::array<VkVertexInputAttributeDescription, 3> attributes{{
			  {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(mesh_vertex, position)},
			  {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(mesh_vertex, normal)},
			  {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(mesh_vertex, uv)},
			}};

			const pipeline_desc desc{
			  .topology     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			  .cull_mode    = VK_CULL_MODE_NONE,
			  .push_stages  = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			  .push_size    = sizeof(mesh_push_constant),
			  .descriptor_set_layout = m_texture_descriptor_layout,
			  .additional_descriptor_set_layout = m_scene_descriptor_layout,
			  .depth_test   = true,
			  .depth_write  = true,
			  .dynamic_depth = true,
			  .depth_format = m_depth_format,
			};

			auto pipeline = create_pipeline(shaders[0], shaders[1], bindings, attributes, desc);
			if (not pipeline)
				return std::unexpected(std::move(pipeline.error()));

			m_pipeline_mesh = std::move(*pipeline);
			return {};
		}

		void deinitialize_mesh_renderer()
		{
			destroy_pipeline(m_pipeline_mesh);
			if (m_scene_descriptor_pool != VK_NULL_HANDLE)
				vkDestroyDescriptorPool(m_device, m_scene_descriptor_pool, nullptr);
			m_scene_descriptor_pool = VK_NULL_HANDLE;
			if (m_scene_descriptor_layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(m_device, m_scene_descriptor_layout, nullptr);
			m_scene_descriptor_layout = VK_NULL_HANDLE;
			for (auto& frame : m_frames)
				frame.scene_descriptor_set = VK_NULL_HANDLE;
		}

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
			auto allocation = allocate_memory(
			  requirements.size, requirements.alignment, requirements.memoryTypeBits,
			  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, true, "sampled image");
			if (not allocation)
				return std::unexpected(std::move(allocation.error()));

			uploaded.memory           = allocation->memory;
			uploaded.memory_offset    = allocation->offset;
			uploaded.allocation_size  = allocation->size;
			uploaded.allocation_block = allocation->block;
			if (VkResult result = vkBindImageMemory(m_device, uploaded.handle, uploaded.memory, uploaded.memory_offset);
				result != VK_SUCCESS)
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

		// ############################################################
		// mesh upload ################################################

		[[nodiscard]] std::expected<mesh_id, std::string> upload_mesh(const mesh& source)
		{
			if (m_device == VK_NULL_HANDLE)
				return std::unexpected("Vulkan device must be initialized before uploading meshes");
			if (source.empty())
				return std::unexpected("Mesh must contain both vertices and indices");
			if (source.vertices.size() > std::numeric_limits<u32>::max()
				or source.indices.size() > std::numeric_limits<u32>::max())
				return std::unexpected("Mesh has too many vertices or indices");
			if (std::ranges::any_of(source.indices, [&](u32 index) { return index >= source.vertices.size(); }))
				return std::unexpected("Mesh index references a vertex outside the vertex array");
			if (m_meshes.size() >= max_mesh_count)
				return std::unexpected("Mesh table is full");

			mesh_resource resource{};
			if (auto result = create_buffer(
				  source.vertices.size() * sizeof(mesh_vertex),
				  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				  resource.vertex,
				  "mesh vertex");
				not result)
				return std::unexpected(std::move(result.error()));

			if (auto result = create_buffer(
				  source.indices.size() * sizeof(u32), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, resource.index, "mesh index");
				not result)
			{
				destroy_buffer(resource.vertex);
				return std::unexpected(std::move(result.error()));
			}

			std::memcpy(resource.vertex.mapped, source.vertices.data(), source.vertices.size() * sizeof(mesh_vertex));
			std::memcpy(resource.index.mapped, source.indices.data(), source.indices.size() * sizeof(u32));
			resource.index_count = static_cast<u32>(source.indices.size());

			m_meshes.push_back(resource);
			return mesh_id{static_cast<u32>(m_meshes.size() - 1)};
		}
	};

} // namespace deckard::vulkan
