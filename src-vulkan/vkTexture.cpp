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

	void Texture::RecordTransferAndReleaseOperations( VkCommandBuffer cmdBuffer, uint32_t srcQueueFamily, uint32_t dstQueueFamily )
	{
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

			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&barrier); //asking the gpu to reconfigure the old image layout to the new layout.
		}

		//copy buffer into image.
		{
			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.bufferOffset = 0;
			region.imageExtent =
			{
				m_width,
				m_height,
				1
			};

		vkCmdCopyBufferToImage(cmdBuffer, m_stagingBuffer.GetHandle(), m_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		//release transfer queue to graphics queue
		{
			VkImageMemoryBarrier releaseBarrier = {};
			releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			releaseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			releaseBarrier.srcQueueFamilyIndex = srcQueueFamily;
			releaseBarrier.dstQueueFamilyIndex = dstQueueFamily;
			releaseBarrier.image = m_image;
			releaseBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			releaseBarrier.subresourceRange.baseMipLevel = 0;
			releaseBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			releaseBarrier.subresourceRange.baseArrayLayer = 0;
			releaseBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			releaseBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; //since we just wrote to the image.
			releaseBarrier.dstAccessMask = 0;

			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0,
				nullptr, 0, nullptr, 1,
				&releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.
		}
	}

	void Texture::CreateBlankTexture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo )
	{
		VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
		imageCI.format = createInfo.format;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.arrayLayers = createInfo.layerCount;
		imageCI.extent.width = createInfo.width;
		imageCI.extent.height = createInfo.height;
		imageCI.extent.depth = 1;
		imageCI.mipLevels = createInfo.mipLevels;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.usage = createInfo.imageUsage;
		imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCI.flags = createInfo.flags;

		m_image = vk::util::CreateImage( devicePtr, imageCI, m_memory,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		m_width = createInfo.width;
		m_height = createInfo.height;
	}

	void Texture::CreateFromFileName( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo )
	{
		const uint64_t num_channels = 4;
		int textureWidth, textureHeight, textureChannels;
		VkDeviceSize imageSize = 0;

		if (createInfo.format != VK_FORMAT_R8G8B8A8_UNORM &&
			createInfo.format != VK_FORMAT_R8G8B8A8_SRGB &&
			createInfo.format != VK_FORMAT_R8G8B8A8_SNORM)
		{
			std::cerr << "specified format unsupported by vk::Texture::Create() \n";
			std::cerr << "could not create texture: " << createInfo.fileName << std::endl;
			throw std::runtime_error("vk::Texture::Create() FAILED");
		}

		const std::string& filePath = createInfo.fileName;

		stbi_uc* pixelData = stbi_load(filePath.c_str(),
			&textureWidth, &textureHeight, &textureChannels, 4);

		if (pixelData == nullptr)
		{
			std::cerr << "could not load in specified texture " + filePath << std::endl;
			throw std::runtime_error("vk::Texture::Create() FAILED");
		}

		m_width = static_cast<uint32_t>(textureWidth);
		m_height = static_cast<uint32_t>(textureHeight);

		imageSize = static_cast<uint64_t>(textureWidth) *
			static_cast<uint64_t>(textureHeight) * num_channels;

		m_stagingBuffer = vk::Buffer(devicePtr,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			static_cast<size_t>(imageSize));

		m_imageLayerSize = imageSize;
		m_imageLayerSize = vk::util::AlignedSize(m_imageLayerSize, devicePtr->GetProperties().limits.optimalBufferCopyOffsetAlignment);

		m_stagingBuffer.Map();

		void* stagingBufferData = m_stagingBuffer.GetMappedMemory();

		memcpy(static_cast<stbi_uc*>(stagingBufferData), pixelData, m_imageLayerSize);

		m_stagingBuffer.Flush();

		m_stagingBuffer.UnMap();

		stbi_image_free(pixelData);

		VkImageCreateInfo textureImageCI = vk::init::ImageCreateInfo();
		textureImageCI.imageType = VK_IMAGE_TYPE_2D;
		textureImageCI.format = createInfo.format;
		textureImageCI.extent = {m_width, m_height, 1};
		textureImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		textureImageCI.mipLevels = createInfo.mipLevels;
		textureImageCI.arrayLayers = createInfo.layerCount;
		textureImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		textureImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		textureImageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		m_image = vk::util::CreateImage(devicePtr, textureImageCI, m_memory,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//RecordTransferAndReleaseOperations( devicePtr, createInfo.stagingBuffer, createInfo.pTransferMutex );

	}

	Texture::Texture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo )
	{
		assert(devicePtr != nullptr);

		c_device = devicePtr->GetDevice();

		if (createInfo.fileName.empty() == false)
		{
			CreateFromFileName( devicePtr, createInfo );

			m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			CreateBlankTexture( devicePtr, createInfo );

			m_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}


		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;

		if (createInfo.layerCount == 6)
		{
			viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		}

		m_descriptor.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(), m_image,
			createInfo.format, viewType);

		m_descriptor.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(),
			devicePtr->GetDevice(), createInfo.mipLevels );

	}

	Texture::Texture( Texture&& other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_image = other.m_image;
			this->m_memory = other.m_memory;
			this->m_width = other.m_width;
			this->m_height = other.m_height;
			this->m_imageLayerSize = other.m_imageLayerSize;
			this->m_descriptor = other.m_descriptor;

			other.c_device = VK_NULL_HANDLE;
		}
	}

	Texture& Texture::operator=( Texture &&other ) noexcept
	{
		if (this != &other)
		{
			std::swap(this->c_device, other.c_device);
			std::swap(this->m_image, other.m_image);
			std::swap(this->m_memory, other.m_memory);
			std::swap(this->m_width, other.m_width);
			std::swap(this->m_height, other.m_height);
			std::swap(this->m_imageLayerSize, other.m_imageLayerSize);
			std::swap(this->m_descriptor, other.m_descriptor);
		}

		return *this;
	}

	Texture::~Texture()
	{
		if (c_device != VK_NULL_HANDLE)
		{
			if (m_descriptor.sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(c_device, m_descriptor.sampler, nullptr);
				m_descriptor.sampler = VK_NULL_HANDLE;
			}

			if (m_descriptor.imageView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(c_device, m_descriptor.imageView, nullptr);
				m_descriptor.imageView = VK_NULL_HANDLE;
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

	VkExtent2D Texture::GetImageExtent() const
	{
		return {m_width, m_height };
	}

	void Texture::SetImageLayout( VkImageLayout layout )
	{
		m_descriptor.imageLayout = layout;
	}



}