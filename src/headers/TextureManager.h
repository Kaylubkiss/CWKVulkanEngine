#pragma once


class TextureManager
{
	public:
		TextureManager() = default;
		~TextureManager() = default;

		void Init( std::shared_ptr<vk::GraphicsContextInfo>& contextInfo );
		void Destroy();

		VkDescriptorImageInfo GetTextureDescriptorInfo( const char* fileName );
		VkDescriptorImageInfo GetTextureDescriptorInfo( uint32_t index );
		size_t GetSize();

		void BindTextureToModelPrimitive( const std::string& fileName, Primitive& primitive );

		//returns whether or not a command was recorded.
		bool UploadTextureDataToGPU( uint32_t currentFrame, VkSemaphore textureUploadSemaphore );
		uint32_t AddTexture( const std::string& fileName ); //returns the index of the texture
	private:
		std::mutex m_textureMutex;
		std::mutex m_transferMutex;
		std::mutex m_pendingTexturesMutex;
		VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
		std::array<VkCommandBuffer, gMaxFramesInFlight> m_commandBuffers;

		std::shared_ptr<vk::GraphicsContextInfo> s_graphicsContextInfo = {};
		struct TextureInfo
		{
			std::shared_ptr<vk::Texture> handle;
			uint32_t index = 0;
		};
		std::vector<TextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
		std::unordered_map<std::string, TextureInfo> m_textures;
};