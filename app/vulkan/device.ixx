module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

#include <vulkan/vulkan_win32.h>

export module deckard.vulkan:device;

import :semaphore;

import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	constexpr u32 VENDOR_NVIDIA   = 0x10DE;
	constexpr u32 VENDOR_AMD      = 0x1002;
	constexpr u32 VENDOR_INTEL    = 0x8086;
	constexpr u32 VENDOR_ARM      = 0x13B5;
	constexpr u32 VENDOR_QUALCOMM = 0x5143;

	export class device
	{
	private:
		std::vector<VkExtensionProperties> device_extensions;

		bool has_extension(std::string_view name) const
		{
			return std::ranges::contains(
			  device_extensions, name, [](const VkExtensionProperties& ext) { return std::string_view(ext.extensionName); });
		}

		VkDevice         m_device{nullptr};
		VkPhysicalDevice m_physical_device{nullptr};
		VkQueue          m_queue{nullptr};

	public:
		bool initialize(VkInstance instance)
		{
			assert::check(instance != nullptr);


			u32      device_count{0};
			VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
			if (result != VK_SUCCESS)
			{
				dbg::println("Enumerate physical devices failed: {}", string_VkResult(result));

				return false;
			}

			std::vector<VkPhysicalDevice> devices;
			devices.resize(device_count);
			result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

			if (result != VK_SUCCESS)
			{
				dbg::println("{}", string_VkResult(result));

				return false;
			}

			std::vector<u32> discrete_gpus;
			std::vector<u32> integrated_gpus;

			std::vector<VkPhysicalDeviceProperties> device_properties;
			device_properties.resize(device_count);

			std::vector<VkPhysicalDeviceMemoryProperties> device_memories;
			device_memories.resize(device_count);

			std::vector<VkPhysicalDeviceFeatures> device_features;
			device_features.resize(device_count);

			std::vector<u64> mem_counts;
			mem_counts.resize(device_count);


			for (u32 i = 0; i < device_count; i++)
			{
				VkPhysicalDeviceVulkan13Features v13_features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
				v13_features.synchronization2 = VK_TRUE;
				v13_features.dynamicRendering = VK_TRUE;

				VkPhysicalDeviceFeatures2 feat2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
				feat2.pNext = &v13_features;
				vkGetPhysicalDeviceFeatures2(devices[i], &feat2);


				vkGetPhysicalDeviceProperties(devices[i], &device_properties[i]);

				const auto& prop = device_properties[i];

				if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
					discrete_gpus.push_back(i);

				if (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
					integrated_gpus.push_back(i);

				vkGetPhysicalDeviceMemoryProperties(devices[i], &device_memories[i]);
				for (u32 j = 0; j < device_memories[i].memoryHeapCount; j++)
					mem_counts[i] += device_memories[i].memoryHeaps[j].size;

				// Features
				vkGetPhysicalDeviceFeatures(devices[i], &device_features[i]);
			}

			std::vector<u32> gpu_score;
			gpu_score.resize(device_count);
			// TODO: u32 score, add points for best score, d-gpu +10, i-gpu, +5

			u32 best_gpu_index{0};
			if (not discrete_gpus.empty())
			{
				// just take first discrete
				// TODO: maybe better way
				best_gpu_index = discrete_gpus[0];
			}
			else if (not integrated_gpus.empty())
			{
				// fallback on integrated
				best_gpu_index = integrated_gpus[0];
			}
			else
			{
				dbg::println("Vulkan: no suitable GPU");
				return false;
			}

			m_physical_device = devices[best_gpu_index];

			// device memory

			VkPhysicalDeviceMemoryProperties memory_property{};
			vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_property);

			for (u32 i = 0; i < memory_property.memoryTypeCount; i++)
			{
				VkMemoryType memtype = memory_property.memoryTypes[i];
				dbg::println("{}. {}, index {}", i, memtype.propertyFlags, memtype.heapIndex);

				if (memtype.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
					dbg::print("DEVICE LOCAL, ");
				if (memtype.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
					dbg::print("HOST VISIBLE BIT, ");
				if (memtype.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
					dbg::print("HOST COHERENT BIT, ");
				if (memtype.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
					dbg::print("HOST CACHED BIT, ");

				dbg::println("{} bytes", memory_property.memoryHeaps[memtype.heapIndex].size);
				dbg::println();
			}

			for (u32 i = 0; i < memory_property.memoryHeapCount; i++)
			{
				auto heap = memory_property.memoryHeaps[i];

				dbg::print("Flags: ");
				if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
					dbg::print("HEAP DEVICE LOCAL, ");
				if (heap.flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
					dbg::print("HEAP MULTI INSTANCE BIT, ");
				dbg::println();

				dbg::println("Heap {}. {}, {} bytes", i, heap.flags, heap.size);
			}

			// device extensions
			u32 de_count{0};
			result = vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &de_count, nullptr);
			if (result != VK_SUCCESS)
			{
				dbg::println("Enumerate device extensions: {}", string_VkResult(result));
				return false;
			}

			device_extensions.resize(de_count);
			result = vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &de_count, device_extensions.data());
			if (result != VK_SUCCESS)
			{
				dbg::println("Enumerate device extensions: {}", string_VkResult(result));
				return false;
			}
			//


			const auto& prop = device_properties[best_gpu_index];

			dbg::println("Device: {}", prop.deviceName);


			// NVidia versioning 10/8/8/6
			// AMD, Intel, vulkan default 10/10/12
			if (prop.vendorID == VENDOR_NVIDIA)
			{
				u32 major = (prop.driverVersion >> 22) & 0x3ff;
				u32 minor = (prop.driverVersion >> 14) & 0x0ff;
				u32 patch = (prop.driverVersion >> 6) & 0x0ff;
				u32 rev   = (prop.driverVersion) & 0x003f;

				dbg::println("Driver: {}.{}.{}.{}", major, minor, patch, rev);
			}
			else
			{

				dbg::println("Driver v{}.{}.{}",
							 VK_API_VERSION_MAJOR(prop.driverVersion),
							 VK_API_VERSION_MINOR(prop.driverVersion),
							 VK_API_VERSION_PATCH(prop.driverVersion));
			}

			dbg::println("Vulkan API v{}.{}.{}",
						 VK_API_VERSION_MAJOR(prop.apiVersion),
						 VK_API_VERSION_MINOR(prop.apiVersion),
						 VK_API_VERSION_PATCH(prop.apiVersion));


			// queue
			u32 queue_families_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);
			if (queue_families_count == 0)
			{
				dbg::println("Vulkan device has no queue families");
				return false;
			}


			i32 queue_index{select_queue()};

			// queue
			VkDeviceQueueCreateInfo queue_create{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};

			std::array<float, 1> priority{1.0f};
			queue_create.queueCount       = 1;
			queue_create.queueFamilyIndex = queue_index;
			queue_create.pQueuePriorities = &priority[0];


			// required device extensions
			std::vector<const char*> extensions;
			extensions.reserve(device_extensions.size());

			bool instance_is_1_2 = false;

			u32      InstanceVersion = 0;
			VkResult res             = vkEnumerateInstanceVersion(&InstanceVersion);

			if (res == VK_SUCCESS)
			{
				instance_is_1_2 = VK_API_VERSION_MAJOR(InstanceVersion) >= 1 and VK_API_VERSION_MINOR(InstanceVersion) >= 2;
				u32 major       = VK_API_VERSION_MAJOR(InstanceVersion);
				u32 minor       = VK_API_VERSION_MINOR(InstanceVersion);
				u32 patch       = VK_API_VERSION_PATCH(InstanceVersion);

				dbg::println("Vulkan instance version: {}.{}.{}", major, minor, patch);
			}
			else
			{
				dbg::println("Failed to get instance version: {}", string_VkResult(res));
			}

			dbg::println("Vulkan device extensions({}):", device_extensions.size());

			for (const auto& extension : device_extensions)
			{
				std::string_view name   = extension.extensionName;
				bool             marked = false;

				if (name.compare(VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
				{
					extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
					marked = true;
				}

				if (name.compare(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
				}

				// If the VK_KHR_portability_subset extension is included in pProperties of
				// vkEnumerateDeviceExtensionProperties, ppEnabledExtensionNames must include "VK_KHR_portability_subset"
				if (name.compare(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0)
				{

					extensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
					marked = true;
				}

				if (instance_is_1_2 and name.compare(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
				}

				if (name.compare(VK_EXT_SHADER_OBJECT_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
				}

				//
				if (name.compare(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
				}

				if (name.compare(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
				}

				if (name.compare(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
				}

				if (name.compare(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME) == 0)
				{
					marked = true;
					extensions.emplace_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
				}


				dbg::println("{:>48}{} (rev {})", name, marked ? "*" : " ", VK_API_VERSION_PATCH(extension.specVersion));
			}
			dbg::println();

			//


			//
			VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
			features13.synchronization2 = VK_TRUE;
			features13.dynamicRendering = VK_TRUE;

			VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
			features12.bufferDeviceAddress = true;
			features12.descriptorIndexing  = true;


			VkPhysicalDeviceShaderObjectFeaturesEXT shader_features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT};
			shader_features.shaderObject = VK_TRUE;

			features12.pNext      = &features13;
			features13.pNext      = &shader_features;
			shader_features.pNext = nullptr;

			VkDeviceCreateInfo device_create{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
			device_create.pNext = &features12;

			device_create.queueCreateInfoCount = 1;
			device_create.pQueueCreateInfos    = &queue_create;
			// extensions
			device_create.enabledExtensionCount   = as<u32>(extensions.size());
			device_create.ppEnabledExtensionNames = extensions.data();


			result = vkCreateDevice(m_physical_device, &device_create, nullptr, &m_device);

			if (result != VK_SUCCESS)
			{
				dbg::println("Vulkan m_device creation failed: {}", string_VkResult(result));
				return false;
			}


			vkGetDeviceQueue(m_device, queue_index, 0, &m_queue);
			assert::check(m_queue != nullptr);

			return true;
		}

		void deinitialize()

		{

			if (m_device != nullptr)
			{
				vkDeviceWaitIdle(m_device);
				vkDestroyDevice(m_device, nullptr);
				m_device = nullptr;
			}
		}

		VkResult next_swapchain_image(VkDevice device, VkSwapchainKHR swapchain, VkSemaphore image_available, u32* index)
		{
			return vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available, nullptr, index);
		}

		VkResult present(const VkSwapchainKHR swapchain, VkSemaphore rendering_finished, u32 image_index)
		{
			VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

			present_info.waitSemaphoreCount = 1;
			present_info.pWaitSemaphores    = &rendering_finished;

			present_info.swapchainCount = 1;
			present_info.pSwapchains    = &swapchain;
			present_info.pImageIndices  = &image_index;

			return vkQueuePresentKHR(m_queue, &present_info);
		}

		i32 select_queue() const
		{
			u32 queue_families_count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);
			if (queue_families_count == 0)
			{
				dbg::println("Vulkan device has no queue families");
				return false;
			}

			i32                                  queue_index{-1};
			std::vector<VkQueueFamilyProperties> queue_family_properties(queue_families_count);

			vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, queue_family_properties.data());

			for (u32 i = 0; i < queue_families_count; i++)
			{
				if ((queue_family_properties[i].queueCount > 0) and (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
				{
					queue_index = i;
					break;
				}
			}

			if (queue_index < 0)
				dbg::println("Vulkan device had not required queue family properties");

			return queue_index;
		}

		void wait() const
		{
			assert::check(m_device != nullptr);
			vkDeviceWaitIdle(m_device);
		}

		VkResult present(VkSemaphore rendering_finished, u32& image_index, VkSwapchainKHR swapchain)
		{
			VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

			present_info.waitSemaphoreCount = 1;
			present_info.pWaitSemaphores    = &rendering_finished;

			present_info.swapchainCount = 1;
			present_info.pSwapchains    = &swapchain;
			present_info.pImageIndices  = &image_index;

			return vkQueuePresentKHR(m_queue, &present_info);
		}

		operator VkDevice() const { return m_device; }

		VkPhysicalDevice physical_device() const { return m_physical_device; }

		VkQueue queue() const { return m_queue; }
	};

} // namespace deckard::vulkan
