#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace vk 
{

	struct Texture
	{
		std::string mName;
		VkImage mTextureImage;
		VkDeviceMemory mTextureMemory;
		VkImageView mTextureImageView;
		VkSampler mTextureSampler;

		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

		Texture() : mTextureImage(), mTextureMemory(), mTextureImageView(), mTextureSampler(), mDescriptorSet(VK_NULL_HANDLE) {};

		void Destroy(const VkDevice l_device) {

			vkDestroySampler(l_device, mTextureSampler, nullptr);
			vkDestroyImageView(l_device, mTextureImageView, nullptr);
			vkDestroyImage(l_device, mTextureImage, nullptr);
			vkFreeMemory(l_device, mTextureMemory, nullptr);
		}
	};

}
