#pragma once

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			~TextureManager() = default;

			void Init(GraphicsContextInfo contextInfo);

			VkDescriptorImageInfo GetTextureDescriptorInfo(const char* fileName);
			VkDescriptorImageInfo GetTextureDescriptorInfo(uint32_t index);

			void BindTextureToObject(const std::string& fileName, Object& obj);

		private:
			bool AddTexture(GraphicsContextInfo& graphicsContextInfo, const std::string& fileName);

			vk::GraphicsContextInfo graphicsContextInfo = {};

			struct TextureInfo 
			{
				std::unique_ptr<vk::Texture> handle;
				uint32_t index = 0;
			};

			std::unordered_map<std::string, TextureInfo> mTextures;
	};

}