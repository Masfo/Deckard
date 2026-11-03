module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:texture;


namespace deckard::vulkan
{
	export struct texture_id
	{
		u32 index{0};
		u32 generation{0};

		[[nodiscard]] constexpr bool operator==(const texture_id& other) const noexcept
		{
			return index == other.index and generation == other.generation;
		}

		[[nodiscard]] constexpr bool operator!=(const texture_id& other) const noexcept { return not(*this == other); }
	};



}; // namespace deckard::vulkan
