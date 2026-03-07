module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>



export module deckard.vulkan:core;

import std;
import deckard.debug;
import deckard.types;

/*
 *  PushContants: 128 bytes - 256 bytes
 *		- maxPushConstantsSize
 *			- AMD RX 5700 XT:		256
			- Nvidia RTX 5060 Ti:	256
			- Intel Arc A380:		256
 *
 *  Uniform buffer (UBO): 16KB -> 64KB (min guaranteed by spec) -> 256KB (max guaranteed by spec)
		- AMD has max_uint size, nvidia typically 64KB, intel typically 16KB
		- maxUniformBufferRange
			- AMD RX 5700 XT:		4294967295
			- Nvidia RTX 5060 Ti:	65536
			- Intel Arc A380:		1073741824
 *
 *  Shader Storage Buffer (SSBO): 16KB -> 64KB (min guaranteed by spec) -> 256KB (max guaranteed by spec)
 *			maxStorageBufferRange
 *			- AMD RX 5700 XT:		4294967295
			- Nvidia RTX 5060 Ti:	4294967295
			- Intel Arc A380:		4294967295
* 
* 
* 
*	Vertex Pulling
* 
*/ 



namespace deckard::vulkan
{
	constexpr u32 VENDOR_NVIDIA   = 0x10DE;
	constexpr u32 VENDOR_AMD      = 0x1002;
	constexpr u32 VENDOR_INTEL    = 0x8086;
	constexpr u32 VENDOR_ARM      = 0x13B5;
	constexpr u32 VENDOR_QUALCOMM = 0x5143;

	struct GPUInfo
	{
		u64              vram_size_mb{0};
		VkPhysicalDevice physical_device{nullptr};
		VkPhysicalDeviceType type{VK_PHYSICAL_DEVICE_TYPE_OTHER}; // Discrete, Integrated
		int                  score{0};
		bool                 dynamic_rendering{false};

		bool operator<(const GPUInfo& other) const { return score > other.score; } // High to low

	};

	// Dynamic renderer
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR{nullptr};
	PFN_vkCmdEndRenderingKHR   vkCmdEndRenderingKHR{nullptr};

	// Debug utils
	PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT{nullptr};
	PFN_vkSubmitDebugUtilsMessageEXT    vkSubmitDebugUtilsMessageEXT{nullptr};
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};
	bool                                debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
													   const VkDebugUtilsMessengerCallbackDataEXT*, void*);

	export class core
	{


	private:
		// instance
		std::vector<VkLayerProperties>     validator_layers;
		std::vector<VkExtensionProperties> instance_extensions;
		VkInstance                         instance{nullptr};

		[[nodiscard("Check the instance status")]] bool initialize_instance();
		bool                                            enumerate_instance_extensions(std::vector<VkExtensionProperties>&);
		bool                                            enumerate_validator_layers(std::vector<VkLayerProperties>&);

		// Device
		std::vector<VkExtensionProperties> device_extensions;
		// VkDevice                           device{nullptr};
		VkPhysicalDevice physical_device{nullptr};
		VkQueue          queue{nullptr};

		[[nodiscard("Check the device status")]] bool initialize_device();

		// Surface
		VkSurfaceKHR             surface{nullptr};
		VkSurfaceCapabilitiesKHR surface_capabilities;

		// Swapchain
		VkSwapchainKHR m_swapchain{nullptr};


		// Debug
		VkDebugUtilsMessengerEXT debug_messenger{nullptr};
		bool                     initialize_debug_functions(void*);


	private:
		u32 minimum_apiversion{VK_API_VERSION_1_3};

		void initialize()
		{
			if (not initialize_instance())
			{
				dbg::println("Vulkan instance initialization failed");
				return;
			}
			if (not initialize_debug_functions(this))
			{
				dbg::println("Vulkan debug functions initialization failed");
				return;
			}
			if (not initialize_device())
			{
				dbg::println("Vulkan device initialization failed");
				return;
			}
		}

		void deinitialize()
		{
			if (vkDestroyDebugUtilsMessengerEXT != nullptr)
			{
				vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
				debug_messenger = nullptr;
			}

			if (instance != nullptr)
			{
				vkDestroyInstance(instance, nullptr);
				instance = nullptr;
			}
		}

	public:
		core() { initialize(); }

		~core() { deinitialize(); }
	};


} // namespace deckard::vulkan
