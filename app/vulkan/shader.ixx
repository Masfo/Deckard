module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:shader;

import std;
import deckard.types;

namespace deckard::vulkan
{
	class shader
	{
	private:
		VkDevice               device       = VK_NULL_HANDLE;
		VkShaderModule         shaderModule = VK_NULL_HANDLE;
		VkShaderCreateInfoEXT* pCreateInfos;
		VkShaderStageFlagBits  stage;
		VkShaderStageFlags     next_stage;
		VkShaderEXT            shaderEXT  = VK_NULL_HANDLE;
		VkShaderCreateInfoEXT  createInfo = {};
		std::vector<u32>       spirv;

	public:
	};

} // namespace deckard::vulkan
