#pragma once
#include "vkInit.h"
#include "vkTexture.h"

//WIP

namespace vk
{
    class PanoramicTexture : public Texture //technically a type of cubemap, but inheriting cubemap would be useless.
    {
    public:
    	~PanoramicTexture() override
    	{
    		if (c_device != VK_NULL_HANDLE)
    		{
    			vkDestroyImage(c_device, m_cubemapImage, nullptr);
    			vkFreeMemory(c_device, m_cubemapImageMemory, nullptr);

    			vkDestroySampler(c_device, m_cubemapInfo.sampler, nullptr);
    			vkDestroyImageView(c_device, m_cubemapInfo.imageView, nullptr);

    			vkDestroyPipeline(c_device, m_computePipeline, nullptr);
    			vkDestroyPipelineLayout(c_device, m_computePipelineLayout, nullptr);

    			m_computeDescriptorBuffer.Destroy();
    		}
    	}

		void CreateComputePipelineLayout( vk::Device* devicePtr )
    	{
			VkPipelineLayoutCreateInfo pipelineLayoutCI = vk::init::PipelineLayoutCreateInfo();

    		VkDescriptorSetLayout desciptorSetLayout = m_computeDescriptorBuffer.GetLayout();

    		pipelineLayoutCI.pSetLayouts = &desciptorSetLayout;
			pipelineLayoutCI.setLayoutCount = 1;

			vkCreatePipelineLayout(devicePtr->GetDevice(), &pipelineLayoutCI, nullptr, &m_computePipelineLayout);
		}

    	void CreateComputePipeline( vk::Device* devicePtr )
    	{
			CreateComputePipelineLayout( devicePtr );

			VkComputePipelineCreateInfo computePipelineCI = {};
			computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCI.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			vk::ShaderModuleInfo shaderModuleInfo = vk::ShaderModuleInfo(devicePtr->GetDevice(),
				"equirectangular-cubemap-convert.comp", VK_SHADER_STAGE_COMPUTE_BIT);

			VkPipelineShaderStageCreateInfo shaderStageCI =
				vk::init::PipelineShaderStageCreateInfo(shaderModuleInfo.mHandle, shaderModuleInfo.mFlags);

			computePipelineCI.stage = shaderStageCI;
			computePipelineCI.layout = m_computePipelineLayout;

			vkCreateComputePipelines(devicePtr->GetDevice(), VK_NULL_HANDLE, 1,
				&computePipelineCI, nullptr, &m_computePipeline);

			vkDestroyShaderModule(devicePtr->GetDevice(), shaderModuleInfo.mHandle, nullptr);

    	}

        void CreateCubeMapDescriptor( vk::Device* devicePtr )
        {

            VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT ;

            //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT

            VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            std::vector<VkDescriptorSetLayoutBinding> layoutBindings =
            {
            	{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                }
            };

            m_computeDescriptorBuffer.Allocate(devicePtr, bufferUsageFlags, memoryProperties,
                1, 1, layoutBindings);

        }

        void WriteToCubeMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd, VkFence submissionFence, std::mutex& submissionMutex )
        {
            CreateCubeMapDescriptor( devicePtr );

    		auto descriptorBufferProperties = devicePtr->GetDescriptorBufferProperties();

            vk::WriteResource writeResource = {};

            writeResource.pImageData = &m_cubemapInfo;

            m_computeDescriptorBuffer.WriteDescriptor(devicePtr, writeResource,
                0, 0, 0, descriptorBufferProperties.storageImageDescriptorSize);

        	//create compute pipeline
			CreateComputePipeline( devicePtr );

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

			//1 bind pipline
			vkCmdBindPipeline(graphicsCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

			//2 bind descriptor buffer

			//TODO (possibly): usage flag (from creation and this binding) may need to be changed -- unsure if i need to set storage buffer bit
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

            //TODO: fix hardcoding on numerator (image width and height).

			vkCmdDispatch(graphicsCmd, 512 / 16, 512 / 16, 6);

			VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

			//submit to queue!!
			{
            	VkSubmitInfo submitInfo = {};
            	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            	submitInfo.commandBufferCount = 1;
            	submitInfo.pCommandBuffers = &graphicsCmd;

            	std::lock_guard lock(submissionMutex);
            	VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
					submissionFence));
			}

			VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));

    		//transition the image layout to shader read_only.
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = m_cubemapImage;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            	VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

                vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0,
                    nullptr, 0, nullptr, 1,
                    &barrier); //asking the gpu to reconfigure the old image layout to the new layout.

            	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));
            }

            //submit to queue!!
            {
                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &graphicsCmd;

                std::lock_guard lock(submissionMutex);
                VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
                    submissionFence));
            }

            VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
            VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));
        }

        void CreateCubeMapImage( vk::Device* devicePtr, VkCommandBuffer graphicsCmd,
        	VkFence submissionFence, std::mutex& submissionMutex )
        {
            if (devicePtr == nullptr)
            {
                return;
            }

        	VkSubmitInfo submitInfo = {};

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

            m_cubemapImage = vk::init::CreateImage(devicePtr, imageCI, m_cubemapImageMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            //transition the image layout to general.
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = m_cubemapImage;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

            	VkCommandBufferBeginInfo beginInfo = {};
            	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            	VK_CHECK_RESULT(vkBeginCommandBuffer(graphicsCmd, &beginInfo));

                vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0, 0,
                    nullptr, 0, nullptr, 1,
                    &barrier); //asking the gpu to reconfigure the old image layout to the new layout.
            }

        	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

            //submit to queue!!
            {
                submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &graphicsCmd;

                std::lock_guard lock(submissionMutex);
                VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
                    submissionFence));
            }

            VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
            VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));


    		m_cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    		m_cubemapInfo.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(),
				m_cubemapImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_CUBE);

    		m_cubemapInfo.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice());

        }

        void TransitionImageLayoutAndWriteToCubeMap( vk::Device* devicePtr, vk::Buffer& stagingBuffer, std::mutex& submissionMutex )
        {

        	VkSubmitInfo submitInfo = {};

        	VkFence submissionFence = vk::init::CreateFence(devicePtr->GetDevice(), false);

            VkCommandPool graphicsCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::GRAPHICS).family);

        	VkCommandBuffer graphicsCmd = vk::util::beginSingleTimeCommand(devicePtr->GetDevice(), graphicsCmdPool);

			//transition image to dst-optimal layout so the staging buffer can be copied into it.
			{
				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = m_image;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				vkCmdPipelineBarrier(graphicsCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0, 0,
					nullptr, 0, nullptr, 1,
					&barrier); //asking the gpu to reconfigure the old image layout to the new layout.
			}

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

        	VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

	        {
            	submitInfo = {};
            	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            	submitInfo.commandBufferCount = 1;
            	submitInfo.pCommandBuffers = &graphicsCmd;

            	std::lock_guard lock(submissionMutex);
            	VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::GRAPHICS).handle, 1, &submitInfo,
					submissionFence));
	        }

        	VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
        	VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));

        	//creating and writing to the cubemap.

			CreateCubeMapImage(devicePtr, graphicsCmd,
				submissionFence, submissionMutex);

			WriteToCubeMapImage(devicePtr, graphicsCmd, submissionFence, submissionMutex );

			//free resource!!!

        	vkDestroyFence(devicePtr->GetDevice(), submissionFence, nullptr);

        	vkFreeCommandBuffers(devicePtr->GetDevice(), graphicsCmdPool, 1, &graphicsCmd);
        	vkDestroyCommandPool(devicePtr->GetDevice(), graphicsCmdPool, nullptr);
        }

        void Create( vk::Device* devicePtr,  const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex ) override;

    private:
    	VkImage m_cubemapImage = VK_NULL_HANDLE;
    	VkDeviceMemory m_cubemapImageMemory = VK_NULL_HANDLE;
    	VkDescriptorImageInfo m_cubemapInfo = {};

    	VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
    	VkPipeline m_computePipeline = VK_NULL_HANDLE;

    	vk::DescriptorBuffer m_computeDescriptorBuffer;

    };


}