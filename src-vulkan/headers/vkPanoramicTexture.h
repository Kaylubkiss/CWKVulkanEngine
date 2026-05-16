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

    			vkDestroyPipeline(c_device, m_computePipeline, nullptr);
    			vkDestroyPipeline(c_device, m_convolutionPipeline, nullptr);

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

    	void CreateEnvironmentComputePipeline( vk::Device* devicePtr )
    	{
			VkComputePipelineCreateInfo computePipelineCI = {};
			computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(devicePtr->GetDevice(),
				"equirectangular-to-cubemap.comp", VK_SHADER_STAGE_COMPUTE_BIT);

			VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.mHandle, shaderModuleInfo.mFlags);

			computePipelineCI.stage = shaderStageCI;
			computePipelineCI.layout = m_computePipelineLayout;

			vkCreateComputePipelines(devicePtr->GetDevice(), VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &m_computePipeline);

			vkDestroyShaderModule(devicePtr->GetDevice(), shaderModuleInfo.mHandle, nullptr);

    	}

    	void CreateIrradianceComputePipeline( vk::Device* devicePtr )
    	{
    		VkComputePipelineCreateInfo computePipelineCI = {};
    		computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    		computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

    		vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(devicePtr->GetDevice(),
				"convolute-cubemap.comp", VK_SHADER_STAGE_COMPUTE_BIT);

    		VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.mHandle, shaderModuleInfo.mFlags);

    		computePipelineCI.stage = shaderStageCI;
    		computePipelineCI.layout = m_computePipelineLayout;

    		vkCreateComputePipelines(devicePtr->GetDevice(), VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &m_convolutionPipeline);

    		vkDestroyShaderModule(devicePtr->GetDevice(), shaderModuleInfo.mHandle, nullptr);
    	}

    	void CreatePrefilterComputePipeline( vk::Device* devicePtr )
    	{
    		VkComputePipelineCreateInfo computePipelineCI = {};
    		computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    		computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

    		vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(devicePtr->GetDevice(),
				"prefilter-cubemap.comp", VK_SHADER_STAGE_COMPUTE_BIT);

    		VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.mHandle, shaderModuleInfo.mFlags);

    		computePipelineCI.stage = shaderStageCI;
    		computePipelineCI.layout = m_computePipelineLayout;

    		vkCreateComputePipelines(devicePtr->GetDevice(), VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &m_prefilterPipeline);

    		vkDestroyShaderModule(devicePtr->GetDevice(), shaderModuleInfo.mHandle, nullptr);
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

            m_computeDescriptorBuffer.Allocate(devicePtr, bufferUsageFlags, memoryProperties,
                1, 3, layoutBindings);

        }

        void WriteToEnvironmentMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, VkFence submissionFence, std::mutex& submissionMutex )
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
			CreateEnvironmentComputePipeline( devicePtr );

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

        void CreateEnvironmentMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	VkFence submissionFence, std::mutex& submissionMutex )
        {
            if (devicePtr == nullptr)
            {
                return;
            }

            VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
            imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.arrayLayers = 6;
            //NOTE: not sure yet how to divide up the resolution. the image I'm sampling is 2048x1024 pixels.
            imageCI.extent.width = 512;
            imageCI.extent.height = 512;
            imageCI.extent.depth = 1;
            imageCI.mipLevels = 1;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
            	VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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

    		m_environmentMapInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice());

    		WriteToEnvironmentMapImage(devicePtr, graphicsCmd, submissionFence, submissionMutex );
        }

    	void WriteToIrradianceImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, VkFence submissionFence, std::mutex& submissionMutex )
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

			CreateIrradianceComputePipeline( devicePtr );

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

			vkCmdDispatch(graphicsCmd, 512 / 16, 512 / 16, 6);

    		vk::util::RecordImageLayoutTransition( graphicsCmd, m_irradianceImage,
    			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt);

    		m_irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    	}

    	void CreateIrradianceImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence, std::mutex& submissionMutex )
    	{

    		assert(m_environmentMapImage != VK_NULL_HANDLE); //must be able to sample from the cubemap image during creation.

			if (devicePtr == nullptr)
            {
                return;
            }

            VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
            imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.arrayLayers = 6;
            //NOTE: not sure yet how to divide up the resolution. the image I'm sampling is 2048x1024 pixels.
            imageCI.extent.width = 512;
            imageCI.extent.height = 512;
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
    			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue(devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt);

    		m_irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    		m_irradianceInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
				m_irradianceImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    		m_irradianceInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice());

    		WriteToIrradianceImage( devicePtr, graphicsCmd, submissionFence, submissionMutex );
    	}

    	void WriteToPrefilterImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, uint32_t mipLevels,
    		VkFence submissionFence, std::mutex& submissionMutex )
    	{
    		vk::WriteResource writeResource = {};
    		auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

    		//NOTE: cubemapInfo.imageLayout changed between CreateCubeMap() -> WriteToIrradianceImage()
    		writeResource.pImageData = &m_environmentMapInfo;
    		m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
				2,0,0, descriptorBufferProperties.combinedImageSamplerDescriptorSize);

    		writeResource.pImageData = &m_irradianceInfo;
    		m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
				2, 0, 1, descriptorBufferProperties.storageImageDescriptorSize, true);

			CreatePrefilterComputePipeline( devicePtr );

    		VkCommandBufferBeginInfo beginInfo = {};
    		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    		uint32_t initWidth = 128, initHeight = 128;
    		for (uint32_t i = 0; i < mipLevels; ++i)
    		{
				uint32_t mipWidth = static_cast<uint32_t>(initWidth * std::pow(0.5, mipLevels));
    			uint32_t mipHeight = static_cast<uint32_t>(initHeight * std::pow(0.5, mipLevels));




    		}

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));


    	}

    	void CreatePrefilterImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
    		VkFence submissionFence, std::mutex& submissionMutex )
    	{

    		assert(m_environmentMapImage != VK_NULL_HANDLE);

    		if (devicePtr == nullptr)
    		{
    			return;
    		}

    		VkSubmitInfo submitInfo = {};

    		uint32_t imageWidth = 128;
    		uint32_t imageHeight = 128;

    		//for each roughness value that's convoluted, store the blurrier results in the image's mipmap levels.
    		uint32_t mipLevels = vk::util::CalculateMipLevels(imageWidth, imageHeight);

    		VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
    		imageCI.format = VK_FORMAT_R32G32B32A32_SFLOAT; //for now, just assume this format --> biggest possible
    		imageCI.imageType = VK_IMAGE_TYPE_2D;
    		imageCI.arrayLayers = 6;
    		//NOTE: not sure yet how to divide up the resolution. the image I'm sampling is 2048x1024 pixels.
    		imageCI.extent.width = imageWidth;
    		imageCI.extent.height = imageHeight;
    		imageCI.extent.depth = 1;
    		imageCI.mipLevels = mipLevels;
    		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    		imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
    			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; //this image will transfer to its mipped images
    		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    		imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    		m_prefilterImage = vk::init::CreateImage(devicePtr, imageCI, m_prefilterImageMemory,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    		VkCommandBufferBeginInfo beginInfo = {};
    		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    		VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    		vk::util::RecordImageLayoutTransition( graphicsCmd, m_prefilterImage,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

    		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

    		vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
				devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
				submissionFence, std::nullopt );

			// copy the environment map into the prefilter map and then generate its mipmaps.
			{
    			auto blitWidth = static_cast<int32_t>(imageWidth);
    			auto blitHeight = static_cast<int32_t>(imageHeight);

    			VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

    			for (int i = 0; i < 6; ++i)
    			{
    				VkImageBlit blit = {};
    				blit.srcOffsets[0] = { 0,0,0 };
    				blit.srcOffsets[1] = { 512, 512, 1 }; // NOTE: hardcoded size of environment faces
    				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    				blit.srcSubresource.mipLevel = 0;
    				blit.srcSubresource.baseArrayLayer = i;
    				blit.srcSubresource.layerCount = 1;

    				blit.dstOffsets[0] = { 0,0,0 };
    				blit.dstOffsets[1] = { blitWidth, blitHeight, 1 };
    				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    				blit.dstSubresource.mipLevel = 0;
    				blit.dstSubresource.baseArrayLayer = i;
    				blit.dstSubresource.layerCount = 1;

    				vkCmdBlitImage(graphicsCmd,
						m_environmentMapImage,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						m_prefilterImage,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1,
						&blit, VK_FILTER_LINEAR);
    			}

    			//this generates the mipmaps
    			vk::util::RecordBlitMipMapImages( graphicsCmd, m_prefilterImage, blitWidth, blitHeight, mipLevels, 6);

    			VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));
    		}

    		vk::util::SubmitCommandToQueue( devicePtr->GetDevice(), graphicsCmd,
    			devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle,
    			submissionFence, std::nullopt );

    		m_prefilterInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    		m_prefilterInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
				m_prefilterImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    		m_prefilterInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice());

			//TODO: UNFINISHED...

    	}
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

    	VkImage m_irradianceImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_irradianceImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_irradianceInfo = {};

    	VkImage m_prefilterImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_prefilterImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_prefilterInfo = {};

    	vk::DescriptorBuffer m_computeDescriptorBuffer;

    };


}