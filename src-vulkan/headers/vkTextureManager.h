#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP

#include "vkTexture.h"

//NOTE: DOES NOT NEED TO BE A POWER OF TWO
enum class TextureType : uint16_t
{
	NONE = 0x0,
	CUBEMAP = 0x1,
	PANORAMIC = 0x2,
};

namespace vk
{
	struct TextureInfo
	{
		std::shared_ptr<vk::Texture> handle;
		uint32_t index = 0;
	};

	struct PendingTextureInfo
	{
		std::shared_ptr<vk::Texture> texture_to_process;

		//the index into bufferOffsets of the texture layout
		size_t totalBindingCount = 0;

		uint32_t bindingIndex = 0;
		//layoutIndex = the base layout index to start from, offset by bindingOffset for other textures in a layout.
		uint32_t layoutIndex = 0;

		TextureType type = TextureType::NONE;

		bool needsGPUTransfer = false;
	};

	class TextureManager
	{
	public:
		TextureManager() = default;
		~TextureManager() = default;

		void Init( vk::Device* devicePtr, DescriptorManager* descriptorManagerPtr );
		void Destroy();

		//returns whether or not a command was recorded.
		bool UploadTextureDataToGPU( uint32_t currentFrame, TextureUploadSemaphores& semaphores );
		uint32_t AddTextures(  std::vector<vk::TextureCreateInfo>& createInfos, TextureType type = TextureType::NONE ); //returns the layout index of the texture
	private:
		bool AddTexture( const vk::TextureCreateInfo& createInfo );
	private:
		std::mutex m_textureMutex;
		std::mutex m_pendingTexturesMutex;

		VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
		VkCommandPool m_transferCommandPool = VK_NULL_HANDLE;

		std::array<VkCommandBuffer, gMaxFramesInFlight> m_commandBuffers = {};
		std::array<VkCommandBuffer, gMaxFramesInFlight> m_transferCommandBuffers = {};

		vk::Device* m_devicePtr = nullptr;
		DescriptorManager* m_descriptorManagerPtr = nullptr;

		//TODO: make pendingTextures an unordered map that gets copied from in UploadTextureDataToGPU. This ensures that
		//m_textures only has valid textures for rendering.
		std::vector<PendingTextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
		std::unordered_map<std::string, TextureInfo> m_textures;

		//TODO: make buffer pool for textures.
	};
}

#endif