#pragma once

#include "vkTexture.h"
#include "vkContextBase.h"
#include <vector>
#include <mutex>
#include "Object.h"

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			~TextureManager() = default;

			void Destroy(const VkDevice l_device);

			void Init(ContextBase* context);

			int GetTextureIndexByName(const char* fileName) const;
			
			const Texture& GetTextureObject(size_t index) const;

			void BindTextureToObject(const std::string& fileName, Object& obj);

			const std::vector<vk::Texture>& Textures() const;

		private:

			bool AddTexture(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName);

			vk::GraphicsContextInfo graphicsContextInfo;

			std::vector<vk::Texture> mTextures;
	};

}