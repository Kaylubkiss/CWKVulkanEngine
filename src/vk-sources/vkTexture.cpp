#include "vkTexture.h"
#include "vkUtility.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vk
{

	VkImageView Texture::CreateImageView( VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels )
	{

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = textureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView nTextImageView;
		VK_CHECK_RESULT(vkCreateImageView(l_device, &viewInfo, nullptr, &nTextImageView));

		return nTextImageView;
	}

	VkSampler Texture::CreateSampler( VkPhysicalDevice p_device, VkDevice l_device, uint32_t mipLevels )
	{
		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		createInfo.addressModeV = createInfo.addressModeU;
		createInfo.addressModeW = createInfo.addressModeU;


		VkPhysicalDeviceProperties pdp = { };
		vkGetPhysicalDeviceProperties(p_device, &pdp);

		createInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy / 2.f;
		createInfo.anisotropyEnable = VK_TRUE;

		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_NEVER;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minLod = 0.f;
		createInfo.maxLod = static_cast<float>(mipLevels);
		createInfo.mipLodBias = 0.f; //optional...

		VkSampler nTextureSampler;
		VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

		return nTextureSampler;
	}


	Texture::Texture( const vk::Device* devicePtr, const std::string& filePath, std::mutex& transferMutex )
	{

		assert(devicePtr);
		//Might want to make command pool a member variable.

		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = filePath.empty() ? nullptr : stbi_load(filePath.c_str(),
			&textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		if (pixels == nullptr)
		{
			std::cerr << "could not load in specified texture " + filePath << std::endl;
			throw std::runtime_error("Texture() FAILED");
		}

		constexpr uint64_t num_channels = 4;
		VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth) *
			static_cast<uint64_t>(textureHeight) * num_channels;

		uint32_t mipLevels = 1;

		vk::Buffer stagingBuffer = vk::Buffer(devicePtr,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			static_cast<size_t>(imageSize), pixels);

		m_image = vk::init::CreateImage(devicePtr->GetGPU(),
			devicePtr->GetDevice(), textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_memory);

		VkCommandPool transferCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::TRANSFER).family);

		VkCommandBuffer transferCmd = beginSingleTimeCommand(devicePtr->GetDevice(), transferCmdPool);
		VkSubmitInfo submitInfo = {};

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


			vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&barrier); //asking the gpu to reconfigure the old image layout to the new layout.

			VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCmd;

			{
				std::lock_guard<std::mutex> lock(transferMutex);
				VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
					VK_NULL_HANDLE));
			}

			VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle));
		}

		//copy buffer into image.
		{
			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0,0,0 };
			region.imageExtent =
			{
				static_cast<uint32_t>(textureWidth),
				static_cast<uint32_t>(textureHeight),
				1
			};

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

			vkCmdCopyBufferToImage(transferCmd, stagingBuffer.GetHandle(), m_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCmd;

			{
				std::lock_guard<std::mutex> lock(transferMutex);
				VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle,
					1, &submitInfo, VK_NULL_HANDLE));
			}

			VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle));
		}

		//release transfer queue to graphics queue
		{
			VkImageMemoryBarrier releaseBarrier = {};
			releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			releaseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			releaseBarrier.srcQueueFamilyIndex = devicePtr->GetQueue(DeviceQueue::TRANSFER).family;
			releaseBarrier.dstQueueFamilyIndex = devicePtr->GetQueue(DeviceQueue::GRAPHICS).family;
			releaseBarrier.image = m_image;
			releaseBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			releaseBarrier.subresourceRange.baseMipLevel = 0;
			releaseBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			releaseBarrier.subresourceRange.baseArrayLayer = 0;
			releaseBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

			vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.

			VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCmd;

			{
				std::lock_guard<std::mutex> lock(transferMutex);
				VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
					VK_NULL_HANDLE));
			}

			VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle));
		}

		vkFreeCommandBuffers(devicePtr->GetDevice(), transferCmdPool, 1, &transferCmd);
		vkDestroyCommandPool(devicePtr->GetDevice(), transferCmdPool, nullptr);

		stagingBuffer.Destroy();
		stbi_image_free(pixels);

		m_imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(), m_image, 1);
		m_sampler   = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1);
		m_descriptor.imageView = m_imageView;
		m_descriptor.sampler = m_sampler;
		m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		c_device = devicePtr->GetDevice();
	}

	Texture::~Texture()
	{
		if (c_device != VK_NULL_HANDLE)
		{
			if (m_sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(c_device, m_sampler, nullptr);
				m_sampler = VK_NULL_HANDLE;
			}

			if (m_imageView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(c_device, m_imageView, nullptr);
				m_imageView = VK_NULL_HANDLE;
			}

			if (m_image != VK_NULL_HANDLE)
			{
				vkDestroyImage(c_device, m_image, nullptr);
				m_image = VK_NULL_HANDLE;
			}

			if (m_memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(c_device, m_memory, nullptr);
				m_memory = VK_NULL_HANDLE;
			}
		}
	}


	VkDescriptorImageInfo Texture::GetDescriptor() const
	{
		return m_descriptor;
	}

	VkImage Texture::GetImage() const
	{
		return m_image;
	}


}