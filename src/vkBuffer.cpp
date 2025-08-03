#include "Application.h"
#include "vkBuffer.h"
#include <cassert>
#include <memory>

namespace vk 
{
	Buffer::Buffer(VkPhysicalDevice p_device, VkDevice l_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data)
	{
		VkResult result;

		this->logicalDevice = l_device;
		this->size = static_cast<VkDeviceSize>(size);

		VkBufferCreateInfo bufferCreateInfo = vk::init::BufferCreateInfo(usage, size);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

		result = vkCreateBuffer(l_device, &bufferCreateInfo, nullptr, &this->handle);
		assert(result == VK_SUCCESS);

		VkMemoryRequirements			memoryRequirments;
		vkGetBufferMemoryRequirements(l_device, this->handle, &memoryRequirments);

		VkMemoryAllocateInfo vmai = vk::init::MemoryAllocateInfo();
		vmai.allocationSize = memoryRequirments.size;

		VkPhysicalDeviceMemoryProperties	vpdmp;
		vkGetPhysicalDeviceMemoryProperties(p_device, &vpdmp);

		uint32_t typeBits = memoryRequirments.memoryTypeBits;
		for (uint32_t i = 0; i < vpdmp.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((vpdmp.memoryTypes[i].propertyFlags & flags) == flags)
				{
					vmai.memoryTypeIndex = i;
					break;
				}
			}
			typeBits >>= 1;
		}


		result = vkAllocateMemory(l_device, &vmai, nullptr, &this->memory);
		assert(result == VK_SUCCESS);


		result = vkBindBufferMemory(l_device, this->handle, this->memory, 0);
		assert(result == VK_SUCCESS);


		//fill data buffer --> THIS COULD BE ITS OWN MODULE...
		if (data != NULL)
		{
			vkMapMemory(l_device, this->memory, 0, this->size, 0, &this->mappedMemory);
			memcpy(this->mappedMemory, data, this->size);

			if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange mappedRange = vk::init::MappedMemoryRange();
				mappedRange.memory = this->memory;
				mappedRange.offset = 0;
				mappedRange.size = this->size;
				vkFlushMappedMemoryRanges(l_device, 1, &mappedRange);
			}
			vkUnmapMemory(l_device, this->memory);
		}

		Buffer::SetDescriptor(this->size, 0);
	}

	void Buffer::SetDescriptor(VkDeviceSize size, VkDeviceSize offset) 
	{
		descriptor.buffer = this->handle;
		descriptor.range = size;
		descriptor.offset = offset;
	}

	void Buffer::Destroy() 
	{
		assert(logicalDevice != VK_NULL_HANDLE);
		vkFreeMemory(logicalDevice, this->memory, nullptr);
		vkDestroyBuffer(logicalDevice, this->handle, nullptr);
	}
}