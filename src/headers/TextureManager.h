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
		size_t GetSize();

		void BindTextureToModelPrimitive( const std::string& fileName, Primitive& primitive );
		void UploadTextureDataToGPU();
		uint32_t AddTexture( const std::string& fileName ); //returns the index of the texture
	private:
		std::mutex m_textureMutex;
		vk::GraphicsContextInfo m_graphicsContextInfo = {};
		struct TextureInfo
		{
			std::shared_ptr<vk::Texture> handle;
			uint32_t index = 0;
		};
		std::unordered_map<std::string, TextureInfo> m_textures;
		std::vector<TextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
};