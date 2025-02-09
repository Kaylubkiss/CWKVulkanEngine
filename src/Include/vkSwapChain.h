#pragma once

#include <vulkan/vulkan.h>


namespace vk 
{
	struct SwapChain
	{
		VkSwapchainKHR handle = VK_NULL_HANDLE;
		uint32_t imageCount;
		VkImage* images = nullptr;

		void Destroy(VkDevice l_device)
		{
			vkDestroySwapchainKHR(l_device, this->handle, nullptr);

			for (unsigned i = 0; i < imageCount; ++i)
			{
				vkDestroyImage(l_device, images[i], nullptr);
			}
		}
	}
}
