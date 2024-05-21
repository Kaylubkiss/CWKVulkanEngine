#include "Common.h"
#include "Application.h"

static int s_count = 0;
static typename std::aligned_storage<sizeof(Application), alignof(Application)>::type applicationBuffer;

Application& app = reinterpret_cast<Application&> (applicationBuffer);


ApplicationManager::ApplicationManager()
{
	if (++s_count == 1)
	{
		new (&app) Application();
	}
}

ApplicationManager::~ApplicationManager()
{
	if (--s_count == 0)
	{
		(&app)->~Application();
	}

}


Application* ApplicationManager::GetApplication()
{
	return (&app);
}


Buffer::Buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, void* data)
{
	VkResult result;

	this->mData = data;

	VkPhysicalDeviceMemoryProperties	vpdmp;
	vkGetPhysicalDeviceMemoryProperties(_Application->PhysicalDevice(), &vpdmp);

	VkBufferCreateInfo bufferCreateInfo = {};

	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.size = size;/*sizeof(uTransformObject)*/
	bufferCreateInfo.usage = usage;

	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices = (const uint32_t*)nullptr;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// can only use CONCURRENT if .queueFamilyIndexCount > 0

	result = vkCreateBuffer(_Application->LogicalDevice(), &bufferCreateInfo, nullptr, &this->buffer);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements			memoryRequirments;
	vkGetBufferMemoryRequirements(_Application->LogicalDevice(), this->buffer, &memoryRequirments);

	VkMemoryAllocateInfo			vmai;
	vmai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vmai.pNext = nullptr;
	vmai.allocationSize = memoryRequirments.size;


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


	result = vkAllocateMemory(_Application->LogicalDevice(), &vmai, nullptr, &this->memory);
	assert(result == VK_SUCCESS);


	result = vkBindBufferMemory(_Application->LogicalDevice(), this->buffer, this->memory, 0);		// 0 is the offset
	assert(result == VK_SUCCESS);


	//fill data buffer --> THIS COULD BE ITS OWN MODULE...
	result = vkMapMemory(_Application->LogicalDevice(), this->memory, 0, VK_WHOLE_SIZE, 0, &this->mappedMemory);	// 0 and 0 are offset and flags
	memcpy(this->mappedMemory, this->mData, size);
	vkUnmapMemory(_Application->LogicalDevice(), this->memory);
	assert(result == VK_SUCCESS);

}





