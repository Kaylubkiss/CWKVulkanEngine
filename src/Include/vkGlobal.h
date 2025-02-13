#pragma once

#include "Common.h"


namespace vk
{
	const char* enabledLayerNames[1] =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const char* deviceExtensions[1] =
	{
		"VK_KHR_swapchain"
	};

	struct DepthResources
	{
		VkImage image;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkFormat format;
	};

	struct Queue 
	{
		VkQueue handle;
		uint32_t family;
	};

	VkCommandBuffer beginCmd(const VkDevice l_device, const VkCommandPool cmdPool);

	void endCmd(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue);


}