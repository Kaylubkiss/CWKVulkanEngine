#pragma once
#include "../vk/headers/vkTexture.h"

struct TextureInfo
{
	std::shared_ptr<vk::Texture> handle;
	uint32_t index = 0;
};

struct PendingTextureInfo
{
	std::shared_ptr<vk::Texture> texture_to_process;
	//the index into bufferOffsets of the texture layout
	uint32_t bindingIndex = 0;
	//layoutIndex = the base layout index to start from, offset by bindingOffset for other textures in a layout.
	uint32_t layoutIndex = 0;

	bool needsGPUTransfer = false;
};

class TextureManager
{
public:
	TextureManager() = default;
	~TextureManager() = default;

	void Init( vk::GraphicsContextInfo* contextInfo );
	void Destroy();

	size_t GetSize();
	//don't want ill-use of this getter for some type cast.
	[[nodiscard]] const vk::DescriptorBuffer& GetTextureSamplerDescriptor() const;

	//returns whether or not a command was recorded.
	bool UploadTextureDataToGPU( uint32_t currentFrame, VkSemaphore textureUploadSemaphore );
	uint32_t AddTextures( const std::vector<std::string>& fileNames ); //returns the layout index of the texture
private:
	void FillDescriptorBuffer(const std::vector<PendingTextureInfo>& texturesToProcess) const;
	bool AddTexture(const std::string& fileName, size_t bindingIndex);
private:
	std::mutex m_textureMutex;
	std::mutex m_transferMutex;
	std::mutex m_pendingTexturesMutex;
	std::mutex m_descriptorFreeListMutex;

	VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;

	std::array<VkCommandBuffer, gMaxFramesInFlight> m_commandBuffers = {};

	vk::GraphicsContextInfo* m_graphicsContextInfoPtr = nullptr;

	vk::DescriptorBuffer m_textureSamplerDescriptor;
	std::vector<size_t> m_descriptorFreeList;

	std::vector<PendingTextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
	std::unordered_map<std::string, TextureInfo> m_textures;

	//TODO: make buffer pool for textures.
};