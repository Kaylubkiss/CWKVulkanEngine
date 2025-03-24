#pragma once
#include "vkTexture.h"


namespace vk 
{
	class Cubemap : public Texture 
	{
		private:
			static constexpr int NUM_CUBEMAP_IMAGES = 6;
			VkFormat mImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

		public:
			Cubemap() = default;
			Cubemap(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorPool dscPool, const VkDescriptorSetLayout dscSetLayout, const std::string* fileNames);

			virtual void Destroy(const VkDevice l_device) override
			{
				vkDestroySampler(l_device, mTextureSampler, nullptr);
				vkDestroyImageView(l_device, mTextureImageView, nullptr);
				vkDestroyImage(l_device, mTextureImage, nullptr);
				vkFreeMemory(l_device, mTextureMemory, nullptr);
			}
	};


}