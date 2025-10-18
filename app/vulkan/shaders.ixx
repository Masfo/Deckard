module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


export module deckard.vulkan:shaders;
import :device;

import deckard.types;
import deckard.file;

import std;
namespace fs = std::filesystem;

namespace deckard::vulkan
{
#define SPV_SPIRV_VERSION_WORD(MAJOR, MINOR) ((uint32_t(uint8_t(MAJOR)) << 16) | (uint32_t(uint8_t(MINOR)) << 8))

	constexpr u32 SPIRV_MAGIC_HEADER = 0x0723'0203;

	class shader
	{
	private:
		VkShaderModule shader_module{VK_NULL_HANDLE};

	public:
		// TODO: have fs::path member, have a reload method

		std::expected<bool, std::string> load(device device, fs::path shader_file)
		{
			if (not fs::exists(shader_file))
				return std::unexpected(std::format("Shader '{}' not found", shader_file.string()));

			auto fsize = file::filesize(shader_file);
			if (fsize and *fsize % sizeof(u32) != 0)
				return std::unexpected(std::format("Shader '{}' module size not a multiple of {}", shader_file.string(), sizeof(u32)));

			u64 size = *fsize;

			//
			std::vector<u8> bytes;
			bytes.resize(size);

			auto content = file::read(shader_file, bytes, size);
			if (not content)
				return std::unexpected(content.error());


			//
			std::vector<u32> spirv;
			spirv.reserve(size / 4);

			for (u64 i = 0; i < size; i += 4)
			{
				std::array<u8, 4> chunk{bytes[i + 0], bytes[i + 1], bytes[i + 2], bytes[i + 3]};
				spirv.push_back(std::bit_cast<u32>(chunk));
			}


			// Check header
			if (spirv[0] != SPIRV_MAGIC_HEADER)
				return std::unexpected(std::format("Shader '{}' header is invalid", shader_file.string()));

			u8 major = (spirv[1] >> 16) & 0xFF;
			u8 minor = (spirv[1] >> 8) & 0xFF;

			dbg::println("SPIR-V version: v{}.{}", major, minor);
			dbg::println("SPIR-V generator: 0x{:X}", spirv[2]);
			dbg::println("SPIR-V bound: 0x{:X}", spirv[3]);
			dbg::println("SPIR-V schema: 0x{:X}", spirv[4]);


			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize                 = spirv.size() * sizeof(u32);
			createInfo.pCode                    = spirv.data();

			if (vkCreateShaderModule(device, &createInfo, nullptr, &shader_module) != VK_SUCCESS)
				return std::unexpected(std::format("Failed to create shader module '{}'", shader_file.string()));

			return true;
		}

		operator VkShaderModule() const { return shader_module; }
	};
} // namespace deckard::vulkan
