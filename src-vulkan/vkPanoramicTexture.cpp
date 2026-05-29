#include "vkPanoramicTexture.h"

namespace vk
{
	PanoramicTexture::PanoramicTexture( const vk::Device* devicePtr,  const vk::TextureCreateInfo& createInfo )
    {
		assert(devicePtr != nullptr);

        int width, height, nChannels;
        float* pixels = stbi_loadf(createInfo.fileNames[0].c_str(),
        	&width, &height, &nChannels, 4);

        if (pixels == nullptr)
        {
            throw std::runtime_error("PanoramicTexture::Create() failed!\n");
        }

		c_device = devicePtr->GetDevice();
        m_width = width;
        m_height = height;
		m_imageCount = createInfo.fileNames.size();
        m_imageLayerSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4 * sizeof(float);

        vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_imageLayerSize, pixels);

        VkImageCreateInfo panoramicImageCI = vk::init::ImageCreateInfo();
        panoramicImageCI.imageType = VK_IMAGE_TYPE_2D;
        panoramicImageCI.extent = { m_width, m_height, 1 };
        panoramicImageCI.arrayLayers = 1;
        panoramicImageCI.mipLevels = 1;
        panoramicImageCI.format = createInfo.format;
        panoramicImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        panoramicImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        panoramicImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        panoramicImageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        m_image = vk::util::CreateImage(devicePtr, panoramicImageCI, m_memory,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_descriptor.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(), m_image,
            createInfo.format, VK_IMAGE_VIEW_TYPE_2D);
        m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //this must be respected by the time its accessed in the compute shader
        m_descriptor.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);

        VkFence submissionFence = vk::init::CreateFence( devicePtr->GetDevice(), false );

        VkCommandPool graphicsCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::GRAPHICS).family);

        VkCommandBuffer graphicsCmd = vk::util::beginSingleTimeCommand(devicePtr->GetDevice(), graphicsCmdPool);

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_image,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		//copy buffer into image.
		{
			std::vector<VkBufferImageCopy> regions(m_imageCount);
			for (int i = 0; i < m_imageCount; ++i)
			{
				regions[i] = {};
				regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				regions[i].imageSubresource.mipLevel = 0;
				regions[i].imageSubresource.baseArrayLayer = i;
				regions[i].imageSubresource.layerCount = 1;
				regions[i].bufferOffset = m_imageLayerSize * i;
				regions[i].imageExtent =
				{
					m_width,
					m_height,
					1
				};
			}

			uint32_t regionCount = static_cast<uint32_t>(regions.size());

			vkCmdCopyBufferToImage(graphicsCmd, stagingBuffer.GetHandle(), m_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, regions.data());
		}

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_image, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


		//for each roughness value that's convoluted, store the blurrier results in the image's mipmap levels.
		m_prefilterMipLevels = vk::util::CalculateMipLevels(m_prefilterWidth, m_prefilterHeight);

		//environment + convolution + prefiltering.
		uint32_t layoutCount = 2 + m_prefilterMipLevels + 1;

    	CreateComputeDescriptorBuffer( devicePtr, layoutCount );

    	CreateComputePipelineLayout();

		uint32_t env_width = 512;
		CreateEnvironmentMapImage( devicePtr, graphicsCmd, env_width, env_width, 6,
			vk::util::CalculateMipLevels( env_width, env_width ) );

    	CreateIrradianceImage( devicePtr, graphicsCmd );

		std::vector<VkDescriptorImageInfo> prefilterInfos;
    	CreatePrefilterImage( devicePtr, graphicsCmd, prefilterInfos );

    	CreateBRDFLUTImage( devicePtr, graphicsCmd );

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMap.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    	vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
    		devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    		submissionFence, std::nullopt );

    	m_environmentMap.SetImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

        stbi_image_free(pixels);

		for ( auto& infos : prefilterInfos )
		{
			vkDestroyImageView(devicePtr->GetDevice(), infos.imageView, nullptr);

		}

    	vkDestroyFence(devicePtr->GetDevice(), submissionFence, nullptr);

    	vkFreeCommandBuffers(devicePtr->GetDevice(), graphicsCmdPool, 1, &graphicsCmd);
    	vkDestroyCommandPool(devicePtr->GetDevice(), graphicsCmdPool, nullptr);

		std::cout << "\033[32m" << "successfully loaded Panormaic Texture in PanoramicTexture::Create()... " << "\033[0m\n";
    }

	PanoramicTexture::PanoramicTexture( PanoramicTexture&& other ) noexcept : Texture(std::move(other))
	{
		if (this != &other)
		{
			this->m_irradianceImage = other.m_irradianceImage;
			this->m_prefilterImage = other.m_prefilterImage;
			this->m_BRDFLUTImage = other.m_BRDFLUTImage;

			this->m_irradianceImageMemory = other.m_irradianceImageMemory;
			this->m_prefilterImageMemory = other.m_prefilterImageMemory;
			this->m_BRDFLUTImageMemory = other.m_BRDFLUTImageMemory;

			this->m_irradianceInfo = other.m_irradianceInfo;
			this->m_prefilterInfo = other.m_prefilterInfo;
			this->m_BRDFLUTInfo = other.m_BRDFLUTInfo;

			this->m_computePipeline = other.m_computePipeline;
			this->m_convolutionPipeline = other.m_convolutionPipeline;
			this->m_prefilterPipeline = other.m_prefilterPipeline;
			this->m_BRDFLUTPipeline = other.m_BRDFLUTPipeline;

			this->m_prefilterWidth = other.m_prefilterWidth;
			this->m_prefilterHeight = other.m_prefilterHeight;
			this->m_prefilterMipLevels = other.m_prefilterMipLevels;

			this->m_environmentMap = std::move(other.m_environmentMap);
			this->m_computeDescriptorBuffer = std::move(other.m_computeDescriptorBuffer);

			this->m_computePipelineLayout = other.m_computePipelineLayout;

			other.c_device = VK_NULL_HANDLE;
		}
	}

	PanoramicTexture& PanoramicTexture::operator=( PanoramicTexture&& other ) noexcept
	{
		if (this != &other)
		{
			Texture::operator=(std::move(other));

			std::swap(this->m_environmentMap, other.m_environmentMap);
			std::swap(this->m_irradianceImage, other.m_irradianceImage);
			std::swap(this->m_prefilterImage, other.m_prefilterImage);
			std::swap(this->m_BRDFLUTImage, other.m_BRDFLUTImage);

			std::swap(this->m_irradianceImageMemory, other.m_irradianceImageMemory);
			std::swap(this->m_prefilterImageMemory, other.m_prefilterImageMemory);
			std::swap(this->m_BRDFLUTImageMemory, other.m_BRDFLUTImageMemory);

			std::swap(this->m_irradianceInfo, other.m_irradianceInfo);
			std::swap(this->m_prefilterInfo, other.m_prefilterInfo);
			std::swap(this->m_BRDFLUTInfo, other.m_BRDFLUTInfo);

			std::swap(this->m_computePipeline, other.m_computePipeline);
			std::swap(this->m_convolutionPipeline, other.m_convolutionPipeline);
			std::swap(this->m_prefilterPipeline, other.m_prefilterPipeline);
			std::swap(this->m_BRDFLUTPipeline, other.m_BRDFLUTPipeline);

			std::swap(this->m_prefilterWidth, other.m_prefilterWidth);
			std::swap(this->m_prefilterHeight, other.m_prefilterHeight);
			std::swap(this->m_prefilterMipLevels, other.m_prefilterMipLevels);

			std::swap(this->m_computeDescriptorBuffer, other.m_computeDescriptorBuffer);

			std::swap(this->m_computePipelineLayout, other.m_computePipelineLayout);
		}

		return *this;
	}

	PanoramicTexture::~PanoramicTexture()
	{
		if (c_device != VK_NULL_HANDLE)
		{
			vkDestroyImage(c_device, m_irradianceImage, nullptr);
			vkFreeMemory(c_device, m_irradianceImageMemory, nullptr);

			vkDestroyImage(c_device, m_prefilterImage, nullptr);
			vkFreeMemory(c_device, m_prefilterImageMemory, nullptr);

			vkDestroyImage(c_device, m_BRDFLUTImage, nullptr);
			vkFreeMemory(c_device, m_BRDFLUTImageMemory, nullptr);

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
		}
	}

	void PanoramicTexture::WriteToEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd )
    {
    	//write to descriptor buffer
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

	    writeResource.pImageData = &m_descriptor;
	    m_computeDescriptorBuffer.WriteDescriptor(writeResource,
		    0,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

		auto environmentMapDescriptor = m_environmentMap.GetDescriptor();
	    writeResource.pImageData = &environmentMapDescriptor;
	    m_computeDescriptorBuffer.WriteDescriptor(writeResource,
		    0, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

	    //create compute pipeline
		m_computePipeline = CreateComputePipeline( "equirectangular-to-cubemap.comp" );

		vkCmdBindPipeline(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

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

		vkCmdDispatch(graphicsCmd, 512 / 16, 512 / 16, 6);

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMap.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );

		m_environmentMap.SetImageLayout( VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
    }

	void PanoramicTexture::CreateEnvironmentMapImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels )
    {
		vk::TextureCreateInfo textureCI = {};
		textureCI.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		textureCI.width = width;
		textureCI.height = height;
		textureCI.layerCount = layerCount;
		textureCI.mipLevels = mipLevels;
		textureCI.imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		textureCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        m_environmentMap = vk::Texture( devicePtr, textureCI );

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMap.GetImage(),
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    	WriteToEnvironmentMapImage( devicePtr, graphicsCmd );

    	//generating mip maps of the environment map
		if (mipLevels > 1)
		{
			//have to set to transfer dst optimal because recording the blit operations assumes that's where the image
			//starts from...
			vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMap.GetImage(),
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			vk::util::RecordBlitMipMapImages(graphicsCmd,  m_environmentMap.GetImage(),
				width, height, mipLevels, layerCount);

			//and because the blit commands transition everything to read_only, we have to transition the layout
			//to src_optimal again
			vk::util::RecordImageLayoutTransition( graphicsCmd,  m_environmentMap.GetImage(),
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		}
    }

	void PanoramicTexture::WriteToIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd )
    {
    	//write to descriptor buffer
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

    	//NOTE: cubemapInfo.imageLayout changed between CreateCubeMap() -> WriteToIrradianceImage()
		auto environmentMapDescriptor = m_environmentMap.GetDescriptor();
    	writeResource.pImageData = &environmentMapDescriptor;
    	m_computeDescriptorBuffer.WriteDescriptor(writeResource,
			1,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

    	writeResource.pImageData = &m_irradianceInfo;
    	m_computeDescriptorBuffer.WriteDescriptor( writeResource,
			1, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

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

		VkDeviceSize bufferOffset = m_computeDescriptorBuffer.GetLayoutSize(); //since this is index 1.
		uint32_t descriptorIndex = 0;

		g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

		vkCmdDispatch(graphicsCmd, 32 / 16, 32 / 16, 6);

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_irradianceImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    	m_irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

	void PanoramicTexture::CreateIrradianceImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd )
    {
    	uint32_t width = 32, height = 32;

        VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
        imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.arrayLayers = 6;
        //NOTE: not sure yet how to divide up the resolution. the image I'm sampling is 2048x1024 pixels.
        imageCI.extent.width = width;
        imageCI.extent.height = height;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = 1;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        m_irradianceImage = vk::util::CreateImage(devicePtr, imageCI, m_irradianceImageMemory,
        	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_irradianceImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    	m_irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    	m_irradianceInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
			m_irradianceImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    	m_irradianceInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);

    	WriteToIrradianceImage( devicePtr, graphicsCmd );
    }

	void PanoramicTexture::WriteToPrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		std::vector<VkDescriptorImageInfo>& prefilterInfos )
    {
    	prefilterInfos.resize(m_prefilterMipLevels);

    	VkImageViewCreateInfo viewCI = vk::init::ImageViewCreateInfo();
    	viewCI.image = m_prefilterImage;
    	viewCI.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    	viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    	viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    	viewCI.subresourceRange.baseMipLevel = 0;
    	viewCI.subresourceRange.levelCount = 1;
    	viewCI.subresourceRange.baseArrayLayer = 0;
    	viewCI.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    	viewCI.components = vk::init::ComponentMappingSwizzleIdentity();

    	VkSampler sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(),
    		m_prefilterMipLevels);

    	for (uint32_t mip = 0; mip < m_prefilterMipLevels; ++mip)
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

    	for (uint32_t mip = 0; mip < m_prefilterMipLevels; ++mip)
    	{
    		writeResource.pImageData = &environmentMapDescriptor;
    		m_computeDescriptorBuffer.WriteDescriptor(writeResource,
				2 + mip,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

    		writeResource.pImageData = &prefilterInfos[mip];
    		m_computeDescriptorBuffer.WriteDescriptor( writeResource,
				2 + mip, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);
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

    	for (uint32_t mip = 0; mip < m_prefilterMipLevels; ++mip)
    	{
    		VkDeviceSize bufferOffset = initOffset + mip * layoutSize;

    		g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
				m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

    		float roughness = static_cast<float>(mip) / static_cast<float>(m_prefilterMipLevels - 1);

    		vkCmdPushConstants(graphicsCmd, m_computePipelineLayout,
    			VK_SHADER_STAGE_COMPUTE_BIT,
    			0, sizeof(float), &roughness);

    		vkCmdDispatch(graphicsCmd, m_prefilterWidth / 16, m_prefilterHeight / 16, 6);
    	}

    	vk::util::RecordImageLayoutTransition(graphicsCmd, m_prefilterImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_GENERAL,
    		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    	//cleanup the temporary image views

    	m_prefilterInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    	m_prefilterInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
    		m_prefilterImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    	m_prefilterInfo.sampler = sampler;
    }

	void PanoramicTexture::CreatePrefilterImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		std::vector<VkDescriptorImageInfo>& prefilterInfos )
	{
		VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
		imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.arrayLayers = 6;
		imageCI.extent.width = m_prefilterWidth;
		imageCI.extent.height = m_prefilterHeight;
		imageCI.extent.depth = 1;
		imageCI.mipLevels = m_prefilterMipLevels;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; //this image will transfer to its mipped images
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		m_prefilterImage = vk::util::CreateImage( devicePtr, imageCI, m_prefilterImageMemory,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		vk::util::RecordImageLayoutTransition( graphicsCmd, m_prefilterImage,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

		WriteToPrefilterImage( devicePtr, graphicsCmd, prefilterInfos );
	}

	void PanoramicTexture::WriteToBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd )
    {
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

    	uint32_t layoutIndex = 2 + m_prefilterMipLevels;

    	writeResource.pImageData = &m_BRDFLUTInfo;
    	m_computeDescriptorBuffer.WriteDescriptor( writeResource,
			layoutIndex, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

		m_BRDFLUTPipeline = CreateComputePipeline( "BRDF-convolute.comp" );

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

    	vkCmdDispatch(graphicsCmd, 512 / 16, 512 / 16, 1);

    	vk::util::RecordImageLayoutTransition(graphicsCmd, m_BRDFLUTImage,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

	void PanoramicTexture::CreateBRDFLUTImage( const vk::Device* devicePtr, VkCommandBuffer graphicsCmd )
    {
    	VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
    	imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
    	imageCI.imageType = VK_IMAGE_TYPE_2D;
    	imageCI.arrayLayers = 1;
	    imageCI.mipLevels = 1;
	    imageCI.extent.width = 512;
	    imageCI.extent.height = 512;
	    imageCI.extent.depth = 1;
	    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    	imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT; //this image will transfer to its mipped images
    	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    	m_BRDFLUTImage = vk::util::CreateImage( devicePtr, imageCI, m_BRDFLUTImageMemory,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_BRDFLUTImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

    	m_BRDFLUTInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    	m_BRDFLUTInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
    		m_BRDFLUTImage, imageCI.format, VK_IMAGE_VIEW_TYPE_2D);
    	m_BRDFLUTInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);

		WriteToBRDFLUTImage( devicePtr, graphicsCmd );

    	m_BRDFLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}