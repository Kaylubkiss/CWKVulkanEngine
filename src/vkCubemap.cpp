#include "vkCubemap.h"
#include "vkUtility.h"
#include "vkResource.h"
#include "vkBuffer.h"
#include "vkInit.h"
#include <stb_image.h>

namespace vk {

	Cubemap::Cubemap(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscSetLayout, const std::string* fileNames) 
	{
		//Might want to make command pool a member variable.
		VkCommandPool cmdPool = vk::init::CommandPool(l_device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		vk::Buffer stagingBuffer;
		
		int textureWidth, textureHeight, textureChannels;
		uint32_t mipLevels = 1;

		stbi_uc* pixels;
		uint64_t imageLayerSize;
		VkDeviceSize imageSize;
		void* data;

		for (int i = 0; i < NUM_CUBEMAP_IMAGES; ++i) 
		{
			pixels = stbi_load((TEXTURE_PATH + fileNames[i]).c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

			if (pixels == NULL)
			{
				throw std::runtime_error("failed to load texture image!");
			}

			if (i == 0) //all image sizes should be the same.
			{
				imageLayerSize = textureWidth * textureHeight * textureChannels;
				imageSize = imageLayerSize * NUM_CUBEMAP_IMAGES;

				stagingBuffer = vk::Buffer(p_device, l_device, static_cast<size_t>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, pixels);

				vkMapMemory(l_device, stagingBuffer.memory, 0, imageSize, 0, &data);
			}

			memcpy(reinterpret_cast<void*>((reinterpret_cast<uint64_t*>(data) + imageLayerSize * i)), reinterpret_cast<void*>(pixels), static_cast<size_t>(imageLayerSize));

			stbi_image_free(pixels);
		}

		vkUnmapMemory(l_device, stagingBuffer.memory);

		this->mTextureImage = vk::rsc::CreateImage(p_device, l_device, textureWidth, textureHeight, 1, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->mTextureMemory, NUM_CUBEMAP_IMAGES);


		//creating mipmaps
		vk::util::TransitionImageLayout(l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

		vk::util::copyBufferToImage(l_device, cmdPool, stagingBuffer.handle, gfxQueue, this->mTextureImage, (uint32_t)(textureWidth), (uint32_t)(textureHeight));

		vk::util::TransitionImageLayout(l_device, cmdPool, gfxQueue, this->mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);


		vkDestroyBuffer(l_device, stagingBuffer.handle, nullptr);
		vkFreeMemory(l_device, stagingBuffer.memory, nullptr);

		vkDestroyCommandPool(l_device, cmdPool, nullptr);

		//end of creating mipmaps.
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = this->mTextureImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = mipLevels;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = NUM_CUBEMAP_IMAGES;

		VK_CHECK_RESULT(vkCreateImageView(l_device, &viewInfo, nullptr, &this->mTextureImageView));

		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = static_cast<float>(mipLevels);
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerCreateInfo.maxAnisotropy = 1.0f;

		samplerCreateInfo.anisotropyEnable = VK_TRUE;

		VkPhysicalDeviceProperties pdp = { };
		vkGetPhysicalDeviceProperties(p_device, &pdp);

		samplerCreateInfo.maxAnisotropy = pdp.limits.maxSamplerAnisotropy;

		VK_CHECK_RESULT(vkCreateSampler(l_device, &samplerCreateInfo, nullptr, &this->mTextureSampler));

		this->mName = "fileName";

		this->mDescriptorSet = vk::init::DescriptorSet(l_device, dscPool, dscSetLayout);

	}

}