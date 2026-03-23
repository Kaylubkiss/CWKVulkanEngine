#pragma once

#include "vkUtility.h"
#include "vkInit.h"
#include "vkBuffer.h"
#include "vkDevice.h"
#include "vkDescriptorBuffer.h"

constexpr uint32_t gMaxFramesInFlight = 3;



class UserInterface;

namespace vk
{
	class Buffer;
	class Device;

	//created in response to the need of texture manager. It needs a lot of graphics context state, but the calls to 
	//function methods of the context to get this information seemed inconvenient.
	//in turn, I've had to create this data structure which contains all the information that
	//texture manager needs of the current context.
	//it's a little janky.
	struct GraphicsContextInfo
	{
		vk::Device* devicePtr = nullptr;
		uint32_t textureBindingCount = 0;
		uint32_t objectCount = 0;
	};

	//This allows the user to pass in relevant arguments to draw an object.
	struct DrawInfo
	{
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDeviceSize textureLayoutSize = 0; //used for buffer offset calculation
		uint32_t imageBufferIndex = 0;
		uint32_t firstSet = 0;
		uint32_t setCount = 1; //it would make sense that there is at least 1 set being described.
	};


	VkCommandBuffer beginSingleTimeCommand( const VkDevice l_device, const VkCommandPool cmdPool );

	void endSingleTimeCommand( const VkDevice l_device, VkCommandBuffer commandBuffer,
		const VkCommandPool cmdPool, const VkQueue gfxQueue );
}




