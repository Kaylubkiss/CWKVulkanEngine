#pragma once


#include <vulkan/vulkan.h>
#include "vkTexture.h"
#include <vector>
#include "vkGraphicsSystem.h"

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			~TextureManager() = default;

			void Destroy(const VkDevice l_device);

			void Init(const VkDevice l_device);

			void Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName);

			void Add(const Texture& nTexture);
			
			int GetTextureIndexByName(const char* fileName) const;
			
			const Texture& GetTextureObject(size_t index) const;

			void UpdateDescriptorSets(const VkDevice l_device, const VkDescriptorBufferInfo* pUniformDescriptorBuffers, size_t uniformDescriptorCount);

			void BindTextureToObject(const std::string& fileName, GraphicsSystem& graphicsSystem, Object& obj);
		private:
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

			std::vector<vk::Texture> mTextures;
	};

}