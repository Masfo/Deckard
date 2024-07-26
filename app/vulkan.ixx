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
	// TODO: partial modules for vulkan, like math
	// instance, device, debug,	queue, surface
	// all used in main vulkan module
	// void init()
	//   create_instance(), create_device(), create_queue(), create_surface()...

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

		bool initialize(HINSTANCE inst, HWND handle) noexcept;

		void deinitialize() noexcept;

		void logme();

		bool draw();

		void wait();

	private:
		// instance
		bool initialize_instance();

		// surface
		bool initialize_surface();

		// device
		bool initialize_device();

		// Debug stuff
		void vulkan_log(std::string_view str, Severity severity = Severity::Info, Type type = Type::General);

		void initialize_debug_callback();

		static bool debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
								   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		VkInstance                      instance{nullptr};
		VkDevice                        device{nullptr};
		VkPhysicalDevice                physical_device{nullptr};
		VkDebugUtilsMessengerEXT        debug_messenger{nullptr};
		VkSwapchainKHR                  swapchain{nullptr};
		VkQueue                         queue{nullptr};
		VkSurfaceKHR                    presentation_surface{nullptr};
		std::vector<VkSurfaceFormatKHR> surface_formats;

		std::vector<VkCommandBuffer> command_buffers;
		VkCommandPool                command_pool{nullptr};

		std::vector<VkImage>       swapchain_images{};
		std::vector<VkImageView>   swapchain_imageviews{};
		std::vector<VkFramebuffer> swapchain_framebuffers;
		VkExtent2D                 swapchain_extent{};
		VkFormat                   swapchain_format;

		VkRenderPass     renderpass{nullptr};
		VkPipelineLayout pipeline_layout{nullptr};

		VkSemaphore image_available{nullptr};
		VkSemaphore rendering_finished{nullptr};
		VkFence     in_flight{nullptr};

		HINSTANCE window_instance{nullptr};
		HWND      window_handle{nullptr};

		bool is_initialized{false};
	};

	bool vulkan::initialize(HINSTANCE instance, HWND handle) noexcept
	{
		window_instance = instance;
		window_handle   = handle;

		if (bool ext_init = enumerate_instance_extensions(); not ext_init)
			return false;
		if (bool layer_init = enumerate_validator_layers(); not layer_init)
			return false;


		if (bool instance_init = initialize_instance(); not instance_init)
			return false;

		if (bool surface_init = initialize_surface(); not surface_init)
			return false;

#ifdef _DEBUG
		initialize_debug_callback();
#endif


		if (bool init_device = initialize_device(); not init_device)
			return false;

		if (bool enum_de = enumerate_device_extensions(physical_device); not enum_de)
			return false;


		is_initialized = true;
		return is_initialized;
	}

	void vulkan::deinitialize() noexcept
	{

		vkDeviceWaitIdle(device);

		if (device != nullptr)
		{
			if (pipeline_layout != nullptr)
				vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

			if (renderpass != nullptr)
				vkDestroyRenderPass(device, renderpass, nullptr);


			for (auto& view : swapchain_imageviews)
				vkDestroyImageView(device, view, nullptr);

			for (auto& framebuffer : swapchain_framebuffers)
				vkDestroyFramebuffer(device, framebuffer, nullptr);


			if (command_buffers.size() > 0 and command_buffers[0] != nullptr)
			{
				vkFreeCommandBuffers(device, command_pool, as<u32>(command_buffers.size()), command_buffers.data());
				command_buffers.clear();
			}

			if (command_pool != nullptr)
			{
				vkDestroyCommandPool(device, command_pool, nullptr);
				command_pool = nullptr;
			}

			if (in_flight != nullptr)
				vkDestroyFence(device, in_flight, nullptr);


			if (image_available != nullptr)
				vkDestroySemaphore(device, image_available, nullptr);

			if (rendering_finished != nullptr)
				vkDestroySemaphore(device, rendering_finished, nullptr);


			if (swapchain != nullptr)
				vkDestroySwapchainKHR(device, swapchain, nullptr);

			vkDestroyDevice(device, nullptr);
			device = nullptr;
		}

		if (presentation_surface != nullptr)
			vkDestroySurfaceKHR(instance, presentation_surface, nullptr);


		if (instance != nullptr)
		{
			if (vkDestroyDebugUtilsMessengerEXT != nullptr)
				vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);


			vkDestroyInstance(instance, nullptr);
			instance = nullptr;
		}

		is_initialized = false;
	}

	// instance
	bool vulkan::initialize_instance()
	{
		VkApplicationInfo app_info{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
		app_info.apiVersion = VK_API_VERSION_1_1;

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

			if (name.compare(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0)
			{
				marked = true;
				required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			}

#ifdef _DEBUG

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
				required_layers.emplace_back("VK_LAYER_KHRONOS_validation");
			}


#if 0
			if (name.compare("VK_LAYER_LUNARG_crash_diagnostic") == 0)
			{
				marked = true;
				required_layers.emplace_back("VK_LAYER_LUNARG_crash_diagnostic");
			}

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


		instance_create.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

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
	// surface
	bool vulkan::initialize_surface()
	{
		VkWin32SurfaceCreateInfoKHR surface_create_info{.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
		surface_create_info.hinstance = window_instance;
		surface_create_info.hwnd      = window_handle;

		VkResult result = vkCreateWin32SurfaceKHR(instance, &surface_create_info, nullptr, &presentation_surface);
		if (result != VK_SUCCESS)
		{
			dbg::println("Could not create vulkan surface on window: {}", result_to_string(result));
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

		physical_device = devices[best_gpu_index];

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


		// queue
		u32 queue_families_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
		if (queue_families_count == 0)
		{
			dbg::println("Vulkan device has no queue families");
			return false;
		}

		i32                                  queue_index{-1};
		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_families_count);

		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, queue_family_properties.data());

		for (u32 i = 0; i < queue_families_count; i++)
		{
			if ((queue_family_properties[i].queueCount > 0) and (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				queue_index = i;
				break;
			}
		}

		if (queue_index < 0)
		{
			dbg::println("Vulkan device had not required queue family properties");
			return false;
		}

		// queue
		VkDeviceQueueCreateInfo queue_create{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};

		std::array<float, 1> priority{1.0f};
		queue_create.queueCount       = 1;
		queue_create.queueFamilyIndex = queue_index;
		queue_create.pQueuePriorities = &priority[0];


		// device
		std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		VkDeviceCreateInfo device_create{.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
		device_create.queueCreateInfoCount = 1;
		device_create.pQueueCreateInfos    = &queue_create;
		// extensions
		device_create.enabledExtensionCount   = extensions.size();
		device_create.ppEnabledExtensionNames = extensions.data();


		result = vkCreateDevice(physical_device, &device_create, nullptr, &device);

		if (result != VK_SUCCESS)
		{
			dbg::println("Vulkan device creation failed: {}", result_to_string(result));
			return false;
		}

		vkGetDeviceQueue(device, queue_index, 0, &queue);

		// semaphores

		VkSemaphoreCreateInfo semaphore_create{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

		result = vkCreateSemaphore(device, &semaphore_create, nullptr, &image_available);
		if (result != VK_SUCCESS)
		{
			dbg::println("Semaphore creation failed: {}", result_to_string(result));
			return false;
		}

		result = vkCreateSemaphore(device, &semaphore_create, nullptr, &rendering_finished);
		if (result != VK_SUCCESS)
		{
			dbg::println("Semaphore creation failed: {}", result_to_string(result));
			return false;
		}
		VkFenceCreateInfo fence_info{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		result = vkCreateFence(device, &fence_info, nullptr, &in_flight);
		if (result != VK_SUCCESS)
		{
			dbg::println("Fence creation failed: {}", result_to_string(result));
			return false;
		}

		// surface capabilities
		VkSurfaceCapabilitiesKHR surface_capabilities;
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, presentation_surface, &surface_capabilities);
		if (result != VK_SUCCESS)
		{
			dbg::println("Device surface capabilities query failed: {}", result_to_string(result));
			return false;
		}


		// surface formats
		u32 formats_count{0};
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, presentation_surface, &formats_count, nullptr);
		if (result == VK_SUCCESS)
		{
			surface_formats.resize(formats_count);

			result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, presentation_surface, &formats_count, surface_formats.data());
			if (result != VK_SUCCESS)
			{
				dbg::println("Device surface formats query failed: {}", result_to_string(result));
				return false;
			}
		}

		// present modes
		u32                           present_mode_count{0};
		std::vector<VkPresentModeKHR> present_modes;

		result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, presentation_surface, &present_mode_count, nullptr);
		if (result == VK_SUCCESS)
		{
			present_modes.resize(present_mode_count);

			result =
			  vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, presentation_surface, &present_mode_count, present_modes.data());

			if (result != VK_SUCCESS)
			{
				dbg::println("Device surface formats query failed: {}", result_to_string(result));
				return false;
			}
		}


		//
		// swapchain

		VkSwapchainCreateInfoKHR create_swapchain{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};

		u32 desired_number_of_images = surface_capabilities.minImageCount + 1;
		desired_number_of_images =
		  std::clamp(desired_number_of_images, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);

		create_swapchain.surface       = presentation_surface;
		create_swapchain.minImageCount = desired_number_of_images;

		VkSurfaceFormatKHR desired_format{VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
		swapchain_format             = desired_format.format;
		create_swapchain.imageFormat = swapchain_format;

		create_swapchain.imageColorSpace = desired_format.colorSpace;

		// extent
		create_swapchain.imageExtent      = surface_capabilities.currentExtent;
		create_swapchain.imageArrayLayers = 1;

		VkImageUsageFlags usage{VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM};
		if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		create_swapchain.imageUsage = usage;

		create_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		create_swapchain.queueFamilyIndexCount = 0;
		create_swapchain.pQueueFamilyIndices   = nullptr;

		// surface_capability
		VkSurfaceTransformFlagBitsKHR transform{VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR};
		transform                     = surface_capabilities.currentTransform;
		create_swapchain.preTransform = transform;

		create_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		// present mode
		VkPresentModeKHR present_mode{VK_PRESENT_MODE_MAX_ENUM_KHR};

		std::array<VkPresentModeKHR, 2> try_order{};
		try_order[0] = VK_PRESENT_MODE_MAILBOX_KHR;
		try_order[1] = VK_PRESENT_MODE_FIFO_KHR;

		for (const auto& mode : present_modes)
		{
			if (mode == try_order[0])
			{
				present_mode = mode;
				break;
			}
		}

		if (present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR)
			for (const auto& mode : present_modes)
			{
				if (mode == try_order[1])
				{
					present_mode = mode;
					break;
				}
			}
		create_swapchain.presentMode = present_mode;

		// clipped
		create_swapchain.clipped = VK_TRUE;

		VkSwapchainKHR old_swap_chain = swapchain;
		create_swapchain.oldSwapchain = old_swap_chain;

		result = vkCreateSwapchainKHR(device, &create_swapchain, nullptr, &swapchain);
		if (result != VK_SUCCESS)
		{
			dbg::println("Vulkan swapchain creation failed: {}", result_to_string(result));
			return false;
		}

		if (old_swap_chain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, old_swap_chain, nullptr);
		}

		// extent
		swapchain_extent = surface_capabilities.currentExtent;


		// swapchain images
		u32 swapchain_image_count{};
		result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nullptr);
		if (result == VK_SUCCESS)
		{
			swapchain_images.resize(swapchain_image_count);
			result = vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, swapchain_images.data());
			if (result != VK_SUCCESS)
			{
				dbg::println("Getting Vulkan swapchain images failed: {}", result_to_string(result));
				return false;
			}
		}
		else
		{
			dbg::println("Getting Vulkan swapchain images failed: {}", result_to_string(result));
			return false;
		}

		// swapchain imageviews
		swapchain_imageviews.resize(swapchain_image_count);
		for (size_t i = 0; i < swapchain_images.size(); ++i)
		{
			VkImageViewCreateInfo iv_create{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
			iv_create.image    = swapchain_images[i];
			iv_create.viewType = VK_IMAGE_VIEW_TYPE_2D;

			// TODO: better format getter
			iv_create.format = desired_format.format;

			iv_create.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			iv_create.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			iv_create.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			iv_create.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			iv_create.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			iv_create.subresourceRange.baseMipLevel   = 0;
			iv_create.subresourceRange.levelCount     = 1;
			iv_create.subresourceRange.baseArrayLayer = 0;
			iv_create.subresourceRange.layerCount     = 1;

			result = vkCreateImageView(device, &iv_create, nullptr, &swapchain_imageviews[i]);
			if (result != VK_SUCCESS)
			{
				dbg::println("Vulkan create image view for swapchain failed: {}", result_to_string(result));
				return false;
			}
		}

		// renderpass
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format         = swapchain_format;
		colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments    = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass    = 0;
		dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments    = &colorAttachment;
		renderPassInfo.subpassCount    = 1;
		renderPassInfo.pSubpasses      = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies   = &dependency;

		result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderpass);
		if (result != VK_SUCCESS)
		{
			dbg::println("Vulkan create render pass failed: {}", result_to_string(result));
			return false;
		}


		// framebuffers
		swapchain_framebuffers.resize(swapchain_imageviews.size());

		for (size_t i = 0; i < swapchain_imageviews.size(); ++i)
		{
			VkImageView attachments[] = {swapchain_imageviews[i]};

			VkFramebufferCreateInfo frambuffer_create{.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};

			// frambuffer_create.renderPass = renderpass;

			frambuffer_create.width  = swapchain_extent.width;
			frambuffer_create.height = swapchain_extent.height;
			frambuffer_create.layers = 1;

			frambuffer_create.attachmentCount = 1;
			frambuffer_create.pAttachments    = attachments;
		}


		// command buffers
		VkCommandPoolCreateInfo cmd_pool_create{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
		cmd_pool_create.queueFamilyIndex = queue_index;
		cmd_pool_create.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		result = vkCreateCommandPool(device, &cmd_pool_create, nullptr, &command_pool);
		if (result != VK_SUCCESS)
		{
			dbg::println("Command pool creation failed: {}", result_to_string(result));
			return false;
		}

		//

		u32 image_count{0};
		result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
		command_buffers.resize(image_count);

		VkCommandBufferAllocateInfo command_buffer_allocate{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
		command_buffer_allocate.commandPool        = command_pool;
		command_buffer_allocate.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate.commandBufferCount = image_count;

		result = vkAllocateCommandBuffers(device, &command_buffer_allocate, command_buffers.data());
		if (result != VK_SUCCESS)
		{
			dbg::println("Failed to allocate command buffers: {}", result_to_string(result));
			return false;
		}

		// record
		std::vector<VkImage> swap_chain_images(image_count);

		result = vkGetSwapchainImagesKHR(device, swapchain, &image_count, swap_chain_images.data());

		VkCommandBufferBeginInfo cmd_buffer_begin{
		  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT};

		VkClearColorValue clear_color{0.0f, 0.5f, 0.75f, 1.0f};

		VkImageSubresourceRange image_subresource_range = {
		  VK_IMAGE_ASPECT_COLOR_BIT,
		  0, // base mip-level
		  1, // level-count
		  0, // base array layer
		  1  // layer count
		};

		// TODO: no reuse of command yet, record per frame
		// render pass, framebuffer
		// viewport, vkCmdDraw

		for (u32 i = 0; i < image_count; ++i)
		{
			VkImageMemoryBarrier barrier_from_present_to_clear = {
			  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			  nullptr,
			  VK_ACCESS_MEMORY_READ_BIT,
			  VK_ACCESS_TRANSFER_WRITE_BIT,
			  VK_IMAGE_LAYOUT_UNDEFINED,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			  VK_QUEUE_FAMILY_IGNORED,
			  VK_QUEUE_FAMILY_IGNORED,
			  swap_chain_images[i],
			  image_subresource_range};

			VkImageMemoryBarrier barrier_from_clear_to_present = {
			  VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			  nullptr,
			  VK_ACCESS_TRANSFER_WRITE_BIT,
			  VK_ACCESS_MEMORY_WRITE_BIT,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			  VK_QUEUE_FAMILY_IGNORED,
			  VK_QUEUE_FAMILY_IGNORED,
			  swap_chain_images[i],
			  image_subresource_range};

			result = vkBeginCommandBuffer(command_buffers[i], &cmd_buffer_begin);

#if 1
			vkCmdPipelineBarrier(
			  command_buffers[i],
			  VK_PIPELINE_STAGE_TRANSFER_BIT,
			  VK_PIPELINE_STAGE_TRANSFER_BIT,
			  0,
			  0,
			  nullptr,
			  0,
			  nullptr,
			  1,
			  &barrier_from_present_to_clear);
#endif

			//
			vkCmdClearColorImage(
			  command_buffers[i], swap_chain_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);

#if 1

			vkCmdPipelineBarrier(
			  command_buffers[i],
			  VK_PIPELINE_STAGE_TRANSFER_BIT,
			  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			  0,
			  0,
			  nullptr,
			  0,
			  nullptr,
			  1,
			  &barrier_from_clear_to_present);
#endif

			result = vkEndCommandBuffer(command_buffers[i]);
			if (result != VK_SUCCESS)
			{
				dbg::println("cmd buffer failed");
				return false;
			}
		}


		return true;
	}

	bool vulkan::draw()
	{
		VkResult result{};
		result = vkWaitForFences(device, 1, &in_flight, VK_TRUE, UINT64_MAX);
		result = vkResetFences(device, 1, &in_flight);


		// VkResult result = vkWaitForFences(device, 1, &in_flight, VK_TRUE, UINT64_MAX);
		// if (result != VK_SUCCESS)
		//{
		//	return false;
		// }
		//
		u32 image_index{0};
		result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available, nullptr, &image_index);
		switch (result)
		{
			case VK_SUCCESS: [[fallthrough]];
			case VK_TIMEOUT: [[fallthrough]];
			case VK_SUBOPTIMAL_KHR: break;

			case VK_ERROR_OUT_OF_DATE_KHR:
			{
				dbg::println("Resize surface...?");
				return false;
			}
			default:
			{
				dbg::println("something wrong on swapchain image acquisition");
				return false;
			}
		}
		VkPipelineStageFlags wait_dest_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		VkSubmitInfo submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};

		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores    = &image_available;

		submit_info.pWaitDstStageMask  = &wait_dest_stage_mask;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &command_buffers[image_index];

		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores    = &rendering_finished;


		result = vkQueueSubmit(queue, 1, &submit_info, in_flight);
		if (result != VK_SUCCESS)
			return false;


		VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};

		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores    = &rendering_finished;

		present_info.swapchainCount = 1;
		present_info.pSwapchains    = &swapchain;
		present_info.pImageIndices  = &image_index;

		result = vkQueuePresentKHR(queue, &present_info);
		if (result != VK_SUCCESS)
			return false;

		return true;
	}

	void vulkan::wait() { vkDeviceWaitIdle(device); }

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

		u32 severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		//
		// debug
		VkDebugUtilsMessengerCreateInfoEXT create{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
		create.messageSeverity = severity;
		create.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
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
