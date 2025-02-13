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

			for (unsigned int i = 0; i < vpdmp.memoryTypeCount; i++)
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


	}
}