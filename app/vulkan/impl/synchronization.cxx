module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


module deckard.vulkan;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{

	VkSemaphore context::create_semaphore(VkSemaphoreType type) const
	{
		assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be valid to create semaphore");

		const VkSemaphoreTypeCreateInfo type_info{
		  .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		  .semaphoreType = type,
		  .initialValue  = 0,
		};
		const VkSemaphoreCreateInfo semaphore_info{
		  .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		  .pNext = type == VK_SEMAPHORE_TYPE_TIMELINE ? &type_info : nullptr,
		};

		VkSemaphore semaphore{VK_NULL_HANDLE};
		if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &semaphore) != VK_SUCCESS)
			return VK_NULL_HANDLE;
		return semaphore;
	}

	void context::assert_timeline(VkSemaphore semaphore) const
	{
		assert::check(semaphore != VK_NULL_HANDLE, "Timeline semaphore must be valid");
		assert::check(m_device != VK_NULL_HANDLE, "Vulkan device must be valid to use timeline semaphore");
	}

	void context::wait_for_timeline_semaphore(VkSemaphore semaphore, u64 value, u64 timeout) const
	{
		assert_timeline(semaphore);

		const VkSemaphoreWaitInfo wait_info{
		  .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		  .semaphoreCount = 1,
		  .pSemaphores    = &semaphore,
		  .pValues        = &value,
		};
		const auto result = vkWaitSemaphores(m_device, &wait_info, timeout);
		assert::check(result == VK_SUCCESS, "Failed to wait for timeline semaphore");
	}

	[[nodiscard]] u64 context::get_timeline_semaphore_value(VkSemaphore semaphore) const
	{
		assert_timeline(semaphore);

		u64        value  = 0;
		const auto result = vkGetSemaphoreCounterValue(m_device, semaphore, &value);
		assert::check(result == VK_SUCCESS, "Failed to get timeline semaphore value");
		return value;
	}

	void context::signal_timeline_semaphore(VkSemaphore semaphore, u64 value) const
	{
		assert_timeline(semaphore);

		const VkSemaphoreSignalInfo signal_info{
		  .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		  .semaphore = semaphore,
		  .value     = value,
		};
		const auto result = vkSignalSemaphore(m_device, &signal_info);
		assert::check(result == VK_SUCCESS, "Failed to signal timeline semaphore");
	}

	// ############################################################
	// Synchronization ############################################

	std::expected<void, std::string> context::initialize_synchronization()
	{
		if (m_device == VK_NULL_HANDLE)
			return std::unexpected("Vulkan device must be created before initializing synchronization");


		m_in_flight_timeline = create_semaphore(VK_SEMAPHORE_TYPE_TIMELINE);
		m_in_flight_timeline_values.assign(swapchain_image_count, 0);

		m_timeline_counter = 0;
		m_currentframe     = 0;

		for (auto* semaphores : {&m_image_available, &m_rendering_finished})
		{
			semaphores->resize(swapchain_image_count);
			std::ranges::generate(*semaphores, [this] { return create_semaphore(); });
		}

		if (m_in_flight_timeline == VK_NULL_HANDLE
			or std::ranges::any_of(m_image_available, [](VkSemaphore s) { return s == VK_NULL_HANDLE; })
			or std::ranges::any_of(m_rendering_finished, [](VkSemaphore s) { return s == VK_NULL_HANDLE; }))
			return std::unexpected("Failed to create synchronization semaphores");


		return {};
	}

	void context::destroy_synchronization()
	{
		destroy(m_in_flight_timeline, vkDestroySemaphore);

		for (auto* semaphores : {&m_rendering_finished, &m_image_available})
			for (auto& semaphore : *semaphores)
				destroy(semaphore, vkDestroySemaphore);
	}

	std::expected<void, std::string> context::resize_synchronization()
	{
		if (m_device == VK_NULL_HANDLE)
			return std::unexpected("Vulkan device must be created before resizing synchronization");
		destroy_synchronization();
		return initialize_synchronization();
	}

	void context::deinitialize_synchronization()
	{
		if (m_device == VK_NULL_HANDLE)
			return;
		vkDeviceWaitIdle(m_device);
		destroy_synchronization();
	}

} // namespace deckard::vulkan
