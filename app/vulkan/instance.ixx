module;
#include <Windows.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

export module deckard.vulkan:instance;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

#ifndef _DEBUG
import deckard_build;
#endif

namespace deckard::vulkan
{
	bool enumerate_instance_extensions(std::vector<VkExtensionProperties>& extensions)
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

	bool enumerate_validator_layers(std::vector<VkLayerProperties>& layers)
	{
		u32      count{0};
		VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
		if (result == VK_SUCCESS)
		{

			layers.resize(count);
			result = vkEnumerateInstanceLayerProperties(&count, layers.data());
		}
		return result == VK_SUCCESS;
	}

	export class instance
	{
	private:
		std::vector<VkLayerProperties>     validator_layers;
		std::vector<VkExtensionProperties> instance_extensions;

		VkInstance m_instance{nullptr};

	public:
		bool initialize()
		{

			if (bool ext_init = enumerate_instance_extensions(instance_extensions); not ext_init)
				return false;
			if (bool layer_init = enumerate_validator_layers(validator_layers); not layer_init)
				return false;

			VkApplicationInfo app_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
			app_info.apiVersion = VK_API_VERSION_1_3;

			app_info.pApplicationName   = "Deckard";
			app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
			app_info.pEngineName        = "Deckard";
#ifndef _DEBUG
			app_info.engineVersion = VK_MAKE_VERSION(deckard_build::build::major, deckard_build::build::minor, deckard_build::build::patch);
#endif
			// extensions

			std::vector<const char*> required_extensions;
#ifdef _DEBUG
			dbg::println("Vulkan instance extensions({}):", instance_extensions.size());
#endif
			for (const auto extension : instance_extensions)
			{

				const std::string_view name = extension.extensionName;

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

				if(name.compare(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
				{
					marked = true;
					required_extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
				}	


#if 0
			if (name.compare(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) == 0)
			{
				marked = true;
				required_extensions.emplace_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
			}
#endif

#ifdef _DEBUG
			#if 0
				if (name.compare(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
				{
					marked = true;
					required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
				}
				#endif

				if (name.compare(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
				{
					marked = true;
					required_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
				}


				dbg::println("{:>48}{} (rev {})", name, marked ? "*" : " ", VK_API_VERSION_PATCH(extension.specVersion));

#endif
			}
#ifdef _DEBUG
			dbg::println();
#endif


			// validator layers
			std::vector<const char*> required_layers;

#ifdef _DEBUG
			dbg::println("Vulkan validators({}):", validator_layers.size());

			for (const auto& layer : validator_layers)
			{
				std::string_view name   = layer.layerName;
				bool             marked = false;
				if (name.compare("VK_LAYER_KHRONOS_validation") == 0)
				{
					marked = true;
					required_layers.emplace_back("VK_LAYER_KHRONOS_validation");
				}


				if (name.compare("VK_LAYER_LUNARG_crash_diagnostic") == 0)
				{
					marked = true;
					required_layers.emplace_back("VK_LAYER_LUNARG_crash_diagnostic");
				}
#if 0
				if (name.compare("VK_LAYER_LUNARG_monitor") == 0)
				{
					marked = true;
					required_layers.emplace_back("VK_LAYER_LUNARG_monitor");
				}
#endif

				dbg::println(
				  "{:>48}{} ({}.{}.{})",
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


			// instance_create.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
			instance_create.pApplicationInfo = &app_info;
			// extensions
			instance_create.enabledExtensionCount   = as<u32>(required_extensions.size());
			instance_create.ppEnabledExtensionNames = required_extensions.data();
			// layers
			instance_create.enabledLayerCount   = as<u32>(required_layers.size());
			instance_create.ppEnabledLayerNames = required_layers.data();

			VkResult result = vkCreateInstance(&instance_create, nullptr, &m_instance);

			if (result != VK_SUCCESS or m_instance == nullptr)
			{
				dbg::println("Create vulkan instance failed: {}", string_VkResult(result));
				return false;
			}


			return true;
		}

		void deinitialize()
		{
			if (m_instance != nullptr)
			{
				vkDestroyInstance(m_instance, nullptr);
				m_instance = nullptr;
			}
		}

		operator VkInstance() const { return m_instance; }
	};

} // namespace deckard::vulkan
