#include "TextureManager.h"

void TextureManager::Init( const std::weak_ptr<vk::GraphicsContextInfo>& contextInfo )
{
	assert(contextInfo.expired() == false);

	std::shared_ptr<vk::GraphicsContextInfo> sharedContextInfo = contextInfo.lock();

	assert(sharedContextInfo->devicePtr != nullptr);
	assert(sharedContextInfo->descriptorBufferCreateInfoPtr != nullptr);

	m_textureSamplerDescriptor.Create(*sharedContextInfo->descriptorBufferCreateInfoPtr);

	m_graphicsContextInfo = sharedContextInfo;

	m_graphicsCommandPool = vk::init::CommandPool(m_graphicsContextInfo.lock()->devicePtr->GetDevice(),
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_graphicsContextInfo.lock()->devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family);

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vk::init::CommandBufferAllocateInfo();
	cmdBufferAllocateInfo.commandPool = m_graphicsCommandPool;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	VK_CHECK_RESULT(vkAllocateCommandBuffers(m_graphicsContextInfo.lock()->devicePtr->GetDevice(), &cmdBufferAllocateInfo,
		m_commandBuffers.data()));
}

void TextureManager::Destroy()
{
	if (m_graphicsContextInfo.expired() == false)
	{
		auto sharedContextInfo = m_graphicsContextInfo.lock();

		vkFreeCommandBuffers(sharedContextInfo->devicePtr->GetDevice(), m_graphicsCommandPool,
			static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

		vkDestroyCommandPool(sharedContextInfo->devicePtr->GetDevice(), m_graphicsCommandPool, nullptr);
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

	m_textureSamplerDescriptor.Destroy();
}

uint32_t TextureManager::AddTexture( const std::string& fileName, uint32_t bindingIndex, uint32_t& layoutIndex )
{
	if (fileName.empty())
	{
		return 0;
	}

	//quick check.
	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		if (m_textures.contains(fileName) == true)
		{
			if (layoutIndex == 0)
			{
				layoutIndex = m_textures[fileName].index;
			}
			return m_textures[fileName].index;
		}
	}

	auto sharedGraphicsContextInfo = m_graphicsContextInfo.lock();
	//TODO: generate checker-board texture for objects if texture loading failed.
	std::shared_ptr<vk::Texture> newTexture = std::make_shared<vk::Texture>();

	newTexture->Create(sharedGraphicsContextInfo->devicePtr, fileName, m_transferMutex);

	if (m_textureSamplerDescriptor.GetBindingOffsets().size() <= bindingIndex)
	{
		std::cerr << "binding index " << bindingIndex << " is greater than the binding count supported in the shaders.\n";
		throw std::runtime_error("AddTexture() Failed!\n");
	}

	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			layoutIndex = m_textures[fileName].index;
			return m_textures[fileName].index;
		}
		m_textures[fileName].handle = std::move(newTexture);
		m_textures[fileName].index = static_cast<uint32_t>(m_textures.size()); //first texture in m_textures will be blank.
	}

	{
		std::lock_guard<std::mutex> lock(m_pendingTexturesMutex);
		PendingTextureInfo pendingInfo = {};
		pendingInfo.texture_to_process = m_textures[fileName].handle;
		pendingInfo.bindingIndex = bindingIndex;
		//because layoutIndex 0 is the null/default texture, we assume that because a texture
		//was successfully allocated, the layout's base index starts where the newly allocated
		//texture does in the buffer.
		if (layoutIndex == 0)
		{
			layoutIndex = m_textures[fileName].index;
		}
		pendingInfo.layoutIndex = layoutIndex;
		m_pendingTextures.push_back(pendingInfo); //sync with this later.
	}

	std::cout << "texture loaded... " << fileName << " loaded.\n";
	return m_textures[fileName].index;
}

void TextureManager::FillDescriptorBuffer(const std::vector<PendingTextureInfo>& texturesToProcess) const
{
	for (auto& t : texturesToProcess)
	{
		auto sharedGraphicsContextInfo = m_graphicsContextInfo.lock();

		if (sharedGraphicsContextInfo == nullptr)
		{
			return;
		}

		vk::Texture* curr_texture = t.texture_to_process.get();

		/*
		auto& UI = sharedGraphicsContextInfo->contextUIPtr;
		if (UI)
		{
			UI->AddImage(*curr_texture);
		}
		*/

		VkDescriptorImageInfo textureDescriptor = curr_texture->GetDescriptor();

		auto& bindingOffsets = m_textureSamplerDescriptor.GetBindingOffsets();

		VkDeviceSize bindingOffset = bindingOffsets[t.bindingIndex];

		VkDescriptorGetInfoEXT imageDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		imageDescriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescriptorInfo.data.pCombinedImageSampler = &textureDescriptor;

		VkDeviceSize textureBindingSize =
			m_textureSamplerDescriptor.GetLayoutSize();

		VkDeviceSize imageSamplerSize =
			sharedGraphicsContextInfo->devicePtr->GetDescriptorBufferProperties().combinedImageSamplerDescriptorSize;

		char* imageBindingDescriptorPtr =
			static_cast<char*>(m_textureSamplerDescriptor.GetBuffer().GetMappedMemory());

		g_vkGetDescriptorEXT(sharedGraphicsContextInfo->devicePtr->GetDevice(), &imageDescriptorInfo,
			imageSamplerSize,
			imageBindingDescriptorPtr + t.layoutIndex * textureBindingSize + bindingOffset);
	}
}

bool TextureManager::UploadTextureDataToGPU( uint32_t currentFrame, const VkSemaphore signalSemaphore )
{
	std::vector<PendingTextureInfo> texturesToProcess;

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

	auto graphicsContextInfo = m_graphicsContextInfo.lock();
	if (graphicsContextInfo == nullptr)
	{
		return false;
	}

	vk::Device* devicePtr = graphicsContextInfo->devicePtr;

	VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[currentFrame], &cmdBufferBeginInfo));

	for (auto& t : texturesToProcess)
	{
		vk::Texture* curr_texture = t.texture_to_process.get();

		VkImageMemoryBarrier acquireBarrier = {};
		acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		acquireBarrier.srcQueueFamilyIndex = devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).family;
		acquireBarrier.dstQueueFamilyIndex = devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family;
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

	VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).handle, 1, &submitInfo, VK_NULL_HANDLE));

	// NOTE: this is incomplete until the submission is synced on the GPU
	FillDescriptorBuffer(texturesToProcess);

	return true;
}

size_t TextureManager::GetSize()
{
	std::lock_guard<std::mutex> lock(m_textureMutex);
	return m_textures.size();
}

const vk::DescriptorBuffer& TextureManager::GetTextureSamplerDescriptor() const
{
	return m_textureSamplerDescriptor;
}

void TextureManager::BindTextureToModelPrimitive( const std::string& fileName, uint32_t bindingIndex, uint32_t& layoutIndex )
{
	AddTexture(fileName, bindingIndex, layoutIndex);
}