#ifndef VK_GLOBAL_HPP
#define VK_GLOBAL_HPP

#include "vkDevice.h"

constexpr uint32_t gMaxFramesInFlight = 3;

namespace vk
{

	//created in response to the need of texture manager. It needs a lot of graphics context state, but the calls to 
	//function methods of the context to get this information seemed inconvenient.
	//in turn, I've had to create this data structure which contains all the information that
	//texture manager needs of the current context.
	//it's a little janky.
	struct GraphicsContextInfo
	{
		vk::Device* devicePtr = nullptr;
	};

	//This allows the user to pass in relevant arguments to draw an object.
	struct DrawInfo
	{
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDeviceSize textureBindingSize = 0; //used for buffer offset calculation
		uint32_t imageBufferIndex = 0;
		uint32_t firstSet = 0;
		uint32_t setCount = 1; //it would make sense that there is at least 1 set being described.
	};

	struct TextureCreateInfo
	{
		std::string fileName; //(note as of 5.9.26: will represent a full file path for now)
		VkImageUsageFlags imageUsage;
		VkImageCreateFlags flags;
		VkFormat format = VK_FORMAT_UNDEFINED;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t layerCount = 0;
		uint32_t mipLevels = 0;
	};

}

#endif




