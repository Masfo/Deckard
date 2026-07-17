module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

export module deckard.vulkan:pipeline;
import :device;
import :shaders;

import std;
import deckard.as;
import deckard.debug;
import deckard.types;

namespace deckard::vulkan
{
	export class graphics_pipeline
	{
	public:
		// Build a pipeline for dynamic rendering (no VkRenderPass).
		// color_format: the swapchain image format.
		bool initialize(
		  device                                           device,
		  VkShaderModule                                   vert,
		  VkShaderModule                                   frag,
		  VkFormat                                         color_format,
		  std::span<const VkVertexInputBindingDescription>   bindings   = {},
		  std::span<const VkVertexInputAttributeDescription> attributes = {})
		{
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
			  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			};

			VkPipelineColorBlendStateCreateInfo blend{
			  .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			  .logicOpEnable   = VK_FALSE,
			  .attachmentCount = 1,
			  .pAttachments    = &blend_attachment,
			};

			constexpr std::array<VkDynamicState, 2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			VkPipelineDynamicStateCreateInfo         dynamic_state{
			  .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			  .dynamicStateCount = as<u32>(dynamic_states.size()),
			  .pDynamicStates    = dynamic_states.data(),
			};

			// Dynamic rendering — attach color format, no depth/stencil
			VkPipelineRenderingCreateInfo rendering{
			  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			  .colorAttachmentCount    = 1,
			  .pColorAttachmentFormats = &color_format,
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
			  .renderPass          = VK_NULL_HANDLE,
			};

			if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_pipeline) != VK_SUCCESS)
			{
				dbg::println("Failed to create graphics pipeline");
				return false;
			}

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
		}

		operator VkPipeline() const { return m_pipeline; }

		bool valid() const { return m_pipeline != VK_NULL_HANDLE; }

	private:
		VkPipeline       m_pipeline{VK_NULL_HANDLE};
		VkPipelineLayout m_layout{VK_NULL_HANDLE};
	};

} // namespace deckard::vulkan
