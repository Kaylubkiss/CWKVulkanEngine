#include "TextureManager.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"

namespace vk 
{
	
	void TextureManager::Init(GraphicsContextInfo contextInfo)
	{
		assert(contextInfo.devicePtr != nullptr);
		assert(contextInfo.contextTextureDescriptorPtr != nullptr);
		graphicsContextInfo = contextInfo;
	}

	bool TextureManager::AddTexture(GraphicsContextInfo& graphicsContextInfo, const std::string& fileName)
	{
		auto* devicePtr = graphicsContextInfo.devicePtr;
		std::unique_ptr<Texture> newTexture = std::make_unique<Texture>(devicePtr, fileName);

		if (newTexture.get()->mImage != VK_NULL_HANDLE) 
		{
			UserInterface* UI = graphicsContextInfo.contextUIPtr;

			if (UI)
			{
				UI->AddImage(*newTexture.get());
			}

			this->mTextures[fileName].handle = std::move(newTexture);
			this->mTextures[fileName].index = mTextures.size();

			return true;
		}

		return false;
	}

	VkDescriptorImageInfo TextureManager::GetTextureDescriptorInfo(const char* fileName)
	{
		if (mTextures.count(fileName)) 
		{
			return mTextures[fileName].handle.get()->descriptor;
		}

		std::cerr << "could not find specified texture!\n";
		std::cerr << "GetTextureDescriptorInfo() Failed.\n";
		return {};

	}

	VkDescriptorImageInfo TextureManager::GetTextureDescriptorInfo(uint32_t index) 
	{

		for (auto& t : mTextures)
		{
			TextureInfo& texture = t.second;
			if (texture.index == 0) 
			{
				return texture.handle.get()->descriptor;
			}
		}

		std::cerr << "could not find specified texture!\n";
		std::cerr << "GetTextureDescriptorInfo() Failed.\n";
		
		return {};
	}

	void TextureManager::BindTextureToObject(const std::string& fileName, Object& obj)
	{
		if (fileName != "")
		{			
			if (mTextures.count(fileName) == 0)
			{
				if (AddTexture(graphicsContextInfo, fileName) == false)
				{
					std::cerr << "Could not load " << fileName << '\n';
					std::cerr << "BindTextureToObject() failed\n";
					return;
				}	
			}

			//NOTE: this assumes that there is only one binding for the textures.
			VkDeviceSize textureBindingSize = 
				graphicsContextInfo.contextTextureDescriptorPtr->size;

			VkDeviceSize combinedImageSamplerSize = 
				graphicsContextInfo.devicePtr->DescriptorBufferProperties().combinedImageSamplerDescriptorSize;
			
			VkDeviceSize bindingOffset =
				graphicsContextInfo.contextTextureDescriptorPtr->binding_offsets.front();

			VkDescriptorGetInfoEXT imageDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
			imageDescriptorInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageDescriptorInfo.data.pCombinedImageSampler = &mTextures[fileName].handle.get()->descriptor;

			char* imageBindingDescriptorPtr = 
				(char*)(graphicsContextInfo.contextTextureDescriptorPtr->buffers.front().mappedMemory);

			g_vkGetDescriptorEXT(graphicsContextInfo.devicePtr->logical, &imageDescriptorInfo, combinedImageSamplerSize,
				imageBindingDescriptorPtr + mTextures[fileName].index * textureBindingSize + bindingOffset);

			obj.UpdateTextureDescriptorOffset(mTextures[fileName].index);
		}

	}

}