#pragma once


#include <vulkan/vulkan.h>
#include "vkTexture.h"
#include <vector>
#include "vkContextBase.h"
#include <mutex>

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			~TextureManager() = default;

			void Destroy(const VkDevice l_device);

			void Init(ContextBase* context);

			void Add(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName);

			void Add(const Texture& nTexture);
			
			int GetTextureIndexByName(const char* fileName) const;
			
			const Texture& GetTextureObject(size_t index) const;

			void BindTextureToObject(const std::string& fileName, Object& obj);

		private:

			vk::GraphicsContextInfo graphicsContextInfo;

			std::vector<vk::Texture> mTextures;
	};

}