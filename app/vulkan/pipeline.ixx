module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

export module deckard.vulkan:pipeline;
import :device;
import :shaders;

import std;
import deckard.as;
import deckard.debug;
import deckard.file;
import deckard.platform;
import deckard.types;
import deckard.utils.hash;
import deckard.helpers;

namespace fs = std::filesystem;
using namespace std::string_view_literals;	

namespace deckard::vulkan
{
	export class graphics_pipeline
	{
	public:
		// Build a pipeline for dynamic rendering (no VkRenderPass).
		// color_format: the swapchain image format.
		bool initialize(device device, VkShaderModule vert, VkShaderModule frag, VkFormat color_format,
						std::span<const VkVertexInputBindingDescription>   bindings   = {},
						std::span<const VkVertexInputAttributeDescription> attributes = {})
		{
			const auto start_time = std::chrono::steady_clock::now();

			const u64 key         = compute_pipeline_key(vert, frag, color_format, bindings, attributes);
			m_cache_path          = cache_path_from_key(key);
			const auto cache_data = file::read(m_cache_path);

			VkPipelineCacheCreateInfo cache_info{
			  .sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
			  .initialDataSize = cache_data.size(),
			  .pInitialData    = cache_data.empty() ? nullptr : cache_data.data(),
			};

			if (vkCreatePipelineCache(device, &cache_info, nullptr, &m_cache) != VK_SUCCESS)
			{
				dbg::println("Failed to create pipeline cache, continuing without cache");
				m_cache = VK_NULL_HANDLE;
			}


			// Pipeline layout (no descriptors, no push constants for a plain triangle)
			VkPipelineLayoutCreateInfo layout_info{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
			if (vkCreatePipelineLayout(device, &layout_info, nullptr, &m_layout) != VK_SUCCESS)
			{
				dbg::println("Failed to create pipeline layout");
				return false;
			}

			// Shader stages
			const std::array<VkPipelineShaderStageCreateInfo, 2> stages{{
			  {
				.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage  = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vert,
				.pName  = "main",
			  },
			  {
				.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = frag,
				.pName  = "main",
			  },
			}};

			// No vertex input — positions are baked into the vertex shader
			VkPipelineVertexInputStateCreateInfo vertex_input{
			  .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			  .vertexBindingDescriptionCount   = as<u32>(bindings.size()),
			  .pVertexBindingDescriptions      = bindings.empty() ? nullptr : bindings.data(),
			  .vertexAttributeDescriptionCount = as<u32>(attributes.size()),
			  .pVertexAttributeDescriptions    = attributes.empty() ? nullptr : attributes.data(),
			};

			VkPipelineInputAssemblyStateCreateInfo input_assembly{
			  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			  .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			  .primitiveRestartEnable = VK_FALSE,
			};

			// Dynamic viewport and scissor
			VkPipelineViewportStateCreateInfo viewport_state{
			  .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			  .viewportCount = 1,
			  .scissorCount  = 1,
			};

			VkPipelineRasterizationStateCreateInfo rasterizer{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			  .depthClampEnable        = VK_FALSE,
			  .rasterizerDiscardEnable = VK_FALSE,
			  .polygonMode             = VK_POLYGON_MODE_FILL,
			  .cullMode                = VK_CULL_MODE_NONE,
			  .frontFace               = VK_FRONT_FACE_CLOCKWISE,
			  .depthBiasEnable         = VK_FALSE,
			  .lineWidth               = 1.0f,
			};

			VkPipelineMultisampleStateCreateInfo multisample{
			  .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			  .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			};

			VkPipelineColorBlendAttachmentState blend_attachment{
			  .blendEnable    = VK_FALSE,
			  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
								| VK_COLOR_COMPONENT_A_BIT,
			};

			VkPipelineColorBlendStateCreateInfo blend{
			  .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			  .logicOpEnable   = VK_FALSE,
			  .attachmentCount = 1,
			  .pAttachments    = &blend_attachment,
			};

			constexpr std::array<VkDynamicState, 2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			VkPipelineDynamicStateCreateInfo        dynamic_state{
			  .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			  .dynamicStateCount = as<u32>(dynamic_states.size()),
			  .pDynamicStates    = dynamic_states.data(),
			};


			// feedback for pipeline creation time and stage creation times
			std::array<VkPipelineCreationFeedback, 2> stage_feedback{};

			VkPipelineCreationFeedback           creation_feedback{};
			VkPipelineCreationFeedbackCreateInfo feedback_info{
			  .sType                              = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO,
			  .pPipelineCreationFeedback          = &creation_feedback,
			  .pipelineStageCreationFeedbackCount = as<u32>(stage_feedback.size()),
			  .pPipelineStageCreationFeedbacks    = stage_feedback.data(),
			};


			// Dynamic rendering — attach color format, no depth/stencil
			VkPipelineRenderingCreateInfo rendering{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			  .pNext                   = &feedback_info,
			  .colorAttachmentCount    = 1,
			  .pColorAttachmentFormats = &color_format,

			  .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
			  .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
			};

			VkGraphicsPipelineCreateInfo pipeline_info{
			  .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			  .pNext               = &rendering,
			  .stageCount          = as<u32>(stages.size()),
			  .pStages             = stages.data(),
			  .pVertexInputState   = &vertex_input,
			  .pInputAssemblyState = &input_assembly,
			  .pViewportState      = &viewport_state,
			  .pRasterizationState = &rasterizer,
			  .pMultisampleState   = &multisample,
			  .pColorBlendState    = &blend,
			  .pDynamicState       = &dynamic_state,
			  .layout              = m_layout,

			  .renderPass = VK_NULL_HANDLE, // Not used with dynamic rendering
			  .subpass    = 0,              // Not used with dynamic rendering
			};

			if (vkCreateGraphicsPipelines(device, m_cache, 1, &pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS)
			{
				dbg::println("Failed to create graphics pipeline");
				return false;
			}



			const auto duration_ns = std::chrono::nanoseconds(creation_feedback.duration);

			const auto elapsed = std::chrono::steady_clock::now() - start_time;
			dbg::println("Created graphics pipeline in {} (driver: {})", pretty_time(elapsed), pretty_time(duration_ns));
			dbg::println("Feedback flags: {}", string_VkPipelineShaderStageCreateFlags(creation_feedback.flags));

			for (size_t i = 0; i < stage_feedback.size(); ++i)
			{

				const auto& fb = stage_feedback[i];
				if (fb.duration == 0)
					continue;

				const auto  ns = std::chrono::nanoseconds(fb.duration);

				std::string flags_str;
				if (fb.flags & VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT)
					flags_str += "valid ";
				if (fb.flags & VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT)
					flags_str += "cache_hit ";
				if (fb.flags & VK_PIPELINE_CREATION_FEEDBACK_BASE_PIPELINE_ACCELERATION_BIT)
					flags_str += "base_pipeline_accel ";
				if (fb.flags == 0)
					flags_str = "none";

				const auto type = (stages[i].stage == VK_SHADER_STAGE_VERTEX_BIT) ? "vertex"sv : "fragment"sv;

				dbg::println("Stage {} ({}) feedback: {}, flags: {}", i, type, pretty_time(ns), flags_str);
				dbg::println("  Stage flags: {}", string_VkPipelineShaderStageCreateFlags(stages[i].flags));
			}

			if (m_cache != VK_NULL_HANDLE)
				save_pipeline_cache(device);

			return true;
		}

		void deinitialize(VkDevice device)
		{
			if (m_pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(device, m_pipeline, nullptr);
				m_pipeline = VK_NULL_HANDLE;
			}
			if (m_layout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(device, m_layout, nullptr);
				m_layout = VK_NULL_HANDLE;
			}
			if (m_cache != VK_NULL_HANDLE)
			{
				vkDestroyPipelineCache(device, m_cache, nullptr);
				m_cache = VK_NULL_HANDLE;
			}
		}

		void save_pipeline_cache(VkDevice device)
		{
			if (m_cache == VK_NULL_HANDLE)
				return;

			if (fs::exists(m_cache_path))
			{
				dbg::println("Pipeline cache '{}' already exists, skipping", m_cache_path.string());
				return;
			}

			// TODO: wipe old caches after writing new

			size_t data_size{0};
			if (vkGetPipelineCacheData(device, m_cache, &data_size, nullptr) != VK_SUCCESS or data_size == 0)
				return;

			std::vector<u8> data(data_size);
			if (vkGetPipelineCacheData(device, m_cache, &data_size, data.data()) != VK_SUCCESS)
				return;

			const auto result = file::write({.filename = m_cache_path, .buffer = data});
			if (result)
			{
				dbg::println("Wrote pipeline cache '{}' ({} bytes)", m_cache_path.string(), data_size);
			}
			else
			{
				dbg::println("Failed to write pipeline cache '{}': {}", m_cache_path.string(), result.error());
			}
		}

		operator VkPipeline() const { return m_pipeline; }

		bool valid() const { return m_pipeline != VK_NULL_HANDLE; }

	private:
		[[nodiscard]] static u64 compute_pipeline_key(
		  VkShaderModule vert, VkShaderModule frag, VkFormat color_format,
		  std::span<const VkVertexInputBindingDescription>   bindings,
		  std::span<const VkVertexInputAttributeDescription> attributes)
		{
			u64 seed = utils::constant_seed;

			seed = utils::hash_combine(seed, reinterpret_cast<u64>(vert));
			seed = utils::hash_combine(seed, reinterpret_cast<u64>(frag));
			seed = utils::hash_combine(seed, static_cast<u64>(color_format));

			seed = utils::hash_values(
			  std::span<const u8>(reinterpret_cast<const u8*>(bindings.data()), bindings.size_bytes()));
			seed = utils::hash_values(
			  std::span<const u8>(reinterpret_cast<const u8*>(attributes.data()), attributes.size_bytes()));

			return seed;
		}

		[[nodiscard]] static fs::path cache_path_from_key(u64 key)
		{
			static const auto cache_dir = []
			{
				auto dir = platform::get_local_appdata_path(fs::path("deckard") / "pipeline_cache");
				if (not fs::exists(dir))
					fs::create_directories(dir);
				return dir;
			}();

			return cache_dir / std::format("{:016X}.bin", key);
		}

		VkPipeline       m_pipeline{VK_NULL_HANDLE};
		VkPipelineLayout m_layout{VK_NULL_HANDLE};

		VkPipelineCache m_cache{VK_NULL_HANDLE};
		fs::path        m_cache_path;
	};

} // namespace deckard::vulkan
