#pragma once

#include <vulkan/vulkan.h>

namespace vk 
{
	struct Buffer
	{
		VkBuffer handle;
		VkDeviceMemory memory;
		VkDeviceSize size;

		void* mappedMemory = NULL;

		bool isAllocated = false;  

		//shallow copy
		void operator=(const Buffer& rhs)
		{
			//note: these are pointers!!!
			this->handle = rhs.handle;
			this->memory = rhs.memory;

			this->size = rhs.size;
			this->mappedMemory = rhs.mappedMemory;
		}

		//assume that build info is shared among all buffers.
		Buffer(VkPhysicalDevice p_device, VkDevice l_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data);
		Buffer() : handle(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(0), mappedMemory(NULL) {};

		void Destroy(const VkDevice l_device);
	};
}


