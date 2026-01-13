module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


export module deckard.vulkan:shaders;
import :device;

import deckard.types;
import deckard.file;
import deckard.platform;

import std;
namespace fs = std::filesystem;
using namespace std::chrono_literals;

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

		auto content = file::read({.file = shader_file, .buffer = bytes, .size = size});
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

	// cache compiled to

	struct shader_cache_entry
	{
		fs::path                                           source_file;
		std::vector<u8>                                    spirv_data;
		std::chrono::time_point<std::chrono::system_clock> last_modified;
	};

	// Global cache for compiled shaders
	// TODO: implement

	export std::vector<u8> compile_spirv13(fs::path source_file)
	{
		static const auto glslc = platform::find_file("glslc.exe");

		if (glslc)
		{

			static const auto cache_dir = platform::get_local_appdata_path(fs::path("deckard\\shader_cache"));
			if (not fs::exists(cache_dir))
				fs::create_directories(cache_dir);


			source_file = std::filesystem::absolute(source_file);

			fs::path cache_file =
			  cache_dir / fs::path(std::format("{}_{}.spv", source_file.filename().string(), file::hash_file_contents(source_file)));

			if (fs::exists(cache_file))
			{
				dbg::println("Using cached shader: {}", cache_file.string());
				return file::read(cache_file);
			}

			std::string commandline =
			  std::format("-Werror -O --target-env=vulkan1.3 \"{}\" -o \"{}\"", source_file.string(), cache_file.string());

			auto result = platform::execute_process(*glslc, commandline, 5s, fs::current_path());

			if (result.exit_code == platform::exit_code_enum::timeout)
			{
				dbg::eprintln("Shader compilation timed out:\n{}", result.error);
				return {};
			}
			else if (result.exit_code != platform::exit_code_enum::ok)
			{
				dbg::eprintln("Shader compilation failed:\n{}", result.output);
				return {};
			}

			dbg::println("Compiled shader '{}' ({}) to '{}'", source_file.string(), result.elapsed_time, cache_file.string());


			auto spirv_data = file::read(cache_file);

			// delete 24 hour old cache files
			auto now = std::chrono::system_clock::now();
			for (const auto& entry : fs::directory_iterator(cache_dir))
			{
				auto ftime = fs::last_write_time(entry.path());
				auto sctp  = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                  ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());


				if (not entry.path().filename().string().starts_with(source_file.filename().string() + "_"))
					continue;


				auto age = now - sctp;
				if (age > 24h)
				{
					dbg::println("remove old shader: {} (age: {}s)",
								 entry.path().string(),
								 std::chrono::duration_cast<std::chrono::seconds>(age).count());
					fs::remove(entry.path());
				}
			}

			return spirv_data;
		}
		dbg::eprintln("glslc.exe not found in PATH, cannot compile shader '{}'", source_file.string());

		return {};
	}

	export std::optional<std::vector<u8>> read_spirv_shader_from_cache(fs::path spirv_binary) { return {}; }


} // namespace deckard::vulkan

