#include "vkPanoramicTexture.h"
#include "vkInit.h"

namespace vk
{
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

        m_descriptor.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice());

        m_imageView = m_descriptor.imageView;


        VkSubmitInfo submitInfo = {};

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

		CreateEnvironmentMapImage( devicePtr, graphicsCmd, submissionFence, transferMutex );

    	CreateIrradianceImage( devicePtr, graphicsCmd, submissionFence, transferMutex );

    	CreatePrefilterImage( devicePtr, graphicsCmd, submissionFence, transferMutex );

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