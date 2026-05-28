#include "vkTexture.h"
#include "vkUtil.h"
#include "vkInit.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vk
{

	VkImageView Texture::CreateImageView( VkDevice l_device, const VkImage& textureImage, VkFormat format, VkImageViewType type )
	{

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = textureImage;
		viewInfo.viewType = type;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		viewInfo.components = vk::init::ComponentMappingSwizzleIdentity();

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
		createInfo.maxLod = VK_LOD_CLAMP_NONE;
		createInfo.mipLodBias = 0.f; //optional...

		VkSampler nTextureSampler;
		VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

		return nTextureSampler;
	}

	void Texture::RecordTransferAndReleaseOperations( const vk::Device* devicePtr, const vk::Buffer& stagingBuffer,
		std::mutex& submissionMutex )
	{
		VkSubmitInfo submitInfo = {};

		VkFence submissionFence = vk::init::CreateFence(devicePtr->GetDevice(), false);

		VkCommandPool transferCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::TRANSFER).family);
		VkCommandBuffer transferCmd = vk::util::beginSingleTimeCommand(devicePtr->GetDevice(), transferCmdPool);

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
		}

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

			vkCmdCopyBufferToImage(transferCmd, stagingBuffer.GetHandle(), m_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, regions.data());
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
			releaseBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; //since we just wrote to the image.
			releaseBarrier.dstAccessMask = 0;

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

		{
			submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCmd;

			std::lock_guard lock(submissionMutex);
			VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
				submissionFence));
		}

		VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));


		vkDestroyFence(devicePtr->GetDevice(), submissionFence, nullptr);

		vkFreeCommandBuffers(devicePtr->GetDevice(), transferCmdPool, 1, &transferCmd);
		vkDestroyCommandPool(devicePtr->GetDevice(), transferCmdPool, nullptr);
	}

	Texture::Texture( const vk::Device* devicePtr, const std::vector<vk::TextureCreateInfo>& createInfos,
		std::mutex& transferMutex )
	{
		assert(devicePtr != nullptr);

		m_imageCount = createInfos.size();
		c_device = devicePtr->GetDevice();

		const uint64_t num_channels = 4;
		int textureWidth, textureHeight, textureChannels;
		VkDeviceSize imageSize = 0;

		vk::Buffer stagingBuffer;

		stbi_uc* pixels_uc = nullptr;
		float* pixels_f = nullptr;

		//Might want to make command pool a member variable.
		const std::string& filePath = createInfos[0].name;

		if (filePath.empty())
		{
			std::cerr << "specified filePath is empty \n";
			throw std::runtime_error("vk::Texture::Create() FAILED");
		}

		if (createInfos[0].format == VK_FORMAT_R8G8B8A8_UNORM ||
			createInfos[0].format == VK_FORMAT_R8G8B8A8_SRGB)
		{
			pixels_uc = stbi_load(filePath.c_str(),
				&textureWidth, &textureHeight, &textureChannels, 4);

			if (pixels_uc == nullptr)
			{
				std::cerr << "could not load in specified texture " + filePath << std::endl;
				throw std::runtime_error("vk::Texture::Create() FAILED");
			}
		}
		else if (createInfos[0].format == VK_FORMAT_R16G16B16A16_SFLOAT ||
			createInfos[0].format == VK_FORMAT_R32G32B32A32_SFLOAT)
		{
			pixels_f = stbi_loadf(filePath.c_str(),
				&textureWidth, &textureHeight, &textureChannels, 4);

			if (pixels_f == nullptr)
			{
				std::cerr << "could not load in specified texture " + filePath << std::endl;
				throw std::runtime_error("vk::Texture::Create() FAILED");
			}
		}
		else
		{
			std::cerr << "specified format unsupported by vk::Texture::Create() \n";
			std::cerr << "could not create texture: " << createInfos[0].name << std::endl;
			throw std::runtime_error("vk::Texture::Create() FAILED");
		}

		m_width = static_cast<uint32_t>(textureWidth);
		m_height = static_cast<uint32_t>(textureHeight);

		if (pixels_uc != nullptr)
		{
			imageSize = static_cast<uint64_t>(textureWidth) *
				static_cast<uint64_t>(textureHeight) * num_channels;

			stagingBuffer = vk::Buffer(devicePtr,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			static_cast<size_t>(imageSize), pixels_uc);
		}
		else if (pixels_f != nullptr)
		{
			imageSize = static_cast<uint64_t>(textureWidth) *
				static_cast<uint64_t>(textureHeight) * num_channels * sizeof(float);

			stagingBuffer = vk::Buffer(devicePtr,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			static_cast<size_t>(imageSize), pixels_f);
		}

		VkImageCreateInfo textureImageCI = vk::init::ImageCreateInfo();
		textureImageCI.imageType = VK_IMAGE_TYPE_2D;
		textureImageCI.format = createInfos[0].format;
		textureImageCI.extent = {m_width, m_height, 1};
		textureImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		textureImageCI.mipLevels = 1;
		textureImageCI.arrayLayers = 1;
		textureImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		textureImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		textureImageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		m_image = vk::init::CreateImage(devicePtr, textureImageCI, m_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		RecordTransferAndReleaseOperations(devicePtr, stagingBuffer, transferMutex);

		if (pixels_f)
		{
			stbi_image_free(pixels_f);
		}

		if (pixels_uc)
		{
			stbi_image_free(pixels_uc);
		}

		m_imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(), m_image, createInfos[0].format, VK_IMAGE_VIEW_TYPE_2D);
		m_sampler   = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice(), 1 );
		m_descriptor.imageView = m_imageView;
		m_descriptor.sampler = m_sampler;
		m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	Texture::Texture( Texture&& other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_image = other.m_image;
			this->m_memory = other.m_memory;
			this->m_imageView = other.m_imageView;
			this->m_sampler = other.m_sampler;
			this->m_width = other.m_width;
			this->m_height = other.m_height;
			this->m_imageCount = other.m_imageCount;
			this->m_imageLayerSize = other.m_imageLayerSize;
			this->m_descriptor = other.m_descriptor;

			//because Destroy() hinges on c_device being valid, we'll just invalidate
			//c_device on the original resource.
			other.c_device = VK_NULL_HANDLE;
		}
	}

	Texture& Texture::operator=( Texture &&other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_image = other.m_image;
			this->m_memory = other.m_memory;
			this->m_imageView = other.m_imageView;
			this->m_sampler = other.m_sampler;
			this->m_width = other.m_width;
			this->m_height = other.m_height;
			this->m_imageCount = other.m_imageCount;
			this->m_imageLayerSize = other.m_imageLayerSize;
			this->m_descriptor = other.m_descriptor;

			//because Destroy() hinges on c_device being valid, we'll just invalidate
			//c_device on the original resource.
			other.c_device = VK_NULL_HANDLE;
		}

		return *this;
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