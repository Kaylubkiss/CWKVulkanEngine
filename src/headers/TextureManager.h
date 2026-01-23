#pragma once

namespace vk 
{
	class TextureManager 
	{
		public:
			TextureManager() = default;
			TextureManager( GraphicsContextInfo contextInfo );
			~TextureManager() = default;

			VkDescriptorImageInfo GetTextureDescriptorInfo(const char* fileName);
			VkDescriptorImageInfo GetTextureDescriptorInfo(uint32_t index);

			void BindTextureToObject(const std::string& fileName, Object& obj);
			void FinishTextureLayoutTransition();

		private:
			bool AddTexture(GraphicsContextInfo& graphicsContextInfo, const std::string& fileName);

			vk::GraphicsContextInfo graphicsContextInfo = {};

			struct TextureInfo 
			{
				std::shared_ptr<vk::Texture> handle;
				uint32_t index = 0;
			};

			std::unordered_map<std::string, TextureInfo> m_textures;
			std::vector<TextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
	};

}