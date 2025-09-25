#pragma once

#include <vulkan/vulkan.h>

namespace vk 
{
	struct Buffer
	{

		VkDevice logicalDevice = VK_NULL_HANDLE;

		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		

		VkDescriptorBufferInfo descriptor = {};

		void* mappedMemory = NULL;

		//assume that build info is shared among all buffers.
		Buffer() = default;
		~Buffer() = default;

		Buffer(VkPhysicalDevice p_device, VkDevice l_device, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data);

		void Destroy();

		void SetDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		void Map();

		void Flush();

		void UnMap();

		//TODO: void Map();

	};
}


