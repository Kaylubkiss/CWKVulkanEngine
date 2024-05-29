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
	VkDeviceSize size;

	void* mappedMemory = NULL;

	void operator=(const Buffer& rhs)
	{
		this->buffer = rhs.buffer;
		this->memory = rhs.memory;
		this->mappedMemory = rhs.mappedMemory;
	}

	/*Buffer(const Buffer& rhs);*/

	//assume that build info is shared among all buffers.
	Buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data);
	Buffer() : buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(0), mappedMemory(NULL) {};
	void FillData(const void* data, size_t dataCount, size_t stride);
	void RecordData();
	void StopRecordData();
	void CopyData(void* data);
	/*~Buffer();*/ //this gets called in std::vector and causes headache. we don't want that.
};



static struct ApplicationManager
{
	ApplicationManager();
	~ApplicationManager();
	Application* GetApplication();
} appManager;


#define _Application appManager.GetApplication()



