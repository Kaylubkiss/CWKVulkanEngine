#include "TextureManager.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"

namespace vk 
{
	
	void TextureManager::Init(GraphicsContextInfo contextInfo)
	{
		assert(contextInfo.devicePtr != nullptr);
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

			obj.UpdateTextureDescriptorOffset(mTextures[fileName].index);
		}

	}

}