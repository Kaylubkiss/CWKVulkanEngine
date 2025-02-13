#pragma once


#include <vulkan/vulkan.h>
#include "vkGraphicsSystem.h"

namespace vk 
{
	
	struct RenderResources
	{
		VkRenderPass renderPass;
		VkFence inFlightFence = VK_NULL_HANDLE;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		
		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

		VkFramebuffer* frameBuffer = nullptr;
	};



}