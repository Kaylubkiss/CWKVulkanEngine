#pragma once


#include "Common.h"

namespace vk
{
	namespace init 
	{
		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo();

		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo AssemblyInputStateCreateInfo(VkPrimitiveTopology primitiveTopology);

		VkDescriptorSetLayout DescriptorSetLayout(VkDevice logicalDevice);

		std::array<VkVertexInputAttributeDescription, 3> VertexAttributeDescriptions();

		VkInstance CreateInstance(SDL_Window* window);	

		VkRenderPass RenderPass(const VkDevice l_device, const VkFormat& depthFormat);

		VkCommandPool CommandPool(const VkDevice& l_device, VkCommandPoolCreateFlags createFlag);

		VkCommandBuffer CommandBuffer(const VkDevice l_device, const VkCommandPool cmdPool);

		VkSemaphore CreateSemaphore(const VkDevice l_device);

		VkFence CreateFence(const VkDevice l_device);

		VkPipelineLayout CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout);

		VkPipeline CreatePipeline(const VkDevice l_device, const VkPipelineLayout pipelineLayout, const VkRenderPass renderPass, VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const VkShaderModule& shaderModule, VkShaderStageFlagBits stage);

		VkShaderModule ShaderModule(const VkDevice& l_device, const char* filename);

		DepthResources CreateDepthResources(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport);

		VkDescriptorPool DescriptorPool(const VkDevice l_device);

		VkDescriptorSet DescriptorSet(const VkDevice l_device, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscLayout);
	}

}