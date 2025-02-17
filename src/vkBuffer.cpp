#include "Application.h"
#include "vkBuffer.h"
#include <cassert>
#include <memory>

namespace vk 
{
	Buffer::Buffer(VkPhysicalDevice p_device, VkDevice l_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data)
	{

		VkResult result;

		this->size = static_cast<VkDeviceSize>(size);

		VkBufferCreateInfo bufferCreateInfo = {};

		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;

		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = (const uint32_t*)nullptr;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

		result = vkCreateBuffer(l_device, &bufferCreateInfo, nullptr, &this->handle);
		assert(result == VK_SUCCESS);

		VkMemoryRequirements			memoryRequirments;
		vkGetBufferMemoryRequirements(l_device, this->handle, &memoryRequirments);

		VkMemoryAllocateInfo			vmai;
		vmai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		vmai.pNext = nullptr;
		vmai.allocationSize = memoryRequirments.size;



		VkPhysicalDeviceMemoryProperties	vpdmp;
		vkGetPhysicalDeviceMemoryProperties(p_device, &vpdmp);

		for (unsigned int i = 0; i < vpdmp.memoryTypeCount; i++)
		{
			VkMemoryType vmt = vpdmp.memoryTypes[i];
			VkMemoryPropertyFlags vmpf = vmt.propertyFlags;
			if ((memoryRequirments.memoryTypeBits & (1 << i)) != 0)
			{
				if ((vmpf & flags) != 0)
				{
					vmai.memoryTypeIndex = i;
					break;
				}
			}
		}


		result = vkAllocateMemory(l_device, &vmai, nullptr, &this->memory);
		assert(result == VK_SUCCESS);


		result = vkBindBufferMemory(l_device, this->handle, this->memory, 0);
		assert(result == VK_SUCCESS);


		//fill data buffer --> THIS COULD BE ITS OWN MODULE...
		if (data != NULL)
		{
			vkMapMemory(l_device, this->memory, 0, VK_WHOLE_SIZE, 0, &this->mappedMemory);
			memcpy(this->mappedMemory, data, this->size);
			vkUnmapMemory(l_device, this->memory);
		}
	}


	void Buffer::Destroy(const VkDevice l_device) 
	{
		vkFreeMemory(l_device, this->memory, nullptr);
		vkDestroyBuffer(l_device, this->handle, nullptr);
	}
}