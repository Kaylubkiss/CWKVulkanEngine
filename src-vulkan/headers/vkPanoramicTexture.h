#pragma once
#include "vkInit.h"
#include "vkTexture.h"

namespace vk
{
	/**
	 * @class PanoramicTexture
	 * @brief A specialized texture resource that takes in an equirectangular image
	 * and bakes it into the cubemaps required for Image-Based Lighting (IBL).
	 *
	 * Architecture Notes:
	 * - Inherits from vk::Texture to integrate with the engine's rendering pipeline.
	 * - Uses VK_EXT_descriptor_buffer to bind resources directly via GPU addresses,
	 * avoiding descriptor pool overhead.
	 * - Enforces move-only semantics to manage the lifetimes of internal pipelines
	 * and generated textures.
	 */
    class PanoramicTexture : public Texture
    {
    public:
    	PanoramicTexture() = default;

    	/**
		 * @brief Synchronously bakes all required IBL maps from an equirectangular source file.
		 */
    	PanoramicTexture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );

    	// Disable copy semantics to guarantee unique ownership of underlying Vulkan handles
    	PanoramicTexture( const PanoramicTexture& other ) = delete;
    	PanoramicTexture& operator=( const PanoramicTexture& other ) = delete;

    	/**
		 * @brief Move constructor transferring unique ownership of compute pipelines and baked textures.
		 */
    	PanoramicTexture( PanoramicTexture&& other ) noexcept;

    	/**
		 * @brief Move assignment operator safely swapping resource ownership.
		 */
    	PanoramicTexture& operator=( PanoramicTexture&& other ) noexcept;

    	/**
		 * @brief Destructor releasing active compute pipelines and layouts.
		 */
	    ~PanoramicTexture() override;

    	/**
		 * @name Public API Resource Descriptor Getters
		 * @{
		 */
    	[[nodiscard]] VkDescriptorImageInfo GetEnvironmentMapImageDescriptor() const
	    {
	    	return m_environmentMap.GetDescriptor();
	    }

    	[[nodiscard]] VkDescriptorImageInfo GetIrradianceImageDescriptor() const
    	{
			return m_irradianceMap.GetDescriptor();
	    }

    	[[nodiscard]] VkDescriptorImageInfo GetPrefilterMapImageDescriptor() const
    	{
			return m_prefilterMap.GetDescriptor();
	    }

    	[[nodiscard]] VkDescriptorImageInfo GetBRDFLUTImageDescriptor() const
    	{
			return m_BRDFLUT.GetDescriptor();
	    }
    	/** @} */

    private:
    	/**
		 * @brief Generates the uniform pipeline layout bound across all compute pipelines.
		 */
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

    	/**
		 * @brief Pre-allocates the modern descriptor buffer backings matching input layout demands.
		 */
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

    	/**
		 * @brief Helper to compile a compute shader with descriptor buffer flags.
		 */
    	[[nodiscard]] VkPipeline CreateComputePipeline( std::string_view fileName ) const
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

    	// Sub-stage execution wrappers orchestrating the physical GPU compute dispatches
        void WriteToEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels );

    	void WriteToIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd );

    	void WriteToPrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		std::vector<VkDescriptorImageInfo>& prefilterInfos, uint32_t mipLevels, uint32_t layoutIndex );

    	void WriteToBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t layoutIndex );

    	// Factory generator tracking internal layout allocation definitions
    	vk::Texture CreateTexture( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels, VkImageUsageFlags imageUsage ) const;
    private:
    	// Persistent specialized PBR asset allocations
    	vk::Texture m_environmentMap;
    	vk::Texture m_irradianceMap;
    	vk::Texture m_prefilterMap;
    	vk::Texture m_BRDFLUT;

    	// Shared Layout Context: Convolution and base cubemap projection share structural binding layouts,
    	// allowing efficient reuse of this unified layout and descriptor buffer allocation.
    	VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;

    	// Individual compute pipelines for each step of the IBL baking process
    	VkPipeline m_EquirectangularToCubemapPipeline = VK_NULL_HANDLE;
    	VkPipeline m_convolutionPipeline = VK_NULL_HANDLE;
    	VkPipeline m_prefilterPipeline = VK_NULL_HANDLE;
    	VkPipeline m_BRDFLUTPipeline = VK_NULL_HANDLE;

    	vk::DescriptorBuffer m_computeDescriptorBuffer;
    };


}