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

		struct DepthStencil
		{
			VkImage image = VK_NULL_HANDLE;
			VkDeviceMemory imageMemory = VK_NULL_HANDLE;
			VkImageView imageView = VK_NULL_HANDLE;
			VkFormat format = {};

			void Destroy(VkDevice l_device)
			{
				vkDestroyImageView(l_device, this->imageView, nullptr);
				vkDestroyImage(l_device, this->image, nullptr);
				vkFreeMemory(l_device, this->imageMemory, nullptr);
			}
		};

		DepthStencil CreateDepthResources(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport);



	}

}