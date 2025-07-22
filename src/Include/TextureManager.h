#pragma once


#include <vulkan/vulkan.h>
#include "vkTexture.h"
#include <vector>
#include "vkContextBase.h"

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			~TextureManager() = default;

			void Destroy(const VkDevice l_device);

			void Init(const ContextBase* graphicsContext);

			void Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName);

			void Add(const Texture& nTexture);
			
			int GetTextureIndexByName(const char* fileName) const;
			
			const Texture& GetTextureObject(size_t index) const;

			void UpdateDescriptorSets(const VkDevice l_device, const VkDescriptorBufferInfo* pUniformDescriptorBuffers, size_t uniformDescriptorCount);

			void BindTextureToObject(const std::string& fileName, ContextBase& graphicsSystem, Object& obj);

		private:

			std::shared_ptr<VkDescriptorPool> contextDescriptorPool;

			std::vector<vk::Texture> mTextures;
	};

}