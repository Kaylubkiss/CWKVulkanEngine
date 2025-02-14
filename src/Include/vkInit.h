#pragma once


#include "Common.h"

namespace vk
{
	namespace init 
	{
		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo AssemblyInputStateCreateInfo(VkPrimitiveTopology primitiveTopology);

		VkDescriptorSetLayout DescriptorSetLayout(VkDevice logicalDevice);

		std::array<VkVertexInputAttributeDescription, 3> VertexAttributeDescriptions();

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo();

		VkInstance CreateInstance(SDL_Window* window);	

		VkRenderPass RenderPass(const VkDevice l_device, const VkFormat& depthFormat);

		VkCommandPool CreateCommandPool(const VkDevice& l_device, VkCommandPoolCreateFlags createFlag);

		VkSemaphore CreateSemaphore(const VkDevice l_device);

		VkFence CreateFence(const VkDevice l_device);

		VkPipelineLayout CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout);

		VkPipeline CreatePipeline(const VkDevice l_device, const VkPipelineLayout pipelineLayout, const VkRenderPass renderPass, VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const VkShaderModule& shaderModule, VkShaderStageFlagBits stage);

		VkShaderModule ShaderModule(const VkDevice l_device, const char* filename);

		DepthResources CreateDepthResources(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport);
	}

}