#pragma once
#include "IModel.h"

class TextureManager
{
	public:
		TextureManager() = default;
		TextureManager( vk::GraphicsContextInfo contextInfo );
		~TextureManager() = default;

		VkDescriptorImageInfo GetTextureDescriptorInfo( const char* fileName );
		VkDescriptorImageInfo GetTextureDescriptorInfo( uint32_t index );

		void BindTextureToModelPrimitive( const std::string& fileName, Primitive& primitive );
		void FinishTextureLayoutTransition();

	private:
		bool AddTexture( vk::GraphicsContextInfo& graphicsContextInfo, const std::string& fileName );

		vk::GraphicsContextInfo graphicsContextInfo = {};

		struct TextureInfo
		{
			std::shared_ptr<vk::Texture> handle;
			uint32_t index = 0;
		};

		std::unordered_map<std::string, TextureInfo> m_textures;
		std::vector<TextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
};