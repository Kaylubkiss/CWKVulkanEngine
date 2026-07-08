#include "vkTextureManager.h"
#include "vkCubemap.h"
#include "vkInit.h"

namespace vk
{
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

		//dont need to allocate a separate command pool if the graphics and transfer queue are the same.
		//all work will just be submitted to the graphics queue.
		if (devicePtr->GetQueue(DeviceQueue::TRANSFER).family != devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family)
		{
			m_transferCommandPool = vk::init::CommandPool(m_devicePtr->GetDevice(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, m_devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).family);

			cmdBufferAllocateInfo.commandPool = m_transferCommandPool;

			VK_CHECK_RESULT(vkAllocateCommandBuffers(m_devicePtr->GetDevice(), &cmdBufferAllocateInfo,
				m_transferCommandBuffers.data()));
		}

	}

	void TextureManager::Destroy()
	{
		if (m_devicePtr != nullptr)
		{
			VkDevice contextDevice = m_devicePtr->GetDevice();

			vkFreeCommandBuffers(contextDevice, m_graphicsCommandPool,
					static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

			vkFreeCommandBuffers(contextDevice, m_transferCommandPool,
				static_cast<uint32_t>(m_transferCommandBuffers.size()), m_transferCommandBuffers.data());

			vkDestroyCommandPool(contextDevice, m_graphicsCommandPool, nullptr);
			vkDestroyCommandPool(contextDevice, m_transferCommandPool, nullptr);
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

	AddTextureResult TextureManager::AddTexture( const vk::TextureCreateInfo& createInfo )
	{
		{
			std::lock_guard<std::mutex> lock(m_textureMutex);
			//maybe a thread beat us to the punch, in the case that two threads call on the same texture
			if (m_textures.contains(createInfo.fileName) == true)
			{
				return { m_textures[createInfo.fileName].handle, false };
			}
		}

		//TODO: generate checker-board texture for objects if texture loading failed
		{
			std::shared_ptr<vk::Texture> newTexture = std::make_shared<vk::Texture>(m_devicePtr, createInfo);

			std::lock_guard<std::mutex> lock(m_textureMutex);
			//maybe a thread beat us to the punch, in the case that two threads call on the same texture
			if (m_textures.contains(createInfo.fileName) == true)
			{
				return { m_textures[createInfo.fileName].handle, false };
			}

			m_textures.insert({ createInfo.fileName, 
				{ newTexture, static_cast<uint32_t>(m_textures.size()) } 
			});

			return { newTexture, true };
		}

		std::cout << "texture loaded... " << createInfo.fileName << " loaded.\n";

		return {};
	}

	uint32_t TextureManager::AddTextures( std::vector<vk::TextureCreateInfo>& createInfos, TextureType type )
	{
		(void)(type); //so the user can specify a panoramic texture, cubemap texture

		if (createInfos.empty())
		{
			return 0;
		}

		uint32_t layoutIndex = m_descriptorManagerPtr->GetLayoutIndex( DescriptorCategory::eMaterial );
		const size_t textureCount = createInfos.size();
		std::vector<PendingTextureInfo> pendingInfos(textureCount);

		vk::TextureCreateInfo individualCI = {};
		individualCI.imageUsage = createInfos.back().imageUsage;

		for (size_t i = 0; i < textureCount; ++i)
		{
			if (createInfos[i].fileName.empty() == false)
			{
				individualCI.fileName = createInfos[i].fileName;
				individualCI.format = createInfos[i].format;
				individualCI.layerCount = createInfos[i].layerCount;
				individualCI.mipLevels = createInfos[i].mipLevels;

				//because layoutIndex 0 is the null/default texture, we assume that because a texture
				//was successfully allocated, the layout's base index starts where the newly allocated
				//texture does in the buffer.
				pendingInfos[i].layoutIndex = layoutIndex;
				pendingInfos[i].bindingIndex = static_cast<uint32_t>(i);
				pendingInfos[i].totalBindingCount = static_cast<uint32_t>(textureCount);

				AddTextureResult result = AddTexture(individualCI);

				pendingInfos[i].texture_to_process = result.texture;
				pendingInfos[i].needsGPUTransfer = result.needsTransfer;
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

	bool TextureManager::UploadTextureDataToGPU( uint32_t currentFrame, TextureUploadSemaphores& semaphores )
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

		uint32_t transferQueueFamily = m_devicePtr->GetQueue(DeviceQueue::TRANSFER).family;
		uint32_t graphicsQueueFamily = m_devicePtr->GetQueue(DeviceQueue::GRAPHICS).family;

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[currentFrame], &cmdBufferBeginInfo));

		if (transferQueueFamily != graphicsQueueFamily)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(m_transferCommandBuffers[currentFrame], &cmdBufferBeginInfo));
		}

		for (auto& t : texturesToProcess)
		{
			if (t.needsGPUTransfer)
			{
				vk::Texture* curr_texture = t.texture_to_process.get();

				if (transferQueueFamily != graphicsQueueFamily)
				{
					curr_texture->RecordStagingCopy( m_transferCommandBuffers[currentFrame]);
					curr_texture->RecordRelease( m_transferCommandBuffers[currentFrame],
					transferQueueFamily, graphicsQueueFamily);
				}
				else
				{
					curr_texture->RecordStagingCopy( m_commandBuffers[currentFrame]);
				}


				VkImageMemoryBarrier acquireBarrier = {};
				acquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				acquireBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				acquireBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				if (transferQueueFamily != graphicsQueueFamily)
				{
					acquireBarrier.srcQueueFamilyIndex = m_devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).family;
					acquireBarrier.dstQueueFamilyIndex = m_devicePtr->GetQueue(vk::DeviceQueue::GRAPHICS).family;
				}
				else
				{
					acquireBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					acquireBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				}

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
		if (transferQueueFamily != graphicsQueueFamily)
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(m_transferCommandBuffers[currentFrame]));

			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_transferCommandBuffers[currentFrame];
			submitInfo.pSignalSemaphores = &semaphores.transferSubmitSemaphore;
			submitInfo.signalSemaphoreCount = 1;

			VK_CHECK_RESULT(vkQueueSubmit(m_devicePtr->GetQueue(vk::DeviceQueue::TRANSFER).handle,
			1, &submitInfo, VK_NULL_HANDLE));
		}

		submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];

		std::array<VkPipelineStageFlags, 1> transferWaitStage = {VK_PIPELINE_STAGE_TRANSFER_BIT};
		if (transferQueueFamily != graphicsQueueFamily)
		{
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitDstStageMask = transferWaitStage.data();
			submitInfo.pWaitSemaphores = &semaphores.transferSubmitSemaphore;
		}

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &semaphores.graphicsSubmitSemaphore;

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
				if  (texturesToProcess[i + binding].texture_to_process != nullptr)
				{
					imageDescriptors[0][binding] = texturesToProcess[i + binding].texture_to_process->GetDescriptor();
				}
			}

			m_descriptorManagerPtr->WriteDescriptors(DescriptorCategory::eMaterial, layoutIndex, imageDescriptors);

			i += texturesToProcess[i].totalBindingCount;
		}



		return true;
	}
}
