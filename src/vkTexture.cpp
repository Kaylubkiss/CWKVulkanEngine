#include "vkTexture.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "vkBuffer.h"
#include <stb_image.h>

namespace vk {


	VkImageView Texture::CreateTextureView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels)
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

	VkSampler Texture::CreateTextureSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels)
	{
		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		createInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties pdp = { };
		vkGetPhysicalDeviceProperties(p_device, &pdp);

		createInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy / 2.f;

		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS; //value is ignored.

		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minLod = 0.f;
		createInfo.maxLod = static_cast<float>(mipLevels);
		createInfo.mipLodBias = 0.f; //optional...

		VkSampler nTextureSampler;
		VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

		return nTextureSampler;
	}

	Texture::Texture(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName)
	{

		assert(graphicsContextInfo != nullptr);
		//Might want to make command pool a member variable.

		std::cout << "constructing a texture\n";
	
		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = fileName == "" ? nullptr : stbi_load((TEXTURE_PATH + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		if (!pixels)
		{
			std::cerr << "could not load in specified texture " + std::string(TEXTURE_PATH + fileName) << std::endl;
			//TODO: generate checker-board texture for objects.
			return;

		}

		uint64_t bytePerPixel = 4;
		VkDeviceSize imageSize = (uint64_t)textureWidth * (uint64_t)textureHeight * bytePerPixel; //4 bytes per pixel.

		/*uint32_t mipLevels = vk::util::CalculateMipLevels(textureWidth, textureHeight); -- commented out because I don't understand it yet. */ 
		uint32_t mipLevels = 1;

		vk::Buffer stagingBuffer = vk::Buffer(graphicsContextInfo->physicalDevice, graphicsContextInfo->logicalDevice, 
			static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)pixels);

		this->mTextureImage = vk::init::CreateImage(graphicsContextInfo->physicalDevice, 
			graphicsContextInfo->logicalDevice, textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->mTextureMemory);

		VkCommandPool cmdPool = vk::init::CommandPool(graphicsContextInfo->logicalDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		vk::util::TransitionImageLayout(graphicsContextInfo->logicalDevice, cmdPool, graphicsContextInfo->graphicsQueue.handle, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		/*vk::util::GenerateMipMaps(p_device, l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);*/
		
		vk::util::copyBufferToImage(graphicsContextInfo->logicalDevice, cmdPool,
			stagingBuffer.handle, 
			graphicsContextInfo->graphicsQueue.handle,
			this->mTextureImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight)); //copy contents of the image (stored in buffer) into the image.

		//transition the image layout to shader read only for sampling in the shader.
		vk::util::TransitionImageLayout(graphicsContextInfo->logicalDevice, cmdPool, graphicsContextInfo->graphicsQueue.handle, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

		//end of creating mipmaps.;

		vkDestroyCommandPool(graphicsContextInfo->logicalDevice, cmdPool, nullptr);
		stagingBuffer.Destroy();
		stbi_image_free(pixels);

		this->mTextureImageView = CreateTextureView(graphicsContextInfo->logicalDevice, this->mTextureImage, mipLevels);

		this->mTextureSampler = CreateTextureSampler(graphicsContextInfo->physicalDevice, graphicsContextInfo->logicalDevice, mipLevels);

		this->descriptor.sampler = this->mTextureSampler;
		this->descriptor.imageView = this->mTextureImageView;
		this->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		this->mName = fileName;		
		
		std::cout << "finished constructing a texture\n";
	}


}