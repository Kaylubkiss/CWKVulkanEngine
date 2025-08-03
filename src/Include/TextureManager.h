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

			void Init(ContextBase* context);

			void Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const std::string& fileName);

			void Add(const Texture& nTexture);
			
			int GetTextureIndexByName(const char* fileName) const;
			
			const Texture& GetTextureObject(size_t index) const;

			void BindTextureToObject(const std::string& fileName, Object& obj);

		private:

			ContextBase* graphicsContext = nullptr;
			std::vector<vk::Texture> mTextures;
	};

}