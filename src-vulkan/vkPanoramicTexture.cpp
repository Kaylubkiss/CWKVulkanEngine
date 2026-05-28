#include "vkPanoramicTexture.h"
#include "vkInit.h"

namespace vk
{
	void PanoramicTexture::WriteToEnvironmentMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, VkFence submissionFence )
    {
    	//write to descriptor buffer
    	vk::WriteResource writeResource = {};
    	auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

	    writeResource.pImageData = &m_descriptor;
	    m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
		    0,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

	    writeResource.pImageData = &m_environmentMapInfo;
	    m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
		    0, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);


	    //create compute pipeline
		m_computePipeline = CreateComputePipeline( devicePtr, "equirectangular-to-cubemap.comp" );

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

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

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMapImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );

    	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

		vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
			submissionFence, std::nullopt);

    }

	void PanoramicTexture::CreateEnvironmentMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
		VkFence submissionFence )
    {
        if (devicePtr == nullptr)
        {
            return;
        }

    	uint32_t width = 512, height = 512;
    	uint32_t layers = 6;
    	uint32_t mipLevels = vk::util::CalculateMipLevels( width, height );

        VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
        imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.arrayLayers = layers;
        //NOTE: not sure yet how to divide up the resolution. the image I'm sampling is 2048x1024 pixels.
        imageCI.extent.width = width;
        imageCI.extent.height = height;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = mipLevels;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        m_environmentMapImage = vk::init::CreateImage(devicePtr, imageCI, m_environmentMapImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    	VkCommandBufferBeginInfo beginInfo = {};
    	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    	VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMapImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    	vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
    		devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    		submissionFence, std::nullopt);

    	m_environmentMapInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    	m_environmentMapInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
			m_environmentMapImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    	m_environmentMapInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), mipLevels);

    	WriteToEnvironmentMapImage(devicePtr, graphicsCmd, submissionFence );

    	//generating mip maps of the environment map

    	VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    	//have to set to transfer dst optimal because recording the blit operations assumes that's where the image
    	//starts from...

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMapImage,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    	vk::util::RecordBlitMipMapImages(graphicsCmd, m_environmentMapImage, width, height, mipLevels, layers);

    	//and because the blit commands transition everything to read_only, we have to transition the layout
    	//to src_optimal again... this is stupid AF.
    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMapImage,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    	vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
			submissionFence, std::nullopt);
    }

	void PanoramicTexture::WriteToIrradianceImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, VkFence submissionFence )
    	{
    		//write to descriptor buffer
    		vk::WriteResource writeResource = {};
    		auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

    		//NOTE: cubemapInfo.imageLayout changed between CreateCubeMap() -> WriteToIrradianceImage()
    		writeResource.pImageData = &m_environmentMapInfo;
    		m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
				1,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

    		writeResource.pImageData = &m_irradianceInfo;
    		m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
				1, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

			m_convolutionPipeline = CreateComputePipeline( devicePtr, "convolute-cubemap.comp" );

    		VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

			vkCmdBindPipeline(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_convolutionPipeline);

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

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt);

    		m_irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    	}

	void PanoramicTexture::CreateIrradianceImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence )
    	{

    		assert(m_environmentMapImage != VK_NULL_HANDLE); //must be able to sample from the cubemap image during creation.

			if (devicePtr == nullptr)
            {
                return;
            }

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

            m_irradianceImage = vk::init::CreateImage(devicePtr, imageCI, m_irradianceImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    		VkCommandBufferBeginInfo beginInfo = {};
    		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    		vk::util::RecordImageLayoutTransition( graphicsCmd, m_irradianceImage,
    			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt);

    		m_irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    		m_irradianceInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
				m_irradianceImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    		m_irradianceInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);

    		WriteToIrradianceImage( devicePtr, graphicsCmd, submissionFence );
    	}

	void PanoramicTexture::WriteToPrefilterImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence )
    	{
    		std::vector<VkDescriptorImageInfo> prefilterInfos(m_prefilterMipLevels);

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

    		for (uint32_t mip = 0; mip < m_prefilterMipLevels; ++mip)
    		{
    			writeResource.pImageData = &m_environmentMapInfo;
    			m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
					2 + mip,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

    			writeResource.pImageData = &prefilterInfos[mip];
    			m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
					2 + mip, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);
    		}

			m_prefilterPipeline = CreateComputePipeline( devicePtr, "prefilter-cubemap.comp" );

    		VkCommandBufferBeginInfo beginInfo = {};
    		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

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

    		for (uint32_t mip = 1; mip < m_prefilterMipLevels; ++mip)
    		{
    			VkDeviceSize bufferOffset = initOffset + mip * layoutSize;

    			g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
					m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

    			float roughness = (float)mip / (float)(m_prefilterMipLevels - 1);

    			vkCmdPushConstants(graphicsCmd, m_computePipelineLayout,
    				VK_SHADER_STAGE_COMPUTE_BIT,
    				0, sizeof(float), &roughness);

    			vkCmdDispatch(graphicsCmd, m_prefilterWidth / 16, m_prefilterHeight / 16, 6);
    		}

    		vk::util::RecordImageLayoutTransition(graphicsCmd, m_prefilterImage,
    			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    			VK_IMAGE_LAYOUT_GENERAL,
    			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt );

    		//cleanup the temporary image views
    		for (auto& infos : prefilterInfos)
    		{
				vkDestroyImageView(devicePtr->GetDevice(), infos.imageView, nullptr);
    		}

    		m_prefilterInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    		m_prefilterInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
    			m_prefilterImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    		m_prefilterInfo.sampler = sampler;
    	}

	void PanoramicTexture::CreatePrefilterImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
			VkFence submissionFence )
	{
		assert(m_environmentMapImage != VK_NULL_HANDLE);

		if (devicePtr == nullptr)
		{
			return;
		}

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

		m_prefilterImage = vk::init::CreateImage( devicePtr, imageCI, m_prefilterImageMemory,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

		vk::util::RecordImageLayoutTransition( graphicsCmd, m_prefilterImage,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

		vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
			submissionFence, std::nullopt );

		WriteToPrefilterImage( devicePtr, graphicsCmd, submissionFence );
	}

	void PanoramicTexture::WriteToBRDFLUTImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence )
    	{
    		vk::WriteResource writeResource = {};
    		auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

    		uint32_t layoutIndex = 2 + m_prefilterMipLevels;

    		writeResource.pImageData = &m_BRDFLUTInfo;
    		m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
				layoutIndex, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

			m_BRDFLUTPipeline = CreateComputePipeline( devicePtr, "BRDF-convolute.comp" );

    		VkCommandBufferBeginInfo beginInfo = {};
    		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

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
    		VkDeviceSize bufferOffset =  layoutSize * ( 2 + m_prefilterMipLevels );

    		uint32_t descriptorIndex = 0;

    		g_vkCmdSetDescriptorBufferOffsetsEXT(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE,
					m_computePipelineLayout, 0, 1, &descriptorIndex, &bufferOffset);

    		vkCmdDispatch(graphicsCmd, 512 / 16, 512 / 16, 1);

    		vk::util::RecordImageLayoutTransition(graphicsCmd, m_BRDFLUTImage,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
			submissionFence, std::nullopt );
    	}

	void PanoramicTexture::CreateBRDFLUTImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence )
    	{
    		if (devicePtr == nullptr)
    		{
    			return;
    		}

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

    		m_BRDFLUTImage = vk::init::CreateImage( devicePtr, imageCI, m_BRDFLUTImageMemory,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    		VkCommandBufferBeginInfo beginInfo = {};
    		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    		vk::util::RecordImageLayoutTransition( graphicsCmd, m_BRDFLUTImage,
    			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL );

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt );

    		m_BRDFLUTInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    		m_BRDFLUTInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
    			m_BRDFLUTImage, imageCI.format, VK_IMAGE_VIEW_TYPE_2D);
    		m_BRDFLUTInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);

			WriteToBRDFLUTImage( devicePtr, graphicsCmd, submissionFence );

    		m_BRDFLUTInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    	}

    void PanoramicTexture::Create( vk::Device* devicePtr,  const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex )
    {
        int width, height, nChannels;
        float* pixels = stbi_loadf(createInfos[0].name.c_str(), &width, &height, &nChannels, 4);

        if (pixels == nullptr)
        {
            throw std::runtime_error("PanoramicTexture::Create() failed!\n");
        }

        m_width = width;
        m_height = height;

        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4 * sizeof(float);

        m_imageCount = createInfos.size();
        m_imageLayerSize = imageSize; //technically, this will be transformed into a cubemap, so the image layer size will be divided by 6.

        vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, imageSize, pixels);

        VkImageCreateInfo panoramicImageCI = vk::init::ImageCreateInfo();
        panoramicImageCI.imageType = VK_IMAGE_TYPE_2D;
        panoramicImageCI.extent = { m_width, m_height, 1 };
        panoramicImageCI.arrayLayers = 1;
        panoramicImageCI.mipLevels = 1;
        panoramicImageCI.format = createInfos[0].format;
        panoramicImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        panoramicImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        panoramicImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        panoramicImageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        m_image = vk::init::CreateImage(devicePtr, panoramicImageCI, m_memory,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_descriptor.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(), m_image,
            createInfos[0].format, VK_IMAGE_VIEW_TYPE_2D);

        m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //this must be respected by the time its accessed in the compute shader

        m_descriptor.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);

    	m_sampler = m_descriptor.sampler;
        m_imageView = m_descriptor.imageView;

    	//for each roughness value that's convoluted, store the blurrier results in the image's mipmap levels.
    	m_prefilterMipLevels = vk::util::CalculateMipLevels(m_prefilterWidth, m_prefilterHeight);

        VkFence submissionFence = vk::init::CreateFence(devicePtr->GetDevice(), false);

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

        VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

        vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
        	devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
        	submissionFence, std::nullopt );


    	CreateComputeDescriptorBuffer( devicePtr );

    	CreateComputePipelineLayout( devicePtr );

		CreateEnvironmentMapImage( devicePtr, graphicsCmd, submissionFence );

    	CreateIrradianceImage( devicePtr, graphicsCmd, submissionFence );

    	CreatePrefilterImage( devicePtr, graphicsCmd, submissionFence );

    	CreateBRDFLUTImage( devicePtr, graphicsCmd, submissionFence );

    	VkCommandBufferBeginInfo beginInfo = {};
    	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    	VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    	vk::util::RecordImageLayoutTransition( graphicsCmd, m_environmentMapImage,
    		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    	vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
    		devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    		submissionFence, std::nullopt );

    	m_environmentMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::cout << "\033[32m" << "successfully loaded Panormaic Texture in PanoramicTexture::Create()... " << "\033[0m\n";

        stbi_image_free(pixels);
        stagingBuffer.Destroy();

    	vkDestroyFence(devicePtr->GetDevice(), submissionFence, nullptr);

    	vkFreeCommandBuffers(devicePtr->GetDevice(), graphicsCmdPool, 1, &graphicsCmd);
    	vkDestroyCommandPool(devicePtr->GetDevice(), graphicsCmdPool, nullptr);


        c_device = devicePtr->GetDevice();
    }
}