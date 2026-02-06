#include "TextureManager.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"


void TextureManager::Init( std::shared_ptr<vk::GraphicsContextInfo>& contextInfo )
{
	assert(contextInfo->devicePtr != nullptr);
	assert(contextInfo->contextTextureDescriptorPtr != nullptr);

	s_graphicsContextInfo = contextInfo;

	m_graphicsCommandPool = vk::init::CommandPool(s_graphicsContextInfo->devicePtr->logical,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, s_graphicsContextInfo->devicePtr->graphicsQueue.family);

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vk::init::CommandBufferAllocateInfo();
	cmdBufferAllocateInfo.commandPool = m_graphicsCommandPool;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	VK_CHECK_RESULT(vkAllocateCommandBuffers(s_graphicsContextInfo->devicePtr->logical, &cmdBufferAllocateInfo,
		m_commandBuffers.data()));

}

void TextureManager::Destroy()
{
	if (s_graphicsContextInfo != nullptr)
	{
		vkFreeCommandBuffers(s_graphicsContextInfo->devicePtr->logical, m_graphicsCommandPool,
			static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

		vkDestroyCommandPool(s_graphicsContextInfo->devicePtr->logical, m_graphicsCommandPool, nullptr);
	}


	//NOTE: object/asset manager should call terminate on all threads before this code
	//so these shouldn't be locked up.
	{
		std::lock_guard<std::mutex> lock(m_pendingTexturesMutex);
		m_pendingTextures.clear();
	}

	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		m_textures.clear(); //this should call ~Texture()
	}

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
			std::make_shared<vk::Texture>(s_graphicsContextInfo->devicePtr, fileName, m_transferMutex);

	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			return m_textures[fileName].index;
		}
		m_textures[fileName].handle = std::move(newTexture);
		m_textures[fileName].index = static_cast<uint32_t>(m_textures.size()); //first texture in m_textures will be blank.
	}

	{
		std::lock_guard<std::mutex> lock(m_pendingTexturesMutex);
		m_pendingTextures.push_back(m_textures[fileName]); //sync with this later.
	}

	std::cout << "texture loaded... " << fileName << " loaded.\n";
	return m_textures[fileName].index;
}

bool TextureManager::UploadTextureDataToGPU( uint32_t currentFrame, const VkSemaphore signalSemaphore )
{
	std::vector<TextureInfo> texturesToProcess;

	{
		std::lock_guard<std::mutex> lock(m_pendingTexturesMutex);
		if (m_pendingTextures.empty() == false)
		{
			texturesToProcess.resize(m_pendingTextures.size());
			texturesToProcess = m_pendingTextures;
			m_pendingTextures.clear();
		}
		else
		{
			return false;
		}
	}

	vk::Device* devicePtr = s_graphicsContextInfo->devicePtr;

	VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[currentFrame], &cmdBufferBeginInfo));

	for (auto& t : texturesToProcess)
	{
		vk::Texture* curr_texture = t.handle.get();

		VkImageMemoryBarrier acquireBarrier = {};
		acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		acquireBarrier.srcQueueFamilyIndex = devicePtr->transferQueue.family;
		acquireBarrier.dstQueueFamilyIndex = devicePtr->graphicsQueue.family;
		acquireBarrier.image = curr_texture->GetImage();
		acquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		acquireBarrier.subresourceRange.baseMipLevel = 0;
		acquireBarrier.subresourceRange.levelCount = 1;
		acquireBarrier.subresourceRange.baseArrayLayer = 0;
		acquireBarrier.subresourceRange.layerCount = 1;

		acquireBarrier.srcAccessMask = 0;
		acquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			m_commandBuffers[currentFrame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr,
			0, nullptr,
			1, &acquireBarrier
		);
	}

	VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[currentFrame]));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];
	submitInfo.pSignalSemaphores = &signalSemaphore;
	submitInfo.signalSemaphoreCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(devicePtr->graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE));

	//fill descriptors -- NOTE: this is incomplete until the submission is synced on the GPU
	for (auto& t : texturesToProcess)
	{
		vk::Texture* curr_texture = t.handle.get();
		VkDescriptorImageInfo textureDescriptor = curr_texture->GetDescriptor();

		auto& UI = s_graphicsContextInfo->contextUIPtr;
		if (UI)
		{
			UI->AddImage(*curr_texture);
		}

		VkDeviceSize textureBindingSize =
			s_graphicsContextInfo->contextTextureDescriptorPtr->size;

		VkDeviceSize combinedImageSamplerSize =
			s_graphicsContextInfo->devicePtr->DescriptorBufferProperties().combinedImageSamplerDescriptorSize;

		VkDeviceSize bindingOffset =
			s_graphicsContextInfo->contextTextureDescriptorPtr->binding_offsets.front();

		VkDescriptorGetInfoEXT imageDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		imageDescriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescriptorInfo.data.pCombinedImageSampler = &textureDescriptor;

		char* imageBindingDescriptorPtr =
			(char*)(s_graphicsContextInfo->contextTextureDescriptorPtr->buffers.front().GetMappedMemory());

		g_vkGetDescriptorEXT(s_graphicsContextInfo->devicePtr->logical, &imageDescriptorInfo,
			combinedImageSamplerSize,
			imageBindingDescriptorPtr + t.index * textureBindingSize + bindingOffset);
	}

	return true;
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