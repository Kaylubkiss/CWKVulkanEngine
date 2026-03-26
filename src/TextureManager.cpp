#include "TextureManager.h"

void TextureManager::Init( vk::GraphicsContextInfo* contextInfo )
{

	assert(contextInfo != nullptr);
	assert(contextInfo->devicePtr != nullptr);
	assert(contextInfo->descriptorBufferCreateInfoPtr != nullptr);

	m_textureSamplerDescriptor.Create(*contextInfo->descriptorBufferCreateInfoPtr);

	size_t freeListSize = 10000 + 1; //TODO: EQUIVALENT TO OBJECT_COUNT in deferred context

	m_descriptorFreeList.reserve(freeListSize);
	for (size_t i = 0; i < freeListSize; ++i)
	{
		m_descriptorFreeList.push_back(i);
	}

	m_graphicsContextInfoPtr = contextInfo;

	m_graphicsCommandPool = vk::init::CommandPool(m_graphicsContextInfoPtr->devicePtr->GetDevice(),
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_graphicsContextInfoPtr->devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family);

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vk::init::CommandBufferAllocateInfo();
	cmdBufferAllocateInfo.commandPool = m_graphicsCommandPool;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	VK_CHECK_RESULT(vkAllocateCommandBuffers(m_graphicsContextInfoPtr->devicePtr->GetDevice(), &cmdBufferAllocateInfo,
		m_commandBuffers.data()));
}

void TextureManager::Destroy()
{

	if (m_graphicsContextInfoPtr && m_graphicsContextInfoPtr->devicePtr != nullptr)
	{
		VkDevice contextDevice = m_graphicsContextInfoPtr->devicePtr->GetDevice();

		vkFreeCommandBuffers(contextDevice, m_graphicsCommandPool,
				static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

		vkDestroyCommandPool(contextDevice, m_graphicsCommandPool, nullptr);
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

bool TextureManager::AddTexture(const std::string& fileName, size_t bindingIndex)
{
	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			return false;
		}
	}

	//TODO: generate checker-board texture for objects if texture loading failed.
	std::shared_ptr<vk::Texture> newTexture = std::make_shared<vk::Texture>();

	newTexture->Create(m_graphicsContextInfoPtr->devicePtr, fileName, m_transferMutex);

	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			return false;
		}
		m_textures[fileName].handle = std::move(newTexture);
		m_textures[fileName].index = static_cast<uint32_t>(m_textures.size()); //first texture in m_textures will be blank.
	}

	std::cout << "texture loaded... " << fileName << " loaded.\n";

	return true;
}

uint32_t TextureManager::AddTextures( const std::vector<std::string>& fileNames )
{
	if (fileNames.empty())
	{
		return 0;
	}

	if (m_textureSamplerDescriptor.GetBindingOffsets().size() < fileNames.size())
	{
		std::cerr << "The requested bindings from fileNames is larger than binding count supported by the shaders!\n";
		throw std::runtime_error("AddTextures() Failed!\n");
	}

	uint32_t descriptorSetLayoutIndex = 0;
	{
		std::lock_guard lock(m_descriptorFreeListMutex);
		if (m_descriptorFreeList.back() == 0)
		{
			std::cerr << "no more space to allocate descriptor with!\n";
			throw std::runtime_error("TextureManager::AddTextures() Failed!\n");
		}

		descriptorSetLayoutIndex = static_cast<uint32_t>(m_descriptorFreeList.back());
		m_descriptorFreeList.pop_back();
	}

	for (size_t i = 0; i < fileNames.size(); ++i)
	{
		PendingTextureInfo pendingInfo = {};
		//because layoutIndex 0 is the null/default texture, we assume that because a texture
		//was successfully allocated, the layout's base index starts where the newly allocated
		//texture does in the buffer.
		pendingInfo.layoutIndex = descriptorSetLayoutIndex;
		pendingInfo.bindingIndex = static_cast<uint32_t>(i);
		pendingInfo.needsGPUTransfer = AddTexture(fileNames[i], i);
		pendingInfo.texture_to_process = m_textures[fileNames[i]].handle;

		{
			std::lock_guard<std::mutex> lock(m_pendingTexturesMutex);
			m_pendingTextures.push_back(pendingInfo); //sync with this later.
		}
	}

	return descriptorSetLayoutIndex;
}

void TextureManager::FillDescriptorBuffer(const std::vector<PendingTextureInfo>& texturesToProcess) const
{
	for (auto& t : texturesToProcess)
	{
		vk::Texture* curr_texture = t.texture_to_process.get();

		VkDescriptorImageInfo textureDescriptor = curr_texture->GetDescriptor();

		auto& bindingOffsets = m_textureSamplerDescriptor.GetBindingOffsets();

		VkDeviceSize bindingOffset = bindingOffsets[t.bindingIndex];

		VkDescriptorGetInfoEXT imageDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
		imageDescriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageDescriptorInfo.data.pCombinedImageSampler = &textureDescriptor;

		VkDeviceSize textureBindingSize =
			m_textureSamplerDescriptor.GetLayoutSize();

		vk::Device* devicePtr = m_graphicsContextInfoPtr->devicePtr;

		VkDeviceSize imageSamplerSize =
			devicePtr->GetDescriptorBufferProperties().combinedImageSamplerDescriptorSize;

		char* imageBindingDescriptorPtr =
			static_cast<char*>(m_textureSamplerDescriptor.GetBuffer().GetMappedMemory());

		g_vkGetDescriptorEXT(devicePtr->GetDevice(), &imageDescriptorInfo,
			imageSamplerSize,
			imageBindingDescriptorPtr + t.layoutIndex * textureBindingSize + bindingOffset);
	}
}

bool TextureManager::UploadTextureDataToGPU( uint32_t currentFrame, const VkSemaphore signalSemaphore )
{
	std::vector<PendingTextureInfo> texturesToProcess;
	{
		std::lock_guard lock(m_pendingTexturesMutex);
		if (m_pendingTextures.empty() == false)
		{
			texturesToProcess.swap(m_pendingTextures);
		}
		else
		{
			return false;
		}
	}

	vk::Device* devicePtr = m_graphicsContextInfoPtr->devicePtr;

	VkCommandBufferBeginInfo cmdBufferBeginInfo = vk::init::CommandBufferBeginInfo();
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[currentFrame], &cmdBufferBeginInfo));

	for (auto& t : texturesToProcess)
	{
		if (t.needsGPUTransfer)
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
	std::lock_guard lock(m_textureMutex);
	return m_textures.size();
}

const vk::DescriptorBuffer& TextureManager::GetTextureSamplerDescriptor() const
{
	return m_textureSamplerDescriptor;
}