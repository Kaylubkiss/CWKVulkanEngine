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

	bool TextureManager::AddTexture(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName)
	{
		if (graphicsContextInfo)
		{
			std::unique_ptr<Texture> newTexture = std::make_unique<Texture>(graphicsContextInfo->devicePtr, fileName);

			if (newTexture.get()->mImage != VK_NULL_HANDLE) 
			{
				UserInterface* UI = graphicsContextInfo->contextUIPtr;

				if (UI)
				{
					UI->AddImage(*newTexture.get());
				}

				this->mTextures[fileName].first = std::move(newTexture);

				return true;
			}
		}

		return false;
	}

	VkDescriptorImageInfo TextureManager::GetTextureDescriptorSet(const char* fileName)
	{
		if (mTextures.count(fileName) == 1) 
		{
			return mTextures[fileName].first.get()->descriptor;
		}

		std::cerr << "could not find specified texture!\n";

		return {};

	}

	void TextureManager::BindTextureToObject(const std::string& fileName, Object& obj)
	{
		if (fileName != "")
		{
			auto descriptorWrites = graphicsContextInfo.sceneWriteDescriptorSets; //TODO: copying a 2D vector...inefficient.

			VkDescriptorSetAllocateInfo descriptorSetInfo = vk::init::DescriptorSetAllocateInfo
			(
				graphicsContextInfo.descriptorPool,
				&graphicsContextInfo.descriptorSetLayout, 1
			);
			
			if (mTextures.count(fileName) == 0)
			{
				bool result = AddTexture(&graphicsContextInfo, fileName);

				if (!result)
				{
					std::cerr << "Could not load " << fileName << std::endl;
					std::cerr << "BindTextureToObject() failed\n";
					return;
				}


				VkDescriptorSet nDescriptorSet = VK_NULL_HANDLE;
				VK_CHECK_RESULT(vkAllocateDescriptorSets(graphicsContextInfo.devicePtr->logical, &descriptorSetInfo, &nDescriptorSet));

				mTextures[fileName].second = std::make_shared<VkDescriptorSet>(nDescriptorSet);
			}

			VkWriteDescriptorSet dscWrite = vk::init::WriteDescriptorSet
			(
				VK_NULL_HANDLE,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				graphicsContextInfo.samplerBinding,
				&mTextures[fileName].first.get()->descriptor
			);
			
			descriptorWrites.push_back(dscWrite);

			obj.UpdateDescriptorSet(descriptorWrites, mTextures[fileName].second);
		}

	}

}