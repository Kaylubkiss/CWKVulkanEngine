#pragma once

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			~TextureManager() = default;

			void Init(GraphicsContextInfo contextInfo);

			VkDescriptorImageInfo GetTextureDescriptorSet(const char* fileName);

			void BindTextureToObject(const std::string& fileName, Object& obj);

		private:

			bool AddTexture(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName);

			vk::GraphicsContextInfo graphicsContextInfo;

			std::unordered_map<std::string, std::pair<std::unique_ptr<vk::Texture>, std::shared_ptr<VkDescriptorSet>>> mTextures;
	};

}