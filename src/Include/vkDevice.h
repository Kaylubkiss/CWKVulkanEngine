#pragma once


#include <vulkan/vulkan.h>
#include "vkBuffer.h"

namespace vk 
{
	struct Device {
		
		//data
		VkPhysicalDevice physical = VK_NULL_HANDLE;
		VkDevice logical = VK_NULL_HANDLE;
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;

		VkPhysicalDeviceMemoryProperties memoryProperties;

		//functionality
		uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);
		Buffer CreateBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data);
	};

}

