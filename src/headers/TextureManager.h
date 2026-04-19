#pragma once
#include "vkTexture.h"

//NOTE: DOES NOT NEED TO BE A POWER OF TWO
enum class TextureType : uint16_t
{
	NONE = 0x0,
	CUBEMAP = 0x1,
	PANORAMIC = 0x2,
};

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
	size_t totalBindingCount = 0;

	//layoutIndex = the base layout index to start from, offset by bindingOffset for other textures in a layout.
	uint32_t layoutIndex = 0;

	TextureType type = TextureType::NONE;

	bool needsGPUTransfer = false;

	bool isPanoramicImage = false; //needs conversion after being acquired by the graphics queue
};

class TextureManager
{
public:
	TextureManager() = default;
	~TextureManager() = default;

	void Init( vk::Device* devicePtr, DescriptorManager* descriptorManagerPtr );
	void Destroy();

	size_t GetSize();

	//returns whether or not a command was recorded.
	bool UploadTextureDataToGPU( uint32_t currentFrame, VkSemaphore textureUploadSemaphore );
	uint32_t AddTextures( const std::vector<std::string>& fileNames, TextureType type = TextureType::NONE ); //returns the layout index of the texture
private:
	bool AddTexture(const std::string& fileName);
	bool AddCubeMapTexture( const std::vector<std::string>& fileNames );
private:
	std::mutex m_textureMutex;
	std::mutex m_transferMutex;
	std::mutex m_pendingTexturesMutex;

	VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;

	std::array<VkCommandBuffer, gMaxFramesInFlight> m_commandBuffers = {};

	vk::Device* m_devicePtr = nullptr;
	DescriptorManager* m_descriptorManagerPtr = nullptr;

	std::vector<PendingTextureInfo> m_pendingTextures; //textures that need to finish their layout transition.
	std::unordered_map<std::string, TextureInfo> m_textures;

	//TODO: make buffer pool for textures.
};