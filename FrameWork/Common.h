#pragma once
//IF ENABLED: this means older versions of code compiled with mvsc from before 2017 will probably not be compatible.
//IF DISABLED: alignment may not be correct. For now, I'm willing to let this happen until something goes bad.
#define _DISABLE_EXTENDED_ALIGNED_STORAGE 
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

class Application; //forward declare class.

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
	Buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data);
	Buffer() : buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), mappedMemory(NULL), mData(NULL) {};
};

//struct Texture 
//{
//
//	Texture() {}
//};


static struct ApplicationManager
{
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();
} appManager;


#define _Application appManager.GetApplication()



