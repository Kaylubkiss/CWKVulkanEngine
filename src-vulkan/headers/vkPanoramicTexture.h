#ifndef VK_PANORAMIC_TEXTURE_HPP
#define VK_PANORAMIC_TEXTURE_HPP

#include "vkInit.h"
#include "vkTexture.h"

namespace vk
{
	/**
	* Produces image-based lighting (IBL) resources from an HDR
	* equirectangular source texture.
	*
	* Created resources:
	* - Environment cubemap
	* - Diffuse irradiance cubemap
	* - GGX prefiltered specular cubemap
	* - BRDF integration LUT
	*
	* The class uses compute pipelines to bake all derived textures
	* during construction.
	*
	* Notes:
	* - Inherits from vk::Texture for integration with the rendering system.
	* - Uses VK_EXT_descriptor_buffer for GPU-addressable descriptor access.
	* - Enforces move-only semantics for Vulkan resource ownership.
	*/
    class PanoramicTexture : public Texture
    {
    public:
    	PanoramicTexture() = default;

    	/**
		* Loads an HDR equirectangular texture and synchronously creates
		* all required IBL resources.
		*
		* @param devicePtr Logical device abstraction.
		* @param createInfo Texture creation parameters and source paths.
		*/
    	PanoramicTexture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );

    	// Non-copyable due to unique Vulkan resource ownership.
    	PanoramicTexture( const PanoramicTexture& other ) = delete;
    	PanoramicTexture& operator=( const PanoramicTexture& other ) = delete;

    	/**
		* Transfers ownership of Vulkan resources.
		*/
    	PanoramicTexture( PanoramicTexture&& other ) noexcept;

    	/**
		* Transfers ownership while releasing existing resources.
		*/
    	PanoramicTexture& operator=( PanoramicTexture&& other ) noexcept;

    	/**
		* Releases compute pipelines and pipeline layout resources.
		*/
	    ~PanoramicTexture() override;

    	/**
		* @name IBL Texture Descriptors
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
    	* Creates the shared compute pipeline layout used by all
		* IBL baking pipelines.
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
		* Allocates the descriptor buffer used by compute pipelines.
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
    	* Creates a compute pipeline from a shader module.
    	*/
    	[[nodiscard]] VkPipeline CreateComputePipeline( std::string_view fileName ) const
    	{
			VkComputePipelineCreateInfo computePipelineCI = {};
			computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(c_device,
				fileName, VK_SHADER_STAGE_COMPUTE_BIT);

			VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.GetHandle(), shaderModuleInfo.GetShaderStageFlags());

			computePipelineCI.stage = shaderStageCI;
			computePipelineCI.layout = m_computePipelineLayout;

    		VkPipeline handle;
			vkCreateComputePipelines( c_device, VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &handle );

    		return handle;
    	}

    	// IBL generation stages.
        void WriteToEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels );

    	void WriteToIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t layoutIndex );

    	void WriteToPrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		std::vector<VkDescriptorImageInfo>& prefilterInfos, uint32_t mipLevels, uint32_t layoutIndex );

    	void WriteToBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t layoutIndex );

    	/**
		* Creates a texture resource initialized for compute access.
		*/
    	vk::Texture CreateTexture( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels, VkImageUsageFlags imageUsage ) const;
    private:
    	// IBL image resources.
    	vk::Texture m_environmentMap;
    	vk::Texture m_irradianceMap;
    	vk::Texture m_prefilterMap;
    	vk::Texture m_BRDFLUT;

    	// Shared compute pipeline layout.
    	VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;

    	// Compute pipelines for IBL creation stages
    	VkPipeline m_EquirectangularToCubemapPipeline = VK_NULL_HANDLE;
    	VkPipeline m_convolutionPipeline = VK_NULL_HANDLE;
    	VkPipeline m_prefilterPipeline = VK_NULL_HANDLE;
    	VkPipeline m_BRDFLUTPipeline = VK_NULL_HANDLE;

    	vk::DescriptorBuffer m_computeDescriptorBuffer;
    };


}

#endif