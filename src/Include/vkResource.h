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

		struct DepthResources
		{
			VkImage depthImage = VK_NULL_HANDLE;
			VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
			VkImageView depthImageView = VK_NULL_HANDLE;
			VkFormat depthFormat = {};

			const DepthResources& operator=(const DepthResources& other)
			{
				if (this != &other)
				{
					this->depthImage = other.depthImage;
					this->depthImageMemory = other.depthImageMemory;
					this->depthImageView = other.depthImageView;
					this->depthFormat = other.depthFormat;
				}

				return *this;
			}

			void Destroy(VkDevice l_device)
			{
				vkDestroyImageView(l_device, this->depthImageView, nullptr);
				vkDestroyImage(l_device, this->depthImage, nullptr);
				vkFreeMemory(l_device, this->depthImageMemory, nullptr);
			}
		};

		DepthResources CreateDepthResources(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport);



	}

}