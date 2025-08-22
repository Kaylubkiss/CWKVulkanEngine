

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

}