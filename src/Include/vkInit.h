#pragma once


#include <vulkan/vulkan.h>
#include "vkResource.h"

namespace vk
{
	namespace init 
	{
		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo();

		VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo AssemblyInputStateCreateInfo(VkPrimitiveTopology primitiveTopology);

		VkDescriptorSetLayout DescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayoutBinding* bindings, uint32_t sizeOfBindings);

		std::array<VkVertexInputAttributeDescription, 3> VertexAttributeDescriptions();


		VkRenderPass RenderPass(const VkDevice l_device, const VkFormat& depthFormat);

		VkCommandPool CommandPool(const VkDevice& l_device, VkCommandPoolCreateFlags createFlag);

		VkCommandBuffer CommandBuffer(const VkDevice l_device, const VkCommandPool cmdPool, VkCommandBufferLevel cmdLevel);

		VkSemaphore CreateSemaphore(const VkDevice l_device);

		VkFence CreateFence(const VkDevice l_device);

		VkPipelineLayout CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout, std::vector<VkPushConstantRange>& pushConstantRanges);

		VkPushConstantRange PushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags shaderStages);

		VkDescriptorSetLayoutBinding DescriptorLayoutBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage);

		VkPipeline CreateGraphicsPipeline(const VkDevice l_device, const VkPipelineLayout pipelineLayout, const VkRenderPass renderPass, VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const VkShaderModule& shaderModule, VkShaderStageFlagBits stage);

		VkShaderModule ShaderModule(const VkDevice& l_device, const char* filename);

		VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);

		VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, uint32_t dscCount);

		VkDescriptorPool DescriptorPool(const VkDevice l_device, const VkDescriptorPoolCreateInfo& poolInfo);

		VkDescriptorSet DescriptorSet(const VkDevice l_device, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscLayout);
	}

}