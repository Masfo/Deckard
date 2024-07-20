module;
#include <windows.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

export module deckard.vulkan;
import std;
import deckard;

namespace deckard::vulkan
{
	PFN_vkCreateDebugUtilsMessengerEXT  vkCreateDebugUtilsMessengerEXT{nullptr};
	PFN_vkSubmitDebugUtilsMessageEXT    vkSubmitDebugUtilsMessageEXT{nullptr};
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};


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

	std::vector<VkExtensionProperties> extensions;
	std::vector<VkLayerProperties>     validator_layers;

	bool enumerate_extensions() noexcept
	{
		u32      count{0};
		VkResult result = vkEnumerateInstanceExtensionProperties(0, &count, 0);

		if (result == VK_SUCCESS)
		{
			extensions.resize(count);
			result = vkEnumerateInstanceExtensionProperties(0, &count, extensions.data());
		}

		return result == VK_SUCCESS;
	}

	bool enumerate_validator_layers() noexcept
	{
		//
		u32      count{0};
		VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (result == VK_SUCCESS)
		{
			validator_layers.resize(count);
			result = vkEnumerateInstanceLayerProperties(&count, validator_layers.data());
		}
		return result == VK_SUCCESS;
	}

	void initialize_ext_functions(VkInstance instance)
	{
		//
		vkCreateDebugUtilsMessengerEXT =
		  (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		vkSubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");

		vkDestroyDebugUtilsMessengerEXT =
		  (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	}

	export class vulkan
	{
	public:
		vulkan() = default;

		~vulkan() { deinitialize(); }

		bool initialize() noexcept
		{
			if (bool ext_init = enumerate_extensions(); not ext_init)
				return false;
			if (bool layer_init = enumerate_validator_layers(); not layer_init)
				return false;


			VkApplicationInfo app_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
			app_info.apiVersion = VK_API_VERSION_1_1;

			app_info.pApplicationName   = "Deckard";
			app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
			app_info.pEngineName        = "Deckard";
#ifndef _DEBUG
			app_info.engineVersion = VK_MAKE_VERSION(deckard_build::build::major, deckard_build::build::minor, deckard_build::build::patch);
#endif
			// extensions

			dbg::println("Vulkan extensions({}):", extensions.size());
			std::vector<const char*> required_extensions;
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

				//
				dbg::println("{:>42}{} (rev {})", name, marked ? "*" : " ", VK_API_VERSION_PATCH(extension.specVersion));

#endif
			}
			dbg::println();


			// layers
			std::vector<const char*> required_layers;

			dbg::println("Vulkan validators({}):", validator_layers.size());

			for (const auto& layer : validator_layers)
			{
				std::string_view name = layer.layerName;
#ifdef _DEBUG
				bool marked = false;
				if (name.compare("VK_LAYER_KHRONOS_validation") == 0)
				{
					marked = true;
					required_layers.emplace_back("VK_LAYER_KHRONOS_validation");
				}

				//
				dbg::println(
				  "{:>42}{} ({}.{}.{})",
				  layer.layerName,
				  marked ? "*" : " ",
				  VK_API_VERSION_MAJOR(layer.specVersion),
				  VK_API_VERSION_MINOR(layer.specVersion),
				  VK_API_VERSION_PATCH(layer.specVersion));

#endif
			}
			dbg::println();


			// Instance
			VkInstanceCreateInfo instance_create{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
			instance_create.flags            = 0;
			instance_create.pApplicationInfo = &app_info;
			// extensions
			instance_create.enabledExtensionCount   = as<u32>(required_extensions.size());
			instance_create.ppEnabledExtensionNames = !required_extensions.empty() ? required_extensions.data() : nullptr;
			// layers
			instance_create.enabledLayerCount   = as<u32>(required_layers.size());
			instance_create.ppEnabledLayerNames = !required_layers.empty() ? required_layers.data() : nullptr;

			VkResult result = vkCreateInstance(&instance_create, nullptr, &instance);

			if (result != VK_SUCCESS or instance == nullptr)
				return false;

			initialize_debug_callback();

			vulkan_log("initialized");

			is_initialized = true;

			return is_initialized;
		}

		void deinitialize() noexcept
		{

			if (vkDestroyDebugUtilsMessengerEXT != nullptr)
				vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

			vkDestroyInstance(instance, nullptr);
			instance = nullptr;

			is_initialized = false;
		}

		void logme() { dbg::println("logme"); }

	private:
		void vulkan_log(std::string_view str, Severity severity = Severity::Info, Type type = Type::General)
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

		void initialize_debug_callback()
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

		static bool debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
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

		VkInstance               instance{};
		VkDebugUtilsMessengerEXT debug_messenger;


		bool is_initialized{false};
	};
} // namespace deckard::vulkan
