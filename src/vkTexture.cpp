#include "vkTexture.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "vkBuffer.h"
#include <stb_image.h>
#include "ApplicationGlobal.h"

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
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		createInfo.minLod = 0.f;
		createInfo.maxLod = static_cast<float>(mipLevels);
		createInfo.mipLodBias = 0.f; //optional...

		VkSampler nTextureSampler;
		VK_CHECK_RESULT(vkCreateSampler(l_device, &createInfo, nullptr, &nTextureSampler));

		return nTextureSampler;
	}
		
	Texture::Texture(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const std::string& fileName)
	{
		//Might want to make command pool a member variable.
	
		uint32_t arrayLayerCount = 1;
		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = fileName == "" ? nullptr : stbi_load((TEXTURE_PATH + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		//TODO: this code segment does not working!!!
		if (!pixels)
		{
			textureWidth = 128;
			textureHeight = textureWidth;
			int divisions = textureWidth / 4;

			pixels = new stbi_uc[textureWidth * textureHeight];

			//generate default texture
			for (size_t i = 0; i < textureHeight; ++i)
			{
				size_t pixelCol = i * textureWidth;

				for (size_t i = 0; i < textureWidth; ++i) 
				{
					if (i % divisions == 0) 
					{
						pixels[pixelCol + i] = 255;
					}
					else {
						pixels[pixelCol + i] = 0;
					}
				}

			}

		}

		VkDeviceSize imageSize = (uint64_t)textureWidth * (uint64_t)textureHeight * 4;

		uint32_t mipLevels = vk::util::CalculateMipLevels(textureWidth, textureHeight);

		vk::Buffer stagingBuffer = vk::Buffer(p_device, l_device, static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, (void*)pixels);

		stbi_image_free(pixels);

		this->mTextureImage = vk::init::CreateImage(p_device, l_device, textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->mTextureMemory, arrayLayerCount);

		VkCommandPool cmdPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		//creating mipmaps
		vk::util::TransitionImageLayout(l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		vk::util::copyBufferToImage(l_device, cmdPool, stagingBuffer.handle, gfxQueue, this->mTextureImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight));

		vk::util::GenerateMipMaps(p_device, l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);

		stagingBuffer.Destroy();

		vkDestroyCommandPool(l_device, cmdPool, nullptr);

		//end of creating mipmaps.

		this->mTextureImageView = CreateTextureView(l_device, this->mTextureImage, mipLevels);

		this->mTextureSampler = CreateTextureSampler(p_device, l_device, mipLevels);

		this->descriptor.sampler = this->mTextureSampler;
		this->descriptor.imageView = this->mTextureImageView;
		this->descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		this->mName = fileName;
	}


}