#pragma once


#include <vulkan/vulkan.h>

namespace vk
{
	namespace init 
	{
		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo();

		VkVertexInputBindingDescription VertexInputBindingDescription(uint32_t binding = 0);

		VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo();

		VkDescriptorSetLayout DescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayoutBinding* bindings, uint32_t sizeOfBindings);

		VkRenderPass RenderPass(const VkDevice l_device, const VkFormat& depthFormat);

		VkCommandPool CommandPool(const VkDevice& l_device, VkCommandPoolCreateFlags createFlag);

		VkCommandBuffer CommandBuffer(const VkDevice l_device, const VkCommandPool cmdPool, VkCommandBufferLevel cmdLevel);

		VkSemaphore CreateSemaphore(const VkDevice l_device);

		VkFence CreateFence(const VkDevice l_device);

		VkPipelineLayout CreatePipelineLayout(const VkDevice l_device, const VkDescriptorSetLayout descriptorSetLayout, std::vector<VkPushConstantRange>& pushConstantRanges);

		VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();

		VkPushConstantRange PushConstantRange(uint32_t offset, uint32_t size, VkShaderStageFlags shaderStages);

		VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology, VkPipelineInputAssemblyStateCreateFlags flags, VkBool32 primitiveRestartEnable);
		
		VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkPipelineRasterizationStateCreateFlags flags = 0);

		VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 blendEnable);

		VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(uint32_t attachmentCount, VkPipelineColorBlendAttachmentState* pAttachments);

		VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp);

		VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(uint32_t viewportCount, uint32_t scissorCount, VkPipelineViewportStateCreateFlags flags = 0);

		VkPipelineMultisampleStateCreateInfo PipelineMultisampleCreateInfo(VkSampleCountFlagBits rasterizationSamples, VkPipelineMultisampleStateCreateFlags flags = 0);

		VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateFlags flags = 0);

		VkGraphicsPipelineCreateInfo PipelineCreateInfo(VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkPipelineCreateFlags flags = 0);

		VkPipeline CreateGraphicsPipeline(const VkDevice l_device, const VkPipelineLayout pipelineLayout, const VkRenderPass renderPass, VkPipelineShaderStageCreateInfo* pStages, int numStages, VkPrimitiveTopology primitiveTopology);

		VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(const VkShaderModule& shaderModule, VkShaderStageFlagBits stage);
		VkShaderModule ShaderModule(const VkDevice& l_device, const char* filename);

		VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);

		VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type, uint32_t dscCount);

		VkDescriptorPool DescriptorPool(const VkDevice l_device, const VkDescriptorPoolCreateInfo& poolInfo);

		VkDescriptorSet DescriptorSet(const VkDevice l_device, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscLayout, uint32_t dscCount = 1);

		VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

		VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool, const VkDescriptorSetLayout* descriptorLayout, uint32_t descriptorSetCount);

		VkDescriptorSetLayoutBinding DescriptorLayoutBinding(uint32_t binding, uint32_t descriptorCount, VkDescriptorType descriptorType, VkShaderStageFlags shaderStage);

		//buffer descriptors
		VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount = 1);

		//image descriptors
		VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount = 1);

		VkRenderPassCreateInfo RenderPassCreateInfo();

		VkFramebufferCreateInfo FramebufferCreateInfo();

		VkImageCreateInfo ImageCreateInfo();

		VkImageViewCreateInfo ImageViewCreateInfo();

		VkSamplerCreateInfo SamplerCreateInfo();

		VkMemoryAllocateInfo MemoryAllocateInfo();

		VkMappedMemoryRange MappedMemoryRange();

		VkBufferCreateInfo BufferCreateInfo(VkBufferUsageFlags usageFlags, VkDeviceSize size);

		VkCommandBufferBeginInfo CommandBufferBeginInfo();

		VkRenderPassBeginInfo RenderPassBeginInfo();

		VkViewport Viewport(uint32_t width, uint32_t height, float minDepth = 0.f, float maxDepth = 1.f);

		VkRect2D Rect2D(uint32_t width, uint32_t height, int32_t offset_x = 0, int32_t offset_y = 0);

		//special engine-specific resources
		FramebufferAttachment CreateDepthAttachment(const VkPhysicalDevice& p_device, const VkDevice& l_device, const VkViewport& viewport);
		VkImage CreateImage
		(
			const VkPhysicalDevice& p_device, const VkDevice& l_device, uint32_t width, uint32_t height, uint32_t mipLevels,
			VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags flags, VkDeviceMemory& imageMemory, uint32_t arrayLayerCount
		);
	}

}