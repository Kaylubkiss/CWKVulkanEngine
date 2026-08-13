#ifndef VK_GLOBAL_HPP
#define VK_GLOBAL_HPP

#include "vkDevice.h"

constexpr uint32_t gMaxFramesInFlight = 3;

namespace vk
{

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
}

#endif




