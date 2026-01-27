#include "vkTexture.h"
#include "vkUtility.h"
#include <stb_image.h>

namespace vk {


	VkImageView Texture::CreateImageView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels)
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

	VkSampler Texture::CreateSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels)
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

	Texture::~Texture() 
	{
		if (cLogicalDevice != VK_NULL_HANDLE) 
		{
			vkDestroySampler(cLogicalDevice, mSampler, nullptr);
			vkDestroyImageView(cLogicalDevice, mImageView, nullptr);
			vkDestroyImage(cLogicalDevice, mImage, nullptr);
			vkFreeMemory(cLogicalDevice, mMemory, nullptr);
			vkDestroySemaphore(cLogicalDevice, mTransferCompleteSemaphore, nullptr);
		}
	}

	Texture::Texture( vk::Device* devicePtr, const std::string& fileName )
	{

		assert(devicePtr);
		//Might want to make command pool a member variable.

		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = fileName == "" ? nullptr : stbi_load((TEXTURE_PATH + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		if (pixels == nullptr)
		{
			std::cerr << "could not load in specified texture " + std::string(TEXTURE_PATH + fileName) << std::endl;
			throw std::runtime_error("Texture() FAILED");
		}

		uint64_t bytePerPixel = 4;
		VkDeviceSize imageSize = (uint64_t)textureWidth * (uint64_t)textureHeight * bytePerPixel; //4 bytes per pixel.

		/*uint32_t mipLevels = vk::util::CalculateMipLevels(textureWidth, textureHeight); -- commented out because I don't understand it yet. */ 
		uint32_t mipLevels = 1;

		vk::Buffer stagingBuffer = vk::Buffer(devicePtr,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			static_cast<size_t>(imageSize), pixels);

		this->mImage = vk::init::CreateImage(devicePtr->physical,
			devicePtr->logical, textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->mMemory);

		VkCommandPool transferCmdPool = vk::init::CommandPool(devicePtr->logical, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, devicePtr->transferQueue.family);

		vk::util::TransitionImageLayout(devicePtr->logical, transferCmdPool, devicePtr->transferQueue.handle,
			 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, this->mImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
		
		vk::util::copyBufferToImage(devicePtr->logical, transferCmdPool,
			stagingBuffer.GetHandle(),
			devicePtr->transferQueue.handle,
			this->mImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight)); //copy contents of the image (stored in buffer) into the image.

		//release transfer queue
		VkCommandBuffer transferCmd = beginSingleTimeCommand(devicePtr->logical, transferCmdPool);

		VkImageMemoryBarrier releaseBarrier = {};
		releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		releaseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		releaseBarrier.srcQueueFamilyIndex = devicePtr->transferQueue.family; //this might be the key to help sync 
		releaseBarrier.dstQueueFamilyIndex = devicePtr->graphicsQueue.family;
		releaseBarrier.image = this->mImage;
		releaseBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		releaseBarrier.subresourceRange.baseMipLevel = 0;
		releaseBarrier.subresourceRange.levelCount = mipLevels;
		releaseBarrier.subresourceRange.baseArrayLayer = 0;
		releaseBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.

		VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &transferCmd;

		VkSemaphoreCreateInfo transferSemaphoreCI = {};
		transferSemaphoreCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreateSemaphore(devicePtr->logical, &transferSemaphoreCI, nullptr, &mTransferCompleteSemaphore));

		submitInfo.pSignalSemaphores = &mTransferCompleteSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		
		VkFence transferFence;
		VkFenceCreateInfo transferFenceCI = {};
		transferFenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VK_CHECK_RESULT(vkCreateFence(devicePtr->logical, &transferFenceCI, nullptr, &transferFence));

		VK_CHECK_RESULT(vkQueueSubmit(devicePtr->transferQueue.handle, 1, &submitInfo, transferFence));

		VK_CHECK_RESULT(vkWaitForFences(devicePtr->logical, 1, &transferFence, VK_TRUE, UINT64_MAX));

		vkDestroyFence(devicePtr->logical, transferFence, nullptr);
		vkFreeCommandBuffers(devicePtr->logical, transferCmdPool, 1, &transferCmd);

		//end of creating mipmaps.;

		vkDestroyCommandPool(devicePtr->logical, transferCmdPool, nullptr);
		stagingBuffer.Destroy();
		stbi_image_free(pixels);

		this->cLogicalDevice = devicePtr->logical;
	}
}