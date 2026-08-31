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
	* - Uses VK_EXT_descriptor_buffer for GPU-addressable descriptor access.
	* - Enforces move-only semantics for Vulkan resource ownership.
	*/
    class PanoramicTexture
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
	    ~PanoramicTexture();

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

    	[[nodiscard]] std::string_view GetName() const
    	{
    		return m_name;
    	}
    	/** @} */

    private:
    	/**
    	* Creates the shared compute pipeline layout used by all
		* IBL baking pipelines.
		*/
    	void CreateComputePipelineLayout();

    	/**
		* Allocates the descriptor buffer used by compute pipelines.
		*/
    	void CreateComputeDescriptorBuffer( const vk::Device* devicePtr, uint32_t layoutCount );

    	/**
    	* Creates a compute pipeline from a shader module.
    	*/
    	[[nodiscard]] VkPipeline CreateComputePipeline( std::string_view fileName ) const;

    	// IBL generation stages.
        void WriteToEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	VkDescriptorImageInfo panoramicTextureInfo, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels );

    	void WriteToIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t layoutIndex );

    	void WriteToPrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		std::vector<VkDescriptorImageInfo>& prefilterInfos, uint32_t mipLevels, uint32_t layoutIndex );

    	void WriteToBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t layoutIndex );

    	/**
		* Creates a texture resource initialized for compute access.
		*/
    	static vk::Texture CreateTexture( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels, VkImageUsageFlags imageUsage );
    private:
    	std::string m_name;

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
    	VkDevice c_device = VK_NULL_HANDLE;
    };


}

#endif