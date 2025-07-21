#pragma once

#include <vulkan/vulkan.h>
#include "vkGlobal.h"


namespace vk 
{

	struct RenderResources
	{
		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> commandBuffers;

		//for window size information;
		VkExtent2D currentExtent = {0,0};

		void Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow);

		void Destroy(const VkDevice l_device);
	};

	
}