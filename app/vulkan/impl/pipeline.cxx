module;
#include <windows.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

module deckard.vulkan;

import std;
import deckard.debug;

namespace deckard::vulkan
{
	std::expected<pipeline_state, std::string> context::create_pipeline(
	  VkShaderModule vertex, VkShaderModule fragment, std::span<const VkVertexInputBindingDescription> bindings,
	  std::span<const VkVertexInputAttributeDescription> attributes, const pipeline_desc& desc) const
	{
		assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be created before initializing pipeline");

		graphics_pipeline_config config{};
		config.vertex_bindings.assign(bindings.begin(), bindings.end());
		config.vertex_attributes.assign(attributes.begin(), attributes.end());
		config.topology  = desc.topology;
		config.cull_mode = desc.cull_mode;
		config.depth_test_enable        = desc.depth_test ? VK_TRUE : VK_FALSE;
		config.depth_write_enable       = desc.depth_write ? VK_TRUE : VK_FALSE;
		config.depth_compare_op         = desc.depth_compare;
		config.depth_attachment_format  = desc.depth_format;
		if (desc.dynamic_depth)
		{
			config.dynamic_states.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE);
			config.dynamic_states.push_back(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE);
		}
		if (desc.alpha_blend)
		{
			config.color_blend_attachments[0] = {
			  .blendEnable         = VK_TRUE,
			  .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			  .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			  .colorBlendOp        = VK_BLEND_OP_ADD,
			  .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			  .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			  .alphaBlendOp        = VK_BLEND_OP_ADD,
			  .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
									 | VK_COLOR_COMPONENT_A_BIT,
			};
		}

		if (desc.descriptor_set_layout != VK_NULL_HANDLE)
			config.descriptor_set_layouts.push_back(desc.descriptor_set_layout);
		if (desc.additional_descriptor_set_layout != VK_NULL_HANDLE)
			config.descriptor_set_layouts.push_back(desc.additional_descriptor_set_layout);

		if (desc.push_size != 0)
			config.push_constant_ranges.push_back({
			  .stageFlags = desc.push_stages,
			  .offset     = 0,
			  .size       = desc.push_size,
			});

		return pipeline_creator::create(m_device, vertex, fragment, m_surface_format.format, config);
	}

	void context::destroy_pipeline(pipeline_state& state) const noexcept
	{
		destroy(state.pipeline, vkDestroyPipeline);
		destroy(state.layout, vkDestroyPipelineLayout);
		destroy(state.cache, vkDestroyPipelineCache);
	}

	void context::deinitialize_pipelines()
	{
		for (auto& pipeline : m_pipelines)
			destroy_pipeline(pipeline);

		m_pipelines.clear();
	}

} // namespace deckard::vulkan
