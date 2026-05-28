#pragma once
#include "vkInit.h"
#include "vkTexture.h"
#include "vkUtil.h"

//WIP
//Yes, I know there's a lot of copy-paste here. I'm working on it.

namespace vk
{
    class PanoramicTexture : public Texture //technically a type of cubemap, but inheriting cubemap would be useless.
    {
    public:
	    ~PanoramicTexture() override
    	{
    		if (c_device != VK_NULL_HANDLE)
    		{
    			vkDestroyImage(c_device, m_environmentMapImage, nullptr);
    			vkFreeMemory(c_device, m_environmentMapImageMemory, nullptr);

    			vkDestroyImage(c_device, m_irradianceImage, nullptr);
    			vkFreeMemory(c_device, m_irradianceImageMemory, nullptr);

    			vkDestroyImage(c_device, m_prefilterImage, nullptr);
    			vkFreeMemory(c_device, m_prefilterImageMemory, nullptr);

    			vkDestroySampler(c_device, m_environmentMapInfo.sampler, nullptr);
    			vkDestroyImageView(c_device, m_environmentMapInfo.imageView, nullptr);

    			vkDestroySampler(c_device, m_irradianceInfo.sampler, nullptr);
    			vkDestroyImageView(c_device, m_irradianceInfo.imageView, nullptr);

    			vkDestroySampler(c_device, m_prefilterInfo.sampler, nullptr);
    			vkDestroyImageView(c_device, m_prefilterInfo.imageView, nullptr);

    			vkDestroySampler(c_device, m_BRDFLUTInfo.sampler, nullptr);
    			vkDestroyImageView(c_device, m_BRDFLUTInfo.imageView, nullptr);

    			vkDestroyPipeline(c_device, m_computePipeline, nullptr);
    			vkDestroyPipeline(c_device, m_convolutionPipeline, nullptr);
    			vkDestroyPipeline(c_device, m_prefilterPipeline, nullptr);
    			vkDestroyPipeline(c_device, m_BRDFLUTPipeline, nullptr);

    			vkDestroyPipelineLayout(c_device, m_computePipelineLayout, nullptr);

    			m_computeDescriptorBuffer.Destroy();
    		}
    	}

    	[[nodiscard]] VkDescriptorImageInfo GetEnvironmentMapImageDescriptor() const
	    {
	    	return m_environmentMapInfo;
	    }

    	[[nodiscard]] VkDescriptorImageInfo GetIrradianceImageDescriptor() const
    	{
			return m_irradianceInfo;
	    }

    	[[nodiscard]] VkDescriptorImageInfo GetPrefilterMapImageDescriptor() const
    	{
			return m_prefilterInfo;
	    }

    	[[nodiscard]] VkDescriptorImageInfo GetBRDFLUTImageDescriptor() const
    	{
			return m_BRDFLUTInfo;
	    }

	    void Create( vk::Device* devicePtr,  const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex ) override;
    private:
    	void CreateComputePipelineLayout( vk::Device* devicePtr )
    	{
			VkPipelineLayoutCreateInfo pipelineLayoutCI = vk::init::PipelineLayoutCreateInfo();

    		VkDescriptorSetLayout desciptorSetLayout = m_computeDescriptorBuffer.GetLayout();

    		pipelineLayoutCI.pSetLayouts = &desciptorSetLayout;
			pipelineLayoutCI.setLayoutCount = 1;

    		VkPushConstantRange pushConstants = vk::init::PushConstantRange(0,
    			sizeof(float), VK_SHADER_STAGE_COMPUTE_BIT);

    		pipelineLayoutCI.pPushConstantRanges = &pushConstants;
    		pipelineLayoutCI.pushConstantRangeCount = 1;

			vkCreatePipelineLayout(devicePtr->GetDevice(), &pipelineLayoutCI, nullptr, &m_computePipelineLayout);
		}

    	void CreateComputeDescriptorBuffer( vk::Device* devicePtr )
    	{

    		VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    		std::vector<VkDescriptorSetLayoutBinding> layoutBindings =
    		{
    			{
    				.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
				},
				{
					.binding = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				}
    		};

    		//environment + convolution + prefiltering.
    		m_computeDescriptorBuffer.Allocate(devicePtr, bufferUsageFlags, memoryProperties,
				1, 2 + m_prefilterMipLevels + 1, layoutBindings);

    	}

    	VkPipeline CreateComputePipeline( vk::Device* devicePtr, std::string_view fileName )
    	{
			VkComputePipelineCreateInfo computePipelineCI = {};
			computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(devicePtr->GetDevice(),
				fileName, VK_SHADER_STAGE_COMPUTE_BIT);

			VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.mHandle, shaderModuleInfo.mFlags);

			computePipelineCI.stage = shaderStageCI;
			computePipelineCI.layout = m_computePipelineLayout;

    		VkPipeline handle;
			vkCreateComputePipelines(devicePtr->GetDevice(), VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &handle);

			vkDestroyShaderModule( devicePtr->GetDevice(), shaderModuleInfo.mHandle, nullptr );

    		return handle;
    	}

        void WriteToEnvironmentMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, VkFence submissionFence );

        void CreateEnvironmentMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	VkFence submissionFence );

    	void WriteToIrradianceImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence );

    	void CreateIrradianceImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence );

    	void WriteToPrefilterImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence );

    	void CreatePrefilterImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence );

    	void WriteToBRDFLUTImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence );

    	void CreateBRDFLUTImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence );
    private:
    	VkImage m_environmentMapImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_environmentMapImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_environmentMapInfo = {};

    	//NOTE: since convolution and cubemap creation have the exact same layout,
    	//they will share m_computePipelineLayout and m_computeDescriptorBuffer.

    	VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;

    	VkPipeline m_computePipeline = VK_NULL_HANDLE;
    	VkPipeline m_convolutionPipeline = VK_NULL_HANDLE;
    	VkPipeline m_prefilterPipeline = VK_NULL_HANDLE;
    	VkPipeline m_BRDFLUTPipeline = VK_NULL_HANDLE;

    	VkImage m_irradianceImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_irradianceImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_irradianceInfo = {};

    	VkImage m_prefilterImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_prefilterImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_prefilterInfo = {};

    	VkImage m_BRDFLUTImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_BRDFLUTImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_BRDFLUTInfo = {};

		uint32_t m_prefilterMipLevels = 0; //must compute this later.
    	uint32_t m_prefilterWidth = 512;
    	uint32_t m_prefilterHeight = 512;

    	vk::DescriptorBuffer m_computeDescriptorBuffer;

    };


}