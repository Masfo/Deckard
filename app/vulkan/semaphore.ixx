module;
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>


export module deckard.vulkan:semaphore;
import deckard.vulkan_helpers;

import std;
import deckard.debug;
import deckard.types;
import deckard.as;
import deckard.assert;

namespace deckard::vulkan
{
	export class semaphore
	{

	private:
		VkSemaphore m_semaphore{nullptr};

	public:
		bool initialize(VkDevice device)
		{
			VkSemaphoreCreateInfo semaphore_create{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

			VkResult result = vkCreateSemaphore(device, &semaphore_create, nullptr, &m_semaphore);
			if (result != VK_SUCCESS)
			{
				dbg::println("Semaphore creation failed: {}", string_VkResult(result));
				return false;
			}
			return true;
		}

		void deinitialize(VkDevice device)
		{

			if (m_semaphore != nullptr)
				vkDestroySemaphore(device, m_semaphore, nullptr);
		}

		operator VkSemaphore() const { return m_semaphore; }
	};


	export class timeline_semaphore
	{
	public:
		bool initialize(VkDevice device, u64 initial_value = 0)
		{
			VkSemaphoreTypeCreateInfo type_create{
			  .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			  .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			  .initialValue  = initial_value,
			};

			VkSemaphoreCreateInfo semaphore_create{
			  .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			  .pNext = &type_create,
			};

			VkResult result = vkCreateSemaphore(device, &semaphore_create, nullptr, &m_semaphore);
			if (result != VK_SUCCESS)
			{
				dbg::println("Timeline semaphore creation failed: {}", string_VkResult(result));
				return false;
			}
			return true;
		}

		// Blocks the calling thread until the semaphore's counter reaches `value`.
		void wait(VkDevice device, u64 value, u64 timeout = UINT64_MAX) const
		{
			assert::check(device != nullptr);
			assert::check(m_semaphore != nullptr);

			VkSemaphoreWaitInfo wait_info{
			  .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			  .semaphoreCount = 1,
			  .pSemaphores    = &m_semaphore,
			  .pValues        = &value,
			};

			VkResult result = vkWaitSemaphores(device, &wait_info, timeout);
			assert::check(result == VK_SUCCESS or result == VK_TIMEOUT);
		}

		// Current value of the semaphore's counter, as last known by the host.
		[[nodiscard]] u64 value(VkDevice device) const
		{
			assert::check(device != nullptr);
			assert::check(m_semaphore != nullptr);

			u64      counter{0};
			VkResult result = vkGetSemaphoreCounterValue(device, m_semaphore, &counter);
			assert::check(result == VK_SUCCESS);
			return counter;
		}

		// Host-side signal, useful for CPU/GPU handoff without a queue submit.
		void signal(VkDevice device, u64 value) const
		{
			assert::check(device != nullptr);
			assert::check(m_semaphore != nullptr);

			VkSemaphoreSignalInfo signal_info{
			  .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			  .semaphore = m_semaphore,
			  .value     = value,
			};

			VkResult result = vkSignalSemaphore(device, &signal_info);
			assert::check(result == VK_SUCCESS);
		}

		void deinitialize(VkDevice device)
		{
			if (m_semaphore != nullptr)
				vkDestroySemaphore(device, m_semaphore, nullptr);
		}

		operator VkSemaphore() const { return m_semaphore; }

	private:
		VkSemaphore m_semaphore{nullptr};
	};


} // namespace deckard::vulkan
