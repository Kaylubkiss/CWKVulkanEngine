#pragma once

#include <vulkan/vulkan.h>
#include <string>

#define TEXTURE_PATH "External/textures/"

namespace vk 
{

	struct Texture
	{
		//member variables
		std::string mName = "";
		VkImage mTextureImage = VK_NULL_HANDLE;
		VkDeviceMemory mTextureMemory = VK_NULL_HANDLE;
		VkImageView mTextureImageView = VK_NULL_HANDLE;
		VkSampler mTextureSampler = VK_NULL_HANDLE; //different mip-levels might need different samplers

		//this is unused for now!!!
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkDescriptorImageInfo descriptor = {};

		static VkImageView CreateTextureView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels);
		static VkSampler CreateTextureSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels);

		Texture() = default;
		~Texture() = default;
		Texture(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName);

		void Destroy(const VkDevice l_device) {

			vkDestroySampler(l_device, mTextureSampler, nullptr);
			vkDestroyImageView(l_device, mTextureImageView, nullptr);
			vkDestroyImage(l_device, mTextureImage, nullptr);
			vkFreeMemory(l_device, mTextureMemory, nullptr);
		}
	};

}
