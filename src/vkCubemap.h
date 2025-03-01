#pragma once
#include "vkTexture.h"


namespace vk 
{
	class Cubemap : public Texture 
	{
		private:
			static constexpr int NUM_CUBEMAP_IMAGES = 6;

		public:
			virtual void Destroy(const VkDevice l_device) override
			{
				vkDestroySampler(l_device, mTextureSampler, nullptr);
				vkDestroyImageView(l_device, mTextureImageView, nullptr);
				vkDestroyImage(l_device, mTextureImage, nullptr);
				vkFreeMemory(l_device, mTextureMemory, nullptr);
			}


	};


}