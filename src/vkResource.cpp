#include "vkResource.h"
#include "vkUtility.h"

namespace vk 
{
	namespace rsc 
	{
		VkImage CreateImage
		(
			const VkPhysicalDevice& p_device, const VkDevice& l_device, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags flags, VkDeviceMemory& imageMemory, uint32_t arrayLayerCount
		)
		{

			VkPhysicalDeviceMemoryProperties	vpdmp;
			vkGetPhysicalDeviceMemoryProperties(p_device, &vpdmp);

			VkImageCreateInfo imageCreateInfo = { };
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.extent.width = width;
			imageCreateInfo.extent.height = height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = arrayLayerCount;
			imageCreateInfo.format = format;
			imageCreateInfo.tiling = tiling;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.usage = usage;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;


			VkImage nImage;
			VK_CHECK_RESULT(vkCreateImage(l_device, &imageCreateInfo, nullptr, &nImage));

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(l_device, nImage, &memRequirements);

			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memRequirements.size;

			for (uint32_t i = 0; i < vpdmp.memoryTypeCount; i++)
			{
				VkMemoryType vmt = vpdmp.memoryTypes[i];
				VkMemoryPropertyFlags vmpf = vmt.propertyFlags;
				if ((memRequirements.memoryTypeBits & (1 << i)) != 0)
				{
					if ((vmpf & flags) != 0)
					{
						memAllocInfo.memoryTypeIndex = i;
						break;
					}
				}
			}

			VK_CHECK_RESULT(vkAllocateMemory(l_device, &memAllocInfo, nullptr, &imageMemory));

			VK_CHECK_RESULT(vkBindImageMemory(l_device, nImage, imageMemory, 0));

			return nImage;

		}


		DepthStencil CreateDepthResources(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport)
		{
			DepthStencil nDepthResources = {};

			nDepthResources.format = vk::util::findSupportedFormat(p_device,
				{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

			//create depth image
			nDepthResources.image = vk::rsc::CreateImage(p_device, l_device, (uint32_t)viewport.width, (uint32_t)viewport.height, 1, nDepthResources.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				nDepthResources.imageMemory, 1);

			//create depth image view 
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = nDepthResources.image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = nDepthResources.format;
			viewInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };

			VK_CHECK_RESULT(vkCreateImageView(l_device, &viewInfo, nullptr, &nDepthResources.imageView));

			return nDepthResources;
		}

	
	}
}