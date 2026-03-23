#include "vkTexture.h"
#include "vkUtility.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vk
{
	bool Texture::doTransferSubmission( vk::Device* devicePtr, TransferSubmissionInfo& transferInfo )
	{
		VkDevice device = devicePtr->GetDevice();
		VkPhysicalDevice physicalDevice = devicePtr->GetGPU();

		constexpr uint64_t num_channels = 4;
		VkDeviceSize imageSize = static_cast<uint64_t>(m_width) *
			static_cast<uint64_t>(m_height) * num_channels;

		uint32_t mipLevels = 1;

		vk::Buffer stagingBuffer = vk::Buffer(devicePtr,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			static_cast<size_t>(imageSize), m_pixels);

		m_image = vk::init::CreateImage(physicalDevice,
			device, m_width, m_height, mipLevels, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_memory);

		VkSubmitInfo submitInfo = {};
		VkFence waitFence = vk::init::CreateFence(device, false);

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

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(transferInfo.transferCommand, &beginInfo));

			vkCmdPipelineBarrier(transferInfo.transferCommand, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&barrier); //asking the gpu to reconfigure the old image layout to the new layout.

			VK_CHECK_RESULT(vkEndCommandBuffer(transferInfo.transferCommand));

			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferInfo.transferCommand;

			{
				std::lock_guard<std::mutex> lock(*transferInfo.pTransferMutex);
				VK_CHECK_RESULT(vkQueueSubmit(transferInfo.transferQueue, 1, &submitInfo, waitFence));
			}

			VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(device, 1, &waitFence));
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
				static_cast<uint32_t>(m_width),
				static_cast<uint32_t>(m_height),
				1
			};

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(transferInfo.transferCommand, &beginInfo));

			vkCmdCopyBufferToImage(transferInfo.transferCommand, stagingBuffer.GetHandle(), m_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			VK_CHECK_RESULT(vkEndCommandBuffer(transferInfo.transferCommand));

			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferInfo.transferCommand;

			{
				std::lock_guard<std::mutex> lock(*transferInfo.pTransferMutex);
				VK_CHECK_RESULT(vkQueueSubmit(transferInfo.transferQueue, 1, &submitInfo, waitFence));
			}

			VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(device, 1, &waitFence));
		}

		//release transfer queue to graphics queue
		{
			VkImageMemoryBarrier releaseBarrier = {};
			releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			releaseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			releaseBarrier.srcQueueFamilyIndex = transferInfo.sourceQueueFamily;
			releaseBarrier.dstQueueFamilyIndex = transferInfo.destinationQueueFamily;
			releaseBarrier.image = m_image;
			releaseBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			releaseBarrier.subresourceRange.baseMipLevel = 0;
			releaseBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			releaseBarrier.subresourceRange.baseArrayLayer = 0;
			releaseBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			releaseBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(transferInfo.transferCommand, &beginInfo));

			vkCmdPipelineBarrier(transferInfo.transferCommand, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.

			VK_CHECK_RESULT(vkEndCommandBuffer(transferInfo.transferCommand));

			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferInfo.transferCommand;

			{
				std::lock_guard<std::mutex> lock(*transferInfo.pTransferMutex);
				VK_CHECK_RESULT(vkQueueSubmit(transferInfo.transferQueue, 1, &submitInfo, waitFence));
			}

			VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(vkResetFences(device, 1, &waitFence));
		}

		vkDestroyFence(device, waitFence, nullptr);

		stagingBuffer.Destroy();
		stbi_image_free(m_pixels);

		m_imageView = vk::Texture::CreateImageView(device, m_image, 1);
		m_sampler   = vk::Texture::CreateSampler(physicalDevice, device, 1);
		m_descriptor.imageView = m_imageView;
		m_descriptor.sampler = m_sampler;
		m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		return true;
	}


	bool Texture::doLoad( vk::Device* devicePtr, ResourceManager& resourceManager )
	{
		m_pixels = stbi_load(GetId().c_str(),
			&m_width, &m_height, &m_channels, STBI_rgb_alpha);

		if (m_pixels == nullptr) {
			std::cerr << "could not load in specified texture " + GetId() << std::endl;
			return false;
		}

		return true;
	}

	void Texture::doUnload( vk::Device* devicePtr )
	{
		if (devicePtr != nullptr)
		{
			const VkDevice device = devicePtr->GetDevice();

			if (m_sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(device, m_sampler, nullptr);
				m_sampler = VK_NULL_HANDLE;
			}

			if (m_imageView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(device, m_imageView, nullptr);
				m_imageView = VK_NULL_HANDLE;
			}

			if (m_image != VK_NULL_HANDLE)
			{
				vkDestroyImage(device, m_image, nullptr);
				m_image = VK_NULL_HANDLE;
			}

			if (m_memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(device, m_memory, nullptr);
				m_memory = VK_NULL_HANDLE;
			}
		}
	}


	VkImageView Texture::CreateImageView( VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels )
	{

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = textureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
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
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minLod = 0.f;
		createInfo.maxLod = static_cast<float>(mipLevels);
		createInfo.mipLodBias = 0.f; //optional...

		VkSampler nTextureSampler;
		VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

		return nTextureSampler;
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