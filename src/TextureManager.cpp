#include "TextureManager.h"
#include "vkUtility.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "vkBuffer.h"
#include "vkResource.h"
#include "vkInit.h"

namespace vk 
{
	const std::string texturePath{ "External/textures/" };

	static VkImageView CreateTextureView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels)
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

	static VkSampler CreateTextureSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels)
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

	TextureManager::TextureManager(const VkDevice l_device) 
	{
		this->descriptorPool = vk::init::DescriptorPool(l_device);
	}

	//could very well be inefficient. Look into a way to individually write descriptor sets. Shouldn't be hard. Later.
	void TextureManager::WriteDescriptorSets(const VkDevice l_device, const VkDescriptorBufferInfo* pUniformDescriptorBuffers, size_t uniformDescriptorCount) 
	{
		VkDescriptorImageInfo imageInfo = {};

		for (size_t i = 0; i < this->mTextures.size(); ++i)
		{
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = this->mTextures[i].mTextureImageView;
			imageInfo.sampler = this->mTextures[i].mTextureSampler;

			//writing the uniform transforms.
			VkWriteDescriptorSet descriptorWrite[2] = {};
			descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[0].dstSet = this->mTextures[i].mDescriptorSet;
			descriptorWrite[0].dstBinding = 0;
			descriptorWrite[0].dstArrayElement = 0;
			descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite[0].descriptorCount = uniformDescriptorCount; //how many buffers
			descriptorWrite[0].pBufferInfo = pUniformDescriptorBuffers;
			descriptorWrite[0].pImageInfo = nullptr; // Optional
			descriptorWrite[0].pTexelBufferView = nullptr; // Optional

			//writing the texture sampler.
			descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[1].dstSet = this->mTextures[i].mDescriptorSet;
			descriptorWrite[1].dstBinding = 1;
			descriptorWrite[1].dstArrayElement = 0;
			descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite[1].descriptorCount = 1; //how many images
			descriptorWrite[1].pImageInfo = &imageInfo;
			descriptorWrite[1].pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(l_device, 2, descriptorWrite, 0, nullptr);
		}

	}

	void TextureManager::Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue	gfxQueue, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName)
	{
		//TODO: ensure that the same texture isn't allocated twice.

		//Might want to make command pool a member variable.
		VkCommandPool cmdPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

		this->mTextures.push_back(Texture());
		Texture& newTexture = this->mTextures.back();

		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixels = stbi_load((texturePath + fileName).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

		VkDeviceSize imageSize = textureWidth * textureHeight * 4;

		if (pixels == NULL)
		{
			throw std::runtime_error("failed to load texture image!");
		}

		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

		vk::Buffer stagingBuffer = vk::Buffer(p_device, l_device, static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pixels);

		stbi_image_free(pixels);

		newTexture.mTextureImage = vk::rsc::CreateImage(p_device, l_device, textureWidth, textureHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, newTexture.mTextureMemory, 1);

		vk::util::TransitionImageLayout(l_device, cmdPool, gfxQueue, newTexture.mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		vk::util::copyBufferToImage(l_device, cmdPool, stagingBuffer.handle, gfxQueue, newTexture.mTextureImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight));

		vk::util::GenerateMipMaps(p_device, l_device, cmdPool, gfxQueue, newTexture.mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, (uint32_t)textureWidth, (uint32_t)textureHeight, mipLevels);

		vkDestroyBuffer(l_device, stagingBuffer.handle, nullptr);
		vkFreeMemory(l_device, stagingBuffer.memory, nullptr);

		vkDestroyCommandPool(l_device, cmdPool, nullptr);

		newTexture.mTextureImageView = CreateTextureView(l_device, newTexture.mTextureImage, mipLevels);

		newTexture.mTextureSampler = CreateTextureSampler(p_device, l_device, mipLevels);

		this->mTextures.back().mName = fileName;

		this->mTextures.back().mDescriptorSet = vk::init::DescriptorSet(l_device, this->descriptorPool, dscSetLayout);
	}

	void TextureManager::Destroy(const VkDevice l_device) 
	{
		for (size_t i = 0; i < mTextures.size(); ++i) 
		{
			mTextures[i].Destroy(l_device);
		}

		vkDestroyDescriptorPool(l_device, this->descriptorPool, nullptr);

	}

	const Texture& TextureManager::GetTexture(size_t index) const
	{

		if (index < 0 || index >= mTextures.size()) 
		{
			throw std::runtime_error("could not find specified texture!\n");
		}

		return mTextures[index];
	}

	int TextureManager::GetTextureIndexByName(const char* fileName) const 
	{
		for (size_t i = 0; i < mTextures.size(); ++i)
		{
			if (strcmp(fileName, mTextures[i].mName.c_str()) == 0)
			{
				return i;
			}
		}

		throw std::runtime_error("could not find specified texture!\n");

		return this->mTextures.size() - 1;

	}


}