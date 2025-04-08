#pragma once

#include <vulkan/vulkan.h>
#include "vkGlobal.h"
#include "vkBuffer.h"
#include "vkResource.h"
#include "HotReloader.h"

namespace vk 
{

	struct RenderResources
	{
		VkFence inFlightFence = VK_NULL_HANDLE;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

		vk::rsc::DepthResources depthInfo;
		VkRenderPass renderPass = VK_NULL_HANDLE;

		//for window size information;
		VkExtent2D currentExtent = {0,0};

		void Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow);

		/*void RecreatePipeline(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow);*/

		void Destroy(const VkDevice l_device);
	};

	
}