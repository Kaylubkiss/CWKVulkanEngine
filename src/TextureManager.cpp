#include "TextureManager.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"


TextureManager::TextureManager( vk::GraphicsContextInfo contextInfo )
{
	assert(contextInfo.devicePtr != nullptr);
	assert(contextInfo.contextTextureDescriptorPtr != nullptr);
	m_graphicsContextInfo = contextInfo;
}

uint32_t TextureManager::AddTexture( const std::string& fileName )
{
	//quick check.
	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		if (m_textures.contains(fileName) == true)
		{
			return m_textures[fileName].index;
		}
	}

	//TODO: generate checker-board texture for objects if texture loading failed.
	std::shared_ptr<vk::Texture> newTexture =
			std::make_shared<vk::Texture>(m_graphicsContextInfo.devicePtr, fileName);

	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			return m_textures[fileName].index;
		}
		m_textures[fileName].handle = std::move(newTexture);
		m_textures[fileName].index = m_textures.size()-1;

		m_pendingTextures.push_back(m_textures[fileName]); //sync with this later.

		return m_textures[fileName].index;
	}
}

void TextureManager::FinishTextureLayoutTransition()
{
	std::lock_guard<std::mutex> lock(m_textureMutex);
	if (!m_pendingTextures.empty())
	{
		vk::Device* devicePtr = m_graphicsContextInfo.devicePtr;

		VkCommandPool graphicsCmdPool =
			vk::init::CommandPool(devicePtr->logical, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
				devicePtr->graphicsQueue.family);

		VkCommandBuffer graphicsCmd = vk::beginSingleTimeCommand(devicePtr->logical, graphicsCmdPool);

		std::vector<VkSemaphore> waitSemaphores;
		for (auto& t : m_pendingTextures)
		{
			VkImageMemoryBarrier acquireBarrier = {};
			acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			acquireBarrier.srcQueueFamilyIndex = devicePtr->transferQueue.family;
			acquireBarrier.dstQueueFamilyIndex = devicePtr->graphicsQueue.family;

			vk::Texture* curr_texture = t.handle.get();
			acquireBarrier.image = curr_texture->mImage;
			acquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			acquireBarrier.subresourceRange.baseMipLevel = 0;
			acquireBarrier.subresourceRange.levelCount = 1;
			acquireBarrier.subresourceRange.baseArrayLayer = 0;
			acquireBarrier.subresourceRange.layerCount = 1;

			acquireBarrier.srcAccessMask = 0;
			acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				graphicsCmd,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0, 0, nullptr,
				0, nullptr,
				1, &acquireBarrier
			);

			waitSemaphores.push_back(curr_texture->mTransferCompleteSemaphore);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(graphicsCmd));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &graphicsCmd;

		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.waitSemaphoreCount = waitSemaphores.size();

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		submitInfo.pWaitDstStageMask = &waitStage;

		VK_CHECK_RESULT(vkQueueSubmit(devicePtr->graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE));
		VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->graphicsQueue.handle));

		vkFreeCommandBuffers(devicePtr->logical, graphicsCmdPool, 1, &graphicsCmd);
		vkDestroyCommandPool(devicePtr->logical, graphicsCmdPool, nullptr);

		for (auto& t : m_pendingTextures)
		{
			vk::Texture* curr_texture = t.handle.get();

			curr_texture->mImageView = vk::Texture::CreateImageView(devicePtr->logical, curr_texture->mImage, 1);
			curr_texture->mSampler = vk::Texture::CreateSampler(devicePtr->physical, devicePtr->logical, 1);

			curr_texture->descriptor = {
				curr_texture->mSampler,
				curr_texture->mImageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			auto& UI = m_graphicsContextInfo.contextUIPtr;
			if (UI)
			{
				UI->AddImage(*curr_texture);
			}

			VkDeviceSize textureBindingSize =
				m_graphicsContextInfo.contextTextureDescriptorPtr->size;

			VkDeviceSize combinedImageSamplerSize =
				m_graphicsContextInfo.devicePtr->DescriptorBufferProperties().combinedImageSamplerDescriptorSize;

			VkDeviceSize bindingOffset =
				m_graphicsContextInfo.contextTextureDescriptorPtr->binding_offsets.front();

			VkDescriptorGetInfoEXT imageDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
			imageDescriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageDescriptorInfo.data.pCombinedImageSampler = &curr_texture->descriptor;

			char* imageBindingDescriptorPtr =
				(char*)(m_graphicsContextInfo.contextTextureDescriptorPtr->buffers.front().GetMappedMemory());

			g_vkGetDescriptorEXT(m_graphicsContextInfo.devicePtr->logical, &imageDescriptorInfo,
				combinedImageSamplerSize,
				imageBindingDescriptorPtr + t.index * textureBindingSize + bindingOffset);
		}

		m_pendingTextures.clear();
	}
}

VkDescriptorImageInfo TextureManager::GetTextureDescriptorInfo(const char* fileName)
{
	std::lock_guard<std::mutex> lock(m_textureMutex);
	if (m_textures.count(fileName))
	{
		return m_textures[fileName].handle->descriptor;
	}

	std::cerr << "could not find specified texture!\n";
	std::cerr << "GetTextureDescriptorInfo() Failed.\n";
	return {};

}

VkDescriptorImageInfo TextureManager::GetTextureDescriptorInfo(uint32_t index)
{
	std::lock_guard<std::mutex> lock(m_textureMutex);
	for (auto& t : m_textures)
	{
		TextureInfo& texture = t.second;
		if (texture.index == 0)
		{
			return texture.handle.get()->descriptor;
		}
	}

	std::cerr << "could not find specified texture!\n";
	std::cerr << "GetTextureDescriptorInfo() Failed.\n";

	return {};
}

size_t TextureManager::GetSize()
{
	std::lock_guard<std::mutex> lock(m_textureMutex);
	return m_textures.size();
}

void TextureManager::BindTextureToModelPrimitive( const std::string& fileName, Primitive& primitive )
{
	if (fileName != "")
	{
		primitive.textureIndex = AddTexture(fileName);
	}

}