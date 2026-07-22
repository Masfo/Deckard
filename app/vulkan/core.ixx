module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


export module deckard.vulkan:core;

import std;
import deckard.debug;
import deckard.as;
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
https://vulkan.gpuinfo.org/compare.php?devices[]=NVIDIA%20GeForce%20RTX%204060&os=all&devices[]=AMD%20Radeon%20RX%209060%20XT&os=all&devices[]=Intel(R)%20Arc(tm)%20A380%20Graphics%20(DG2)&os=all
*
*	Vertex Pulling
*
*/


namespace deckard::vulkan
{
	using namespace deckard;

	constexpr u32 VENDOR_NVIDIA   = 0x10DE;
	constexpr u32 VENDOR_AMD      = 0x1002;
	constexpr u32 VENDOR_INTEL    = 0x8086;
	constexpr u32 VENDOR_ARM      = 0x13B5;
	constexpr u32 VENDOR_QUALCOMM = 0x5143;

	struct GPUInfo
	{
		u64                  vram_size_mb{0};
		VkPhysicalDevice     physical_device{nullptr};
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
	bool debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
						const VkDebugUtilsMessengerCallbackDataEXT*, void*);

	struct frameresource
	{
		u32             last_frame{0};
		VkCommandPool   command_pool{nullptr};
		VkCommandBuffer command_buffer{nullptr};
		VkSemaphore     image_available_semaphore{nullptr};
		VkSemaphore     render_finished_semaphore{nullptr};
	};


	export class core
	{


	private:
		// instance
		VkInstance                         instance{nullptr};


		// Instance
		[[nodiscard("Check the instance status")]] std::expected<void, std::string>
		create_instance(std::string_view application_name = "Default")
		{
			#if 0
			// Instance extensions
			std::vector<VkExtensionProperties> extensions;
			u32                                count{0};
			if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS)
				return std::unexpected("Failed to enumerate instance extension properties");

			extensions.resize(count);

			if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS)
				return std::unexpected("Failed to enumerate instance extension properties");

			// Validator layers
			if (vkEnumerateInstanceLayerProperties(&count, nullptr) != VK_SUCCESS)
				return std::unexpected("Failed to enumerate instance layer properties");


			std::vector<VkLayerProperties> layers;
			layers.resize(count);

			if (vkEnumerateInstanceLayerProperties(&count, layers.data()) != VK_SUCCESS)
				return std::unexpected("Failed to enumerate instance layer properties");


			// Required extensions and layers
			std::vector<const char*> required_extensions{
			  VK_KHR_SURFACE_EXTENSION_NAME,
			  VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
			  // VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME,
			  VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			  VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

			for (const auto& ext : required_extensions)
			{
				bool found = std::ranges::any_of(
				  instance_extensions, [&](auto& e) { return std::string_view(e.extensionName) == ext; });
				if (not found)
					return std::unexpected(std::format("Required instance extension missing: {}", ext));
			}


			// Validator layers
			std::vector<const char*> required_layers{"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_crash_diagnostic"};
			for (const auto& layer : required_layers)
			{
				bool found =
				  std::ranges::any_of(validator_layers, [&](auto& l) { return std::string_view(l.layerName) == layer; });
				if (not found)
					return std::unexpected(std::format("Required validator layer missing: {}", layer));
			}

			// Instance creation

			// Application info
			VkApplicationInfo app_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
			app_info.apiVersion = minimum_apiversion;

			app_info.pApplicationName   = application_name.data();
			app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			app_info.pEngineName        = "Deckard";
#ifndef _DEBUG
			app_info.engineVersion =
			  VK_MAKE_VERSION(deckard_build::build::major, deckard_build::build::minor, deckard_build::build::patch);
#endif

			VkInstanceCreateInfo instance_create{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
			instance_create.pApplicationInfo = &app_info;
			// extensions
			instance_create.enabledExtensionCount   = as<u32>(required_extensions.size());
			instance_create.ppEnabledExtensionNames = required_extensions.data();
			// layers
			instance_create.enabledLayerCount   = as<u32>(required_layers.size());
			instance_create.ppEnabledLayerNames = required_layers.data();

			VkResult result = vkCreateInstance(&instance_create, nullptr, &instance);
			if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
			{
				return std::unexpected("Vulkan driver is incompatible with the application.");
			}
			else if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
			{
				return std::unexpected("Vulkan extension not present.");
			}
			else if (result != VK_SUCCESS or instance == nullptr)
			{
				return std::unexpected(std::format("Failed to create Vulkan instance: {}", string_VkResult(result)));
			}
			#endif
			return {};
		}

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

		void initialize(void* native_window_handle)
		{
			if (auto init = create_instance(); not init)
			{
				dbg::println("Vulkan instance initialization failed: {}", init.error());
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

		~core() { deinitialize(); }
	};


} // namespace deckard::vulkan
