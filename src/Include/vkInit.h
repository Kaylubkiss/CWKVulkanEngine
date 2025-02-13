#pragma once


#include "Common.h"

namespace vk
{
	namespace init 
	{
		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo AssemblyInputStateCreateInfo(VkPrimitiveTopology primitiveTopology);

		VkDescriptorSetLayout CreateDescriptorSetLayout();

		std::array<VkVertexInputAttributeDescription, 3> VertexAttributeDescriptions();

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo();

		VkInstance CreateInstance(SDL_Window* window);	

		VkRenderPass CreateRenderPass(const VkDevice l_device, const VkFormat& depthFormat);

		VkCommandPool CreateCommandPool(const VkDevice& l_device, VkCommandPoolCreateFlags createFlag);

		VkSemaphore CreateSemaphore(const VkDevice l_device);
	}

}