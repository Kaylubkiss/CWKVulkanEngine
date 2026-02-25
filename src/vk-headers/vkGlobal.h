#pragma once

#include "vkBuffer.h"
#include "vkDevice.h"

constexpr uint32_t gMaxFramesInFlight = 3;

class UserInterface;

namespace vk
{
	class Buffer;
	class Device;

	struct DescriptorBufferData //248 BYTES!!!
	{
		//per-frame resources need to be independently updated according to which frame is active.
		//1st index of "buffers" is the minimum ever used by a descriptor buffer.
		std::array<vk::Buffer, gMaxFramesInFlight> buffers;

		//at least 1 binding (binding 0)
		std::vector<VkDeviceSize> binding_offsets = { 0ull };
		
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		VkDevice c_device            = VK_NULL_HANDLE;

		VkDeviceSize size = 0ull;

		void Destroy()
		{
			if (c_device != VK_NULL_HANDLE)
			{
				for (auto& b : buffers)
				{
					b.Destroy();
				}
				vkDestroyDescriptorSetLayout(c_device, layout, nullptr);
			}
		}
	};

	//created in response to the need of texture manager. It needs a lot of graphics context state, but the calls to 
	//function methods of the context to get this information seemed inconvenient.
	//in turn, I've had to create this data structure which contains all the information that
	//texture manager needs of the current context.
	//it's a little janky.
	struct GraphicsContextInfo
	{
		vk::Device* devicePtr = nullptr;
		UserInterface* contextUIPtr = nullptr;
		DescriptorBufferData* contextTextureDescriptorPtr = nullptr;
		~GraphicsContextInfo()
		{
			devicePtr = nullptr;
			contextUIPtr = nullptr;
			contextTextureDescriptorPtr = nullptr;
		};
	};

	//This allows the user to pass in relevant arguments to draw an object.
	struct DrawInfo
	{
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDeviceSize bufferOffset = 0; //just in case I ever decide to make a more compact buffer.
		VkDeviceSize textureBindingSize = 0; //used for buffer offset calculation
		uint32_t imageBufferIndex = 0;
		uint32_t firstSet = 0;
		uint32_t setCount = 1; //it would make sense that there is at least 1 set being described.
	};


	VkCommandBuffer beginSingleTimeCommand( const VkDevice l_device, const VkCommandPool cmdPool );

	void endSingleTimeCommand( const VkDevice l_device, VkCommandBuffer commandBuffer,
		const VkCommandPool cmdPool, const VkQueue gfxQueue );
}




