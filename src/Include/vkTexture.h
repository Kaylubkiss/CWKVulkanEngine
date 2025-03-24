#pragma once

#include <string>
#include <vulkan/vulkan.h>

#define TEXTURE_PATH "External/textures/"

namespace vk 
{

	class Texture
	{
		//member variables
		public:
			std::string mName = "";
			VkImage mTextureImage = VK_NULL_HANDLE;
			VkDeviceMemory mTextureMemory = VK_NULL_HANDLE;
			VkImageView mTextureImageView = VK_NULL_HANDLE;
			VkSampler mTextureSampler = VK_NULL_HANDLE;

			VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

		//static methods for public use.
		public:
			static VkImageView CreateTextureView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels);
			static VkSampler CreateTextureSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels);

		//member methods
		public:
			Texture() = default;
			Texture(const Texture& other);
			Texture(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName);


			virtual void Destroy(const VkDevice l_device) {

				vkDestroySampler(l_device, mTextureSampler, nullptr);
				vkDestroyImageView(l_device, mTextureImageView, nullptr);
				vkDestroyImage(l_device, mTextureImage, nullptr);
				vkFreeMemory(l_device, mTextureMemory, nullptr);
			}
	};

}
