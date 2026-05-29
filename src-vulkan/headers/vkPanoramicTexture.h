#pragma once
#include "vkInit.h"
#include "vkTexture.h"

namespace vk
{
	//technically a type of cubemap, but inheriting cubemap would be useless.
    class PanoramicTexture : public Texture
    {
    public:
    	PanoramicTexture() = default;
    	PanoramicTexture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );
    	PanoramicTexture( const PanoramicTexture& other ) = delete;
    	PanoramicTexture( PanoramicTexture&& other ) noexcept;

    	PanoramicTexture& operator=( const PanoramicTexture& other ) = delete;
    	PanoramicTexture& operator=( PanoramicTexture&& other ) noexcept;

	    ~PanoramicTexture() override;

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
    private:
    	void CreateComputePipelineLayout()
    	{
			VkPipelineLayoutCreateInfo pipelineLayoutCI = vk::init::PipelineLayoutCreateInfo();

    		VkDescriptorSetLayout desciptorSetLayout = m_computeDescriptorBuffer.GetLayout();

    		pipelineLayoutCI.pSetLayouts = &desciptorSetLayout;
			pipelineLayoutCI.setLayoutCount = 1;

    		VkPushConstantRange pushConstants = vk::init::PushConstantRange(0,
    			sizeof(float), VK_SHADER_STAGE_COMPUTE_BIT);

    		pipelineLayoutCI.pPushConstantRanges = &pushConstants;
    		pipelineLayoutCI.pushConstantRangeCount = 1;

			vkCreatePipelineLayout(c_device, &pipelineLayoutCI, nullptr, &m_computePipelineLayout);
		}

    	void CreateComputeDescriptorBuffer( const vk::Device* devicePtr, uint32_t layoutCount )
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


    		m_computeDescriptorBuffer = vk::DescriptorBuffer(devicePtr, bufferUsageFlags, memoryProperties,
				1, layoutCount, layoutBindings);
    	}

    	VkPipeline CreateComputePipeline( std::string_view fileName ) const
    	{
			VkComputePipelineCreateInfo computePipelineCI = {};
			computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(c_device,
				fileName, VK_SHADER_STAGE_COMPUTE_BIT);

			VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.mHandle, shaderModuleInfo.mFlags);

			computePipelineCI.stage = shaderStageCI;
			computePipelineCI.layout = m_computePipelineLayout;

    		VkPipeline handle;
			vkCreateComputePipelines( c_device, VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &handle );

			vkDestroyShaderModule( c_device, shaderModuleInfo.mHandle, nullptr );

    		return handle;
    	}

        void WriteToEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd );

        void CreateEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels );

    	void WriteToIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd );

    	void CreateIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd );

    	void WriteToPrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		std::vector<VkDescriptorImageInfo>& prefilterInfos );

    	void CreatePrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		std::vector<VkDescriptorImageInfo>& prefilterInfos );

    	void WriteToBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd );

    	void CreateBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd );
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

		uint32_t m_prefilterMipLevels = 0;
    	uint32_t m_prefilterWidth = 512;
    	uint32_t m_prefilterHeight = 512;

    	vk::DescriptorBuffer m_computeDescriptorBuffer;
    };


}