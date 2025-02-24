#pragma once

#include <vulkan/vulkan.h>
#include "vkGlobal.h"
#include "vkBuffer.h"

namespace vk 
{
	struct RenderResources
	{
		uTransformObject uTransform = {};

		VkFence inFlightFence = VK_NULL_HANDLE;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;

		vk::Buffer uniformBuffer;

		VkRenderPass renderPass;

		vk::DepthResources depthInfo;

		//for window size information;
		VkExtent2D currentExtent;

		//pipeline information
		VkDescriptorSetLayout defaultDescriptorSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout defaultPipelineLayout = VK_NULL_HANDLE;
		VkPipeline defaultPipeline = VK_NULL_HANDLE;

		VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule vertexShaderModule = VK_NULL_HANDLE;

		void Allocate(const VkPhysicalDevice p_device, const VkDevice l_device, const vk::Window& appWindow);

		void Destroy(const VkDevice l_device);
	};


}