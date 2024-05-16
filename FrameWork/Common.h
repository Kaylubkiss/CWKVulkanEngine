#pragma once
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#include <glm/glm.hpp>

class Application; //forward declare class.


static struct ApplicationManager
{
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();
} appManager;


#define _Application appManager.GetApplication()


struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mappedMemory;

	void* mData;

	void operator=(const Buffer& rhs)
	{
		this->buffer = rhs.buffer;
		this->memory = rhs.memory;
		this->mappedMemory = rhs.mappedMemory;
	}

	//assume that build info is shared among all buffers.
	Buffer(size_t size, void* data);
	Buffer() : buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), mappedMemory(NULL), mData(NULL) {};
};

