#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace vk 
{

	struct Texture
	{
		std::string mName = "";
		VkImage mTextureImage = VK_NULL_HANDLE;
		VkDeviceMemory mTextureMemory = VK_NULL_HANDLE;
		VkImageView mTextureImageView = VK_NULL_HANDLE;
		VkSampler mTextureSampler = VK_NULL_HANDLE;

		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

		Texture() = default;

		void Destroy(const VkDevice l_device) {

			vkDestroySampler(l_device, mTextureSampler, nullptr);
			vkDestroyImageView(l_device, mTextureImageView, nullptr);
			vkDestroyImage(l_device, mTextureImage, nullptr);
			vkFreeMemory(l_device, mTextureMemory, nullptr);
		}
	};

}
