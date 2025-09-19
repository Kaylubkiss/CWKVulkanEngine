

#include "vkDevice.h"
#include "vkInit.h"
#include <stdexcept>
#include <cassert>

namespace vk 
{
	uint32_t Device::GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) {

		for (uint32_t i = 0; i < this->memoryProperties.memoryTypeCount; ++i)
		{
			if ((typeBits & 1) == 1)
			{
				if ((this->memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					return i;
				}
			}

			typeBits >>= 1;
		}


		throw std::runtime_error("couldn't find the requested memory type on device");
	}

	Buffer Device::CreateBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data) 
	{
		Buffer buffer;
		VkResult result;

		buffer.logicalDevice = this->logical;
		buffer.size = static_cast<VkDeviceSize>(size);

		VkBufferCreateInfo bufferCreateInfo = vk::init::BufferCreateInfo(usage, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

		result = vkCreateBuffer(logical, &bufferCreateInfo, nullptr, &buffer.handle);
		assert(result == VK_SUCCESS);

		VkMemoryRequirements			memoryRequirments;
		vkGetBufferMemoryRequirements(logical, buffer.handle, &memoryRequirments);

		VkMemoryAllocateInfo			vmai;
		vmai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vmai.pNext = nullptr;
		vmai.allocationSize = memoryRequirments.size;


		VkPhysicalDeviceMemoryProperties	vpdmp;
		vkGetPhysicalDeviceMemoryProperties(physical, &vpdmp);

		vmai.memoryTypeIndex = Device::GetMemoryType(memoryRequirments.memoryTypeBits, flags);

		result = vkAllocateMemory(logical, &vmai, nullptr, &buffer.memory);
		assert(result == VK_SUCCESS);


		result = vkBindBufferMemory(logical, buffer.handle, buffer.memory, 0);
		assert(result == VK_SUCCESS);


		//fill data buffer --> THIS COULD BE ITS OWN MODULE...
		if (data != NULL)
		{
			vkMapMemory(logical, buffer.memory, 0, buffer.size, 0, &buffer.mappedMemory);
			memcpy(buffer.mappedMemory, data, buffer.size);
			vkUnmapMemory(logical, buffer.memory);
		}

		buffer.SetDescriptor(buffer.size, 0);

		return buffer;
	}

	FramebufferAttachment Device::CreateFramebufferAttachment(const VkViewport& viewport, VkImageUsageFlagBits usage, VkFormat format) 
	{
		VkImageAspectFlags aspectMask = 0;

		if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) 
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		}
		else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) 
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (format == VK_FORMAT_UNDEFINED)
			{
				format = vk::util::findSupportedFormat(Device::physical,
					{ VK_FORMAT_D32_SFLOAT_S8_UINT,
					VK_FORMAT_D32_SFLOAT,
					VK_FORMAT_D24_UNORM_S8_UINT,
					VK_FORMAT_D16_UNORM_S8_UINT,
					VK_FORMAT_D16_UNORM },
					VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
				);
			}

			if (format >= VK_FORMAT_D16_UNORM_S8_UINT)
			{
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}

		}

		assert(format != VK_FORMAT_UNDEFINED);


		FramebufferAttachment nDepthResources = {};
		nDepthResources.format = format;

		//create depth image
		nDepthResources.image = vk::init::CreateImage(Device::physical, Device::logical, 
			(uint32_t)viewport.width, (uint32_t)viewport.height, 1, 
			format, 
			usage | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			nDepthResources.imageMemory);

		//create depth image view 
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = nDepthResources.image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange = { aspectMask, 0, 1, 0, 1 };

		VK_CHECK_RESULT(vkCreateImageView(Device::logical, &viewInfo, nullptr, &nDepthResources.imageView));

		return nDepthResources;

	}

}