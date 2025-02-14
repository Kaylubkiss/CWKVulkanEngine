#pragma once


#include <vulkan/vulkan.h>
#include "vkTexture.h"
#include <vector>

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager(const VkDevice l_device);
			TextureManager() = default;
			~TextureManager() = default;

			void Destroy(const VkDevice l_device);

			void Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue	gfxQueue, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName);
			
			const Texture& GetTexture(size_t index) const;

			int GetTextureIndexByName(const char* fileName) const;

			void WriteDescriptorSets(const VkDevice l_device, const VkDescriptorBufferInfo* pUniformDescriptorBuffers, size_t uniformDescriptorCount);

		private:
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

			std::vector<vk::Texture> mTextures;
	};

}