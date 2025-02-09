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
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;
		VkFormat depthFormat;
	};

	struct Queue 
	{
		VkQueue handle;
		uint32_t family;
	};

	
}