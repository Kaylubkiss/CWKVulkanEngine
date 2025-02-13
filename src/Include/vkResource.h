#pragma once


#include <vulkan/vulkan.h>

namespace vk 
{
	namespace rsc 
	{
		VkImage CreateImage
		(
			const VkPhysicalDevice& p_device, const VkDevice& l_device, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags flags, VkDeviceMemory& imageMemory, uint32_t arrayLayerCount
		);

	}

}