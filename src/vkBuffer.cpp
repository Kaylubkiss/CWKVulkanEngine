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

		VkBufferCreateInfo bufferCreateInfo = vk::init::BufferCreateInfo(usage, this->size);
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

		if (data != nullptr)
		{
			Buffer::Map();
			
			if (this->mappedMemory != nullptr) 
			{
				memcpy(this->mappedMemory, data, this->size);
			}

			if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				Buffer::Flush();
			}

			Buffer::UnMap();
		}

		Buffer::SetDescriptor(this->size, 0);

		result = vkBindBufferMemory(l_device, this->handle, this->memory, 0);
		assert(result == VK_SUCCESS);
	}

	void Buffer::SetDescriptor(VkDeviceSize size, VkDeviceSize offset) 
	{
		descriptor.buffer = this->handle;
		descriptor.range = size;
		descriptor.offset = offset;
	}


	void Buffer::Map() 
	{
		if (this->mappedMemory == nullptr) 
		{
			VK_CHECK_RESULT(vkMapMemory(this->logicalDevice, this->memory, 0, VK_WHOLE_SIZE, 0, &this->mappedMemory));
		}
	}

	void Buffer::Flush() 
	{
		VkMappedMemoryRange mappedRange = vk::init::MappedMemoryRange();
		mappedRange.memory = this->memory;
		mappedRange.offset = 0;
		mappedRange.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(this->logicalDevice, 1, &mappedRange);
	}

	void Buffer::UnMap() 
	{
		if (this->mappedMemory != nullptr) 
		{
			vkUnmapMemory(this->logicalDevice, this->memory);
			this->mappedMemory = nullptr;
		}

	}

	void Buffer::Destroy() 
	{
		assert(logicalDevice != VK_NULL_HANDLE);

		if (this->memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(logicalDevice, this->memory, nullptr);
			this->memory = VK_NULL_HANDLE;
		}

		if (this->handle != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(logicalDevice, this->handle, nullptr);
			this->handle = VK_NULL_HANDLE;
		}
	}
}