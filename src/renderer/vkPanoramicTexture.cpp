#include "vkPanoramicTexture.h"
#include "vkShaderModule.h"

namespace vk
{
	/**
	* Loads an HDR equirectangular texture and creates the associated
	* image-based lighting resources:
	*
	* - Environment cubemap
	* - Diffuse irradiance map
	* - GGX prefiltered specular map
	* - BRDF integration LUT
	*
	* GPU baking is executed synchronously during construction to ensure
	* all produced textures are immediately ready for rendering.
	*
	* @param devicePtr Logical device abstraction containing queue access.
	* @param createInfo Texture creation parameters and source image paths.
	*
	* @throws std::runtime_error if HDR image loading fails.
	*/
	PanoramicTexture::PanoramicTexture( const vk::Device* devicePtr,  const vk::TextureCreateInfo& createInfo )
    {
		//check: is there a prefiltered map? is there a BRDF LUT?
		assert( devicePtr != nullptr );

		c_device = devicePtr->GetDevice();

		m_name = createInfo.fileName;

		vk::Texture panoramicTexture = vk::Texture( devicePtr, createInfo );

        VkFence submissionFence = vk::init::CreateFence( devicePtr->GetDevice(), false );

        VkCommandPool graphicsCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::GRAPHICS).family);

        VkCommandBuffer graphicsCmd = vk::util::beginSingleTimeCommand(devicePtr->GetDevice(), graphicsCmdPool);

		panoramicTexture.RecordStagingCopy( graphicsCmd );

		// Transition source texture for shader sampling.
    	vk::util::RecordImageLayoutTransition( graphicsCmd, panoramicTexture.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		uint32_t prefilter_width = 512;
		uint32_t prefilterMipLevels = vk::util::CalculateMipLevels( prefilter_width, prefilter_width );

		// Allocate descriptor layouts for all IBL image resources.
		// Environment Map (1) + Irradiance Map (1) + BRDF LUT (1) + Prefilter Mips
		uint32_t layoutCount = 3 + prefilterMipLevels;

    	CreateComputeDescriptorBuffer( devicePtr, layoutCount );
    	CreateComputePipelineLayout();

		uint32_t layoutIndex = 0;
		uint32_t env_width = 512;
		uint32_t env_mipLevels = vk::util::CalculateMipLevels( env_width, env_width );

		// Create environment cubemap from equirectangular source.
		m_environmentMap = CreateTexture( devicePtr, graphicsCmd, env_width, env_width,
			6, env_mipLevels,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT );

		WriteToEnvironmentMapImage( devicePtr, graphicsCmd, panoramicTexture.GetDescriptor(),
			env_width, env_width, 6, env_mipLevels );

		// Create diffuse irradiance cubemap.
		uint32_t irradiance_width = 32;
		m_irradianceMap = CreateTexture( devicePtr, graphicsCmd, irradiance_width, irradiance_width,
			6, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT );

		layoutIndex = 1;
		WriteToIrradianceImage( devicePtr, graphicsCmd, layoutIndex );

		// Create GGX prefiltered specular cubemap.
		m_prefilterMap = CreateTexture( devicePtr, graphicsCmd, prefilter_width, prefilter_width, 6,
			prefilterMipLevels, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT );

		std::vector<VkDescriptorImageInfo> prefilterInfos;
		layoutIndex = 2;
		WriteToPrefilterImage( devicePtr, graphicsCmd, prefilterInfos, prefilterMipLevels, layoutIndex );

		// Create BRDF integration LUT.
		uint32_t BRDF_width = 512;
    	m_BRDFLUT = CreateTexture( devicePtr, graphicsCmd, BRDF_width, BRDF_width, 1, 1,
    		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT );

		layoutIndex = 2 + prefilterMipLevels;
		WriteToBRDFLUTImage( devicePtr, graphicsCmd, layoutIndex );

    	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

		// Submit baking workload and wait for completion before exposing resources to rendering.
    	vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
    		devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    		submissionFence, std::nullopt );

		// Release temporary mip view resources used during prefilter generation
		if ( !prefilterInfos.empty() )
		{
			vkDestroySampler(devicePtr->GetDevice(), prefilterInfos.front().sampler, nullptr);
		}

		for ( auto& infos : prefilterInfos )
		{
			vkDestroyImageView(devicePtr->GetDevice(), infos.imageView, nullptr);
		}

    	vkFreeCommandBuffers(devicePtr->GetDevice(), graphicsCmdPool, 1, &graphicsCmd);
    	vkDestroyCommandPool(devicePtr->GetDevice(), graphicsCmdPool, nullptr);

		vkDestroyFence(devicePtr->GetDevice(), submissionFence, nullptr);

		std::cout << "\033[32m" << "successfully loaded Texture in PanoramicTexture::Create()... " << "\033[0m\n";
    }

	/**
	* Transfers ownership of GPU resources from another PanoramicTexture.
	*/
	PanoramicTexture::PanoramicTexture( PanoramicTexture&& other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_name = other.m_name;
			this->m_EquirectangularToCubemapPipeline = other.m_EquirectangularToCubemapPipeline;
			this->m_convolutionPipeline = other.m_convolutionPipeline;
			this->m_prefilterPipeline = other.m_prefilterPipeline;
			this->m_BRDFLUTPipeline = other.m_BRDFLUTPipeline;

			this->m_environmentMap = std::move(other.m_environmentMap);
			this->m_irradianceMap = std::move(other.m_irradianceMap);
			this->m_prefilterMap = std::move(other.m_prefilterMap);
			this->m_BRDFLUT = std::move(other.m_BRDFLUT);
			this->m_computeDescriptorBuffer = std::move(other.m_computeDescriptorBuffer);

			this->m_computePipelineLayout = other.m_computePipelineLayout;

			other.c_device = VK_NULL_HANDLE;
		}
	}

	/**
	* Transfers ownership of GPU resources while releasing existing state.
	*/
	PanoramicTexture& PanoramicTexture::operator=( PanoramicTexture&& other ) noexcept
	{
		if (this != &other)
		{
			std::swap(this->c_device, other.c_device);
			std::swap(this->m_name, other.m_name);
			std::swap(this->m_environmentMap, other.m_environmentMap);
			std::swap(this->m_irradianceMap, other.m_irradianceMap);
			std::swap(this->m_prefilterMap, other.m_prefilterMap);
			std::swap(this->m_BRDFLUT, other.m_BRDFLUT);

			std::swap(this->m_EquirectangularToCubemapPipeline, other.m_EquirectangularToCubemapPipeline);
			std::swap(this->m_convolutionPipeline, other.m_convolutionPipeline);
			std::swap(this->m_prefilterPipeline, other.m_prefilterPipeline);
			std::swap(this->m_BRDFLUTPipeline, other.m_BRDFLUTPipeline);

			std::swap(this->m_computeDescriptorBuffer, other.m_computeDescriptorBuffer);

			std::swap(this->m_computePipelineLayout, other.m_computePipelineLayout);
		}

		return *this;
	}

	/**
	* Releases compute pipelines and pipeline layout.
	*
	* Texture resources manage their own image and memory cleanup.
	*/
	PanoramicTexture::~PanoramicTexture()
	{
		if (c_device != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(c_device, m_EquirectangularToCubemapPipeline, nullptr);
			vkDestroyPipeline(c_device, m_convolutionPipeline, nullptr);
			vkDestroyPipeline(c_device, m_prefilterPipeline, nullptr);
			vkDestroyPipeline(c_device, m_BRDFLUTPipeline, nullptr);

			vkDestroyPipelineLayout(c_device, m_computePipelineLayout, nullptr);
		}
	}


	/**
    * Creates the shared compute pipeline layout used by all
	* IBL baking pipelines.
	*/
    void PanoramicTexture::CreateComputePipelineLayout()
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
    void PanoramicTexture::CreateComputeDescriptorBuffer( const vk::Device* devicePtr, uint32_t layoutCount )
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
    [[nodiscard]] VkPipeline PanoramicTexture::CreateComputePipeline( std::string_view fileName ) const
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

	/**
	* Converts the source equirectangular texture into a cubemap.
	*
	* The produced cubemap is mipmapped and transitioned to
	* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
	*
	* @pre Source texture is in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
	*/
	void PanoramicTexture::WriteToEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		VkDescriptorImageInfo panoramicTextureInfo, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels )
    {
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

	    writeResource.pImageData = &panoramicTextureInfo;
	    m_computeDescriptorBuffer.WriteDescriptor(writeResource,
		    0,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

		auto environmentMapDescriptor = m_environmentMap.GetDescriptor();
	    writeResource.pImageData = &environmentMapDescriptor;
	    m_computeDescriptorBuffer.WriteDescriptor(writeResource,
		    0, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

	    //create compute pipeline
		m_EquirectangularToCubemapPipeline = CreateComputePipeline( "equirectangular-to-cubemap.comp" );

		vkCmdBindPipeline(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_EquirectangularToCubemapPipeline);

        std::vector<VkDescriptorBufferBindingInfoEXT> descriptorBufferBindingInfos =
        {
            {
	            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
	            .address = m_computeDescriptorBuffer.GetBuffer().GetDeviceAddress(),
	            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT
            },
        };

		g_vkCmdBindDescriptorBuffersEXT(graphicsCmd, static_cast<uint32_t>(descriptorBufferBindingInfos.size()),
			descriptorBufferBindingInfos.data());

		VkDeviceSize bufferOffset = 0;
		uint32_t descriptorIndex = 0;

		g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

		VkExtent2D imageExtent = m_environmentMap.GetImageExtent();

		// Dispatch conversion across all cubemap faces.
		vkCmdDispatch(graphicsCmd, imageExtent.width / 16, imageExtent.height / 16, 6);

		// Transition image for mipmap generation.
		vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMap.GetImage(),
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		vk::util::RecordBlitMipMapImages( graphicsCmd,  m_environmentMap.GetImage(),
			width, height, mipLevels, layerCount );

		m_environmentMap.SetImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    }

	/**
	* Produces a diffuse irradiance cubemap from the environment map.
	*
	* @pre Environment map is in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
	*/
	void PanoramicTexture::WriteToIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t layoutIndex )
    {
    	//write to descriptor buffer
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

		auto environmentMapDescriptor = m_environmentMap.GetDescriptor();
    	writeResource.pImageData = &environmentMapDescriptor;
    	m_computeDescriptorBuffer.WriteDescriptor(writeResource,
			layoutIndex,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

		VkDescriptorImageInfo m_irradianceInfo = m_irradianceMap.GetDescriptor();

    	writeResource.pImageData = &m_irradianceInfo;
    	m_computeDescriptorBuffer.WriteDescriptor( writeResource,
			layoutIndex, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

		m_convolutionPipeline = CreateComputePipeline( "convolute-cubemap.comp" );

		vkCmdBindPipeline( graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_convolutionPipeline );

        std::vector<VkDescriptorBufferBindingInfoEXT> descriptorBufferBindingInfos =
        {
            {
	            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
	            .address = m_computeDescriptorBuffer.GetBuffer().GetDeviceAddress(),
	            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT
            },
        };

		g_vkCmdBindDescriptorBuffersEXT(graphicsCmd, static_cast<uint32_t>(descriptorBufferBindingInfos.size()),
			descriptorBufferBindingInfos.data());

		// Select descriptor region for irradiance generation.
		VkDeviceSize bufferOffset = m_computeDescriptorBuffer.GetLayoutSize();
		uint32_t descriptorIndex = 0;

		g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

		auto imageExtent = m_irradianceMap.GetImageExtent();

		vkCmdDispatch(graphicsCmd, imageExtent.width / 16, imageExtent.height / 16, 6);

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_irradianceMap.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    	m_irradianceMap.SetImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    }

	/**
	* Produces mipmapped GGX prefiltered specular reflections used for PBR IBL.
	*
	* Each mip level corresponds to an increasing surface roughness.
	*/
	void PanoramicTexture::WriteToPrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		std::vector<VkDescriptorImageInfo>& prefilterInfos, uint32_t mipLevels, uint32_t layoutIndex )
    {
    	prefilterInfos.resize(mipLevels);

    	VkImageViewCreateInfo viewCI = vk::init::ImageViewCreateInfo();
    	viewCI.image = m_prefilterMap.GetImage();
    	viewCI.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    	viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    	viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    	viewCI.subresourceRange.baseMipLevel = 0;
    	viewCI.subresourceRange.levelCount = 1;
    	viewCI.subresourceRange.baseArrayLayer = 0;
    	viewCI.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    	viewCI.components = vk::init::ComponentMappingSwizzleIdentity();

    	VkSampler sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(),
    		mipLevels);

		// Create storage views for individual mip levels.
    	for (uint32_t mip = 0; mip < mipLevels; ++mip)
    	{
    		viewCI.subresourceRange.baseMipLevel = mip;
    		VK_CHECK_RESULT(vkCreateImageView(devicePtr->GetDevice(), &viewCI,
				nullptr, &prefilterInfos[mip].imageView ));

    		prefilterInfos[mip].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    		prefilterInfos[mip].sampler = sampler;
    	}

    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();
		auto environmentMapDescriptor = m_environmentMap.GetDescriptor();

		// Bind descriptors for each prefilter mip level
    	for (uint32_t mip = 0; mip < mipLevels; ++mip)
    	{
    		writeResource.pImageData = &environmentMapDescriptor;
    		m_computeDescriptorBuffer.WriteDescriptor(writeResource,
				layoutIndex + mip,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

    		writeResource.pImageData = &prefilterInfos[mip];
    		m_computeDescriptorBuffer.WriteDescriptor( writeResource,
				layoutIndex + mip, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);
    	}

		m_prefilterPipeline = CreateComputePipeline( "prefilter-cubemap.comp" );
    	vkCmdBindPipeline(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_prefilterPipeline);

        std::vector<VkDescriptorBufferBindingInfoEXT> descriptorBufferBindingInfos =
        {
            {
	            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
	            .address = m_computeDescriptorBuffer.GetBuffer().GetDeviceAddress(),
	            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT
            },
        };

		g_vkCmdBindDescriptorBuffersEXT(graphicsCmd, static_cast<uint32_t>(descriptorBufferBindingInfos.size()),
			descriptorBufferBindingInfos.data());

    	VkDeviceSize layoutSize = m_computeDescriptorBuffer.GetLayoutSize();
    	VkDeviceSize initOffset =  layoutSize * 2;
    	uint32_t descriptorIndex = 0;
		VkExtent2D imageExtent = m_prefilterMap.GetImageExtent();

		// Dispatch one pass per roughness mip level.
    	for (uint32_t mip = 0; mip < mipLevels; ++mip)
    	{
    		VkDeviceSize bufferOffset = initOffset + mip * layoutSize;

    		g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

    		// Map mip level to surface roughness
    		float roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);

    		vkCmdPushConstants(graphicsCmd, m_computePipelineLayout,
    			VK_SHADER_STAGE_COMPUTE_BIT,
    			0, sizeof(float), &roughness);

    		vkCmdDispatch(graphicsCmd, imageExtent.width / 16, imageExtent.height / 16, 6);
    	}

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_prefilterMap.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_GENERAL,
    		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		m_prefilterMap.SetImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    }

	/**
	* Produces the BRDF integration lookup table used for split-sum specular IBL.
	*/
	void PanoramicTexture::WriteToBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		uint32_t layoutIndex )
    {
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

		VkDescriptorImageInfo BRDFLUTInfo = m_BRDFLUT.GetDescriptor();
    	writeResource.pImageData = &BRDFLUTInfo;
    	m_computeDescriptorBuffer.WriteDescriptor( writeResource,
			layoutIndex, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

		m_BRDFLUTPipeline = CreateComputePipeline( "brdf-integrate.comp" );
    	vkCmdBindPipeline(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_BRDFLUTPipeline);

    	std::vector<VkDescriptorBufferBindingInfoEXT> descriptorBufferBindingInfos =
    	{
    		{
    			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
				.address = m_computeDescriptorBuffer.GetBuffer().GetDeviceAddress(),
				.usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT
			},
		};

    	g_vkCmdBindDescriptorBuffersEXT(graphicsCmd, static_cast<uint32_t>(descriptorBufferBindingInfos.size()),
			descriptorBufferBindingInfos.data());

    	VkDeviceSize layoutSize = m_computeDescriptorBuffer.GetLayoutSize();
    	VkDeviceSize bufferOffset =  layoutSize * layoutIndex;
    	uint32_t descriptorIndex = 0;

    	g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

		VkExtent2D imageExtent = m_BRDFLUT.GetImageExtent();

		// Generate 2D BRDF integration LUT.
    	vkCmdDispatch(graphicsCmd, imageExtent.width / 16, imageExtent.height / 16, 1);

    	vk::util::RecordImageLayoutTransition(graphicsCmd, m_BRDFLUT.GetImage(),
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

		m_BRDFLUT.SetImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    }

	/**
	* Creates a texture resource and transitions it to
	* VK_IMAGE_LAYOUT_GENERAL for compute access.
	*
	* @return Initialized texture resource.
	*/
	vk::Texture PanoramicTexture::CreateTexture( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels, VkImageUsageFlags imageUsage )
    {
		vk::TextureCreateInfo textureCI = {};
		textureCI.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		textureCI.width = width;
		textureCI.height = height;
		textureCI.layerCount = layerCount;
		textureCI.mipLevels = mipLevels;
		textureCI.imageUsage = imageUsage;

		if (layerCount == 6)
		{
			textureCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		vk::Texture newTexture = vk::Texture( devicePtr, textureCI );

		// Prepare texture for compute shader writes.
    	vk::util::RecordImageLayoutTransition( graphicsCmd, newTexture.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

		return newTexture;
    }
}