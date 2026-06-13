module;
#include <vulkan/vulkan.h>

export module deckard.vulkan:buffer;
import :device;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;

namespace deckard::vulkan
{
	export class vertex_buffer
	{
	public:
		bool initialize(device& dev, std::span<const std::byte> data)
		{
			const VkDeviceSize size = data.size_bytes();

			VkBufferCreateInfo buffer_info{
			  .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			  .size        = size,
			  .usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			  .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			};

			if (vkCreateBuffer(dev, &buffer_info, nullptr, &m_buffer) != VK_SUCCESS)
			{
				dbg::println("Failed to create vertex buffer");
				return false;
			}

			VkMemoryRequirements mem_req{};
			vkGetBufferMemoryRequirements(dev, m_buffer, &mem_req);

			VkPhysicalDeviceMemoryProperties mem_props{};
			vkGetPhysicalDeviceMemoryProperties(dev.physical_device(), &mem_props);

			u32 memory_type = 0xFFFF'FFFF;
			for (u32 i = 0; i < mem_props.memoryTypeCount; ++i)
			{
				const bool type_ok = (mem_req.memoryTypeBits >> i) & 1u;
				const bool flags_ok =
				  (mem_props.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
				  (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				if (type_ok and flags_ok)
				{
					memory_type = i;
					break;
				}
			}

			if (memory_type == 0xFFFF'FFFF)
			{
				dbg::println("No suitable memory type for vertex buffer");
				return false;
			}

			VkMemoryAllocateInfo alloc_info{
			  .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			  .allocationSize  = mem_req.size,
			  .memoryTypeIndex = memory_type,
			};

			if (vkAllocateMemory(dev, &alloc_info, nullptr, &m_memory) != VK_SUCCESS)
			{
				dbg::println("Failed to allocate vertex buffer memory");
				return false;
			}

			vkBindBufferMemory(dev, m_buffer, m_memory, 0);

			void* mapped{nullptr};
			vkMapMemory(dev, m_memory, 0, size, 0, &mapped);
			std::memcpy(mapped, data.data(), data.size_bytes());
			vkUnmapMemory(dev, m_memory);

			m_vertex_count = as<u32>(data.size_bytes());
			return true;
		}

		void deinitialize(VkDevice dev)
		{
			if (m_buffer != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(dev, m_buffer, nullptr);
				m_buffer = VK_NULL_HANDLE;
			}
			if (m_memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(dev, m_memory, nullptr);
				m_memory = VK_NULL_HANDLE;
			}
		}

		operator VkBuffer() const { return m_buffer; }
		bool valid() const { return m_buffer != VK_NULL_HANDLE; }

	private:
		VkBuffer       m_buffer{VK_NULL_HANDLE};
		VkDeviceMemory m_memory{VK_NULL_HANDLE};
		u32            m_vertex_count{0};
	};

} // namespace deckard::vulkan
