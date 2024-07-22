module;
#include <windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

export module deckard.vulkan;
import std;
import deckard;
import deckard.vulkan_helpers;

namespace deckard::vulkan
{

	constexpr u32 VENDOR_NVIDIA = 0x10DE;

	export enum class Severity : u32 {
		Verbose = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
		Info    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
		Warning = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		Error   = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	};

	export enum class Type : u32 {
		General     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
		Validation  = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
		Performance = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	};

	export class vulkan
	{
	public:
		vulkan() = default;

		~vulkan() { deinitialize(); }

		bool initialize() noexcept;

		void deinitialize() noexcept;

		void logme();

	private:
		// instance
		bool initialize_instance();

		// device
		bool initialize_device();

		// Debug stuff
		void vulkan_log(std::string_view str, Severity severity = Severity::Info, Type type = Type::General);

		void initialize_debug_callback();

		static bool debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
								   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		VkInstance               instance{nullptr};
		VkDevice                 device{nullptr};
		VkPhysicalDevice         physicaldevice{nullptr};
		VkDebugUtilsMessengerEXT debug_messenger{nullptr};


		bool is_initialized{false};
	};

	bool vulkan::initialize() noexcept
	{
		if (bool ext_init = enumerate_instance_extensions(); not ext_init)
			return false;
		if (bool layer_init = enumerate_validator_layers(); not layer_init)
			return false;


		if (bool instance_init = initialize_instance(); not instance_init)
			return false;

#ifdef _DEBUG
		initialize_debug_callback();
#endif


		if (bool init_device = initialize_device(); not init_device)
			return false;

		is_initialized = true;
		return is_initialized;
	}

	void vulkan::deinitialize() noexcept
	{

		if (vkDestroyDebugUtilsMessengerEXT != nullptr)
			vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

		vkDestroyInstance(instance, nullptr);
		instance = nullptr;

		is_initialized = false;
	}

	// instance
	bool vulkan::initialize_instance()
	{
		VkApplicationInfo app_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
		app_info.apiVersion = VK_MAKE_API_VERSION(0, 1, 5, 0);
		// VK_API_VERSION_1_1;

		app_info.pApplicationName   = "Deckard";
		app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		app_info.pEngineName        = "Deckard";
#ifndef _DEBUG
		app_info.engineVersion = VK_MAKE_VERSION(deckard_build::build::major, deckard_build::build::minor, deckard_build::build::patch);
#endif
		// extensions

		std::vector<const char*> required_extensions;
#ifdef _DEBUG
		dbg::println("Vulkan extensions({}):", extensions.size());
#endif
		for (const auto extension : extensions)
		{

			std::string_view name = extension.extensionName;

			bool marked = false;

			if (name.compare(VK_KHR_SURFACE_EXTENSION_NAME) == 0)
			{
				marked = true;
				required_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
			}

			if (name.compare(VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
			{
				marked = true;
				required_extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
			}

#ifdef _DEBUG
			if (name.compare(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
			{
				marked = true;
				required_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			}

			if (name.compare(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
			{
				marked = true;
				required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			dbg::println("{:>42}{} (rev {})", name, marked ? "*" : " ", VK_API_VERSION_PATCH(extension.specVersion));

#endif
		}
#ifdef _DEBUG
		dbg::println();
#endif


		// layers
		std::vector<const char*> required_layers;

		dbg::println("Vulkan validators({}):", validator_layers.size());

#ifdef _DEBUG
		for (const auto& layer : validator_layers)
		{
			std::string_view name   = layer.layerName;
			bool             marked = false;
			if (name.compare("VK_LAYER_KHRONOS_validation") == 0)
			{
				marked = true;
				//	required_layers.emplace_back("VK_LAYER_KHRONOS_validation");
			}


			dbg::println(
			  "{:>42}{} ({}.{}.{})",
			  layer.layerName,
			  marked ? "*" : " ",
			  VK_API_VERSION_MAJOR(layer.specVersion),
			  VK_API_VERSION_MINOR(layer.specVersion),
			  VK_API_VERSION_PATCH(layer.specVersion));
		}
		dbg::println();
#endif


		// Instance
		VkInstanceCreateInfo instance_create{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		instance_create.flags            = 0;
		instance_create.pApplicationInfo = &app_info;
		// extensions
		instance_create.enabledExtensionCount   = as<u32>(required_extensions.size());
		instance_create.ppEnabledExtensionNames = required_extensions.data();
		// layers
		instance_create.enabledLayerCount   = as<u32>(required_layers.size());
		instance_create.ppEnabledLayerNames = required_layers.data();

		VkResult result = vkCreateInstance(&instance_create, nullptr, &instance);

		if (result != VK_SUCCESS or instance == nullptr)
		{
			dbg::println("Create vulkan instance failed: {}", result_to_string(result));
			return false;
		}

		return true;
	}

	// ################################
	// device
	bool vulkan::initialize_device()
	{
		//
		u32      device_count{0};
		VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
		if (result != VK_SUCCESS)
		{
			dbg::println("Enumerate physical devices failed: {}", result_to_string(result));

			return false;
		}

		std::vector<VkPhysicalDevice> devices;
		devices.resize(device_count);
		result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

		if (result != VK_SUCCESS)
		{
			dbg::println("{}", result_to_string(result));

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

		std::vector<u32> mem_counts;
		mem_counts.resize(device_count);

		for (u32 i = 0; i < device_count; i++)
		{

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

		const auto& prop = device_properties[best_gpu_index];

		dbg::println("Device: {}", prop.deviceName);


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


		return true;
	}

	/// ##############################################
	// Vulkan debug ##################################

	void vulkan::vulkan_log(std::string_view str, Severity severity, Type type)
	{
#ifdef _DEBUG

		if (instance != nullptr and vkSubmitDebugUtilsMessageEXT != nullptr)
		{
			VkDebugUtilsMessengerCallbackDataEXT cbdata{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};
			cbdata.pMessage = str.data();


			vkSubmitDebugUtilsMessageEXT(
			  instance, as<VkDebugUtilsMessageSeverityFlagBitsEXT>(severity), as<VkDebugUtilsMessageTypeFlagBitsEXT>(type), &cbdata);
		}
#endif
	}

	void vulkan::initialize_debug_callback()
	{
		// debug functions
		initialize_ext_functions(instance);

		// debug
		VkDebugUtilsMessengerCreateInfoEXT create{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		create.messageSeverity =
		  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		create.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debug_callback;
		create.pUserData       = this;


		if (vkCreateDebugUtilsMessengerEXT != nullptr)
			vkCreateDebugUtilsMessengerEXT(instance, &create, nullptr, &debug_messenger);
	}

	bool vulkan::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
								const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		vulkan* self = (vulkan*)pUserData;

		std::string_view severity, type;

		switch (messageSeverity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "Verbose"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "Info"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "Warning"; break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "Error"; break;
			default: severity = "Unknown"; break;
		}

		switch (messageType)
		{
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: type = "General"; break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: type = "Validation"; break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: type = "Performance"; break;
			case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT: type = "Device address binding"; break;
			default: type = "Unknown"; break;
		}

		if (pCallbackData->pMessage)
		{
			dbg::println("Vulkan {}.{}: {}", severity, type, pCallbackData->pMessage);
		}

		return VK_FALSE;
	};
} // namespace deckard::vulkan
