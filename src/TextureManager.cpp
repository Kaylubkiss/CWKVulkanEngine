#include "TextureManager.h"
#include "vkCubemap.h"
#include "vkInit.h"

void TextureManager::Init( vk::Device* devicePtr, DescriptorManager* descriptorManagerPtr )
{
	m_devicePtr = devicePtr;
	m_descriptorManagerPtr = descriptorManagerPtr;

	m_graphicsCommandPool = vk::init::CommandPool(m_devicePtr->GetDevice(),
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family);

	VkCommandBufferAllocateInfo cmdBufferAllocateInfo = vk::init::CommandBufferAllocateInfo();
	cmdBufferAllocateInfo.commandPool = m_graphicsCommandPool;
	cmdBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
	VK_CHECK_RESULT(vkAllocateCommandBuffers(m_devicePtr->GetDevice(), &cmdBufferAllocateInfo,
		m_commandBuffers.data()));
}

void TextureManager::Destroy()
{
	if (m_devicePtr != nullptr)
	{
		VkDevice contextDevice = m_devicePtr->GetDevice();

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
}

bool TextureManager::AddTexture( const vk::TextureCreateInfo& createInfo )
{
	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(createInfo.fileNames.back()) == true)
		{
			return false;
		}
	}

	//TODO: generate checker-board texture for objects if texture loading failed
	std::shared_ptr<vk::Texture> newTexture = std::make_shared<vk::Texture>(m_devicePtr, createInfo);

	{
		std::lock_guard<std::mutex> lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(createInfo.fileNames.back()) == true)
		{
			return false;
		}
		m_textures[createInfo.fileNames.back()].handle = std::move(newTexture);
		//first texture in m_textures will be blank.
		m_textures[createInfo.fileNames.back()].index = static_cast<uint32_t>(m_textures.size());
	}

	std::cout << "texture loaded... " << createInfo.fileNames.back() << " loaded.\n";

	return true;
}

bool TextureManager::AddCubeMapTexture( const vk::TextureCreateInfo& createInfo )
{

	const std::string& fileName = createInfo.fileNames[0];

	{
		std::lock_guard lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			return false;
		}
	}

	std::shared_ptr<vk::Texture> newTexture = std::make_shared<vk::Cubemap>(m_devicePtr, createInfo);

	{
		std::lock_guard lock(m_textureMutex);
		//maybe a thread beat us to the punch, in the case that two threads call on the same texture
		if (m_textures.contains(fileName) == true)
		{
			return false;
		}
		m_textures[fileName].handle = std::move(newTexture);
		//first texture in m_textures will be blank.
		m_textures[fileName].index = static_cast<uint32_t>(m_textures.size());
	}

	std::cout << "CubeMap texture loaded... " << fileName << " (starting-at) loaded.\n";

	return true;
}

uint32_t TextureManager::AddTextures( std::vector<vk::TextureCreateInfo>& createInfos, TextureType type )
{
	if (createInfos.empty())
	{
		return 0;
	}

	for ( auto& CI : createInfos )
	{
		CI.pTransferMutex = &m_transferMutex;
	}

	uint32_t layoutIndex = m_descriptorManagerPtr->GetLayoutIndex(DescriptorCategory::eMaterial);

	std::vector<PendingTextureInfo> pendingInfos;

	if (type == TextureType::CUBEMAP)
	{
		pendingInfos.resize(1);

		pendingInfos.front().bindingIndex = 0;
		pendingInfos.front().layoutIndex = layoutIndex;
		pendingInfos.front().totalBindingCount = 1;
		pendingInfos.front().needsGPUTransfer = AddCubeMapTexture(createInfos.back());
		pendingInfos.front().texture_to_process = m_textures[createInfos.back().fileNames.back()].handle;
		pendingInfos.front().type = type;
	}
	else
	{
		vk::TextureCreateInfo individualCI = {};
		individualCI.pTransferMutex = &m_transferMutex;
		individualCI.imageUsage = createInfos.back().imageUsage;

		size_t textureCount = createInfos.size();

		pendingInfos.resize(textureCount);

		for (size_t i = 0; i < textureCount; ++i)
		{
			individualCI.fileNames = { createInfos[i].fileNames.back() };
			individualCI.format = createInfos[i].format;
			individualCI.layerCount = createInfos[i].layerCount;
			individualCI.mipLevels = createInfos[i].mipLevels;

			//because layoutIndex 0 is the null/default texture, we assume that because a texture
			//was successfully allocated, the layout's base index starts where the newly allocated
			//texture does in the buffer.
			pendingInfos[i].layoutIndex = layoutIndex;
			pendingInfos[i].bindingIndex = static_cast<uint32_t>(i);
			pendingInfos[i].totalBindingCount = static_cast<uint32_t>(textureCount);
			pendingInfos[i].needsGPUTransfer = AddTexture(individualCI);
			pendingInfos[i].texture_to_process = m_textures[individualCI.fileNames.back()].handle;
		}
	}

	{
		std::lock_guard lock(m_pendingTexturesMutex);
		for (size_t i = 0; i < pendingInfos.size(); ++i)
		{
			m_pendingTextures.push_back(pendingInfos[i]); //sync with this later.
		}
	}

	return layoutIndex;
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
			acquireBarrier.srcQueueFamilyIndex = m_devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).family;
			acquireBarrier.dstQueueFamilyIndex = m_devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family;
			acquireBarrier.image = curr_texture->GetImage();
			acquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			acquireBarrier.subresourceRange.baseMipLevel = 0;
			acquireBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			acquireBarrier.subresourceRange.baseArrayLayer = 0;
			acquireBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

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

	VK_CHECK_RESULT(vkQueueSubmit(m_devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).handle,
		1, &submitInfo, VK_NULL_HANDLE));

	vk::imageBuffers2D imageDescriptors;
	imageDescriptors.resize(1);

	size_t i = 0;
	while (i < texturesToProcess.size())
	{
		imageDescriptors[0].resize(texturesToProcess[i].totalBindingCount);

		uint32_t layoutIndex = texturesToProcess[i].layoutIndex;

		for (size_t binding = 0; binding < texturesToProcess[i].totalBindingCount; ++binding)
		{
			imageDescriptors[0][binding] = texturesToProcess[i + binding].texture_to_process->GetDescriptor();
		}

		m_descriptorManagerPtr->WriteDescriptors(DescriptorCategory::eMaterial, layoutIndex, imageDescriptors);

		i += texturesToProcess[i].totalBindingCount;
	}



	return true;
}
