#include "TextureManager.h"
#include "vkUtility.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"

namespace vk 
{
	
	void TextureManager::Init(ContextBase* context)
	{
		assert(context != nullptr);
		//this->graphicsContext = context;
		graphicsContextInfo = context->GetGraphicsContextInfo();
	}

	bool TextureManager::AddTexture(GraphicsContextInfo* graphicsContextInfo, const std::string& fileName)
	{
		if (graphicsContextInfo)
		{
			Texture newTexture = Texture(graphicsContextInfo, fileName);

			if (newTexture.mTextureImage != VK_NULL_HANDLE) 
			{
				this->mTextures.push_back(newTexture);

				UserInterface* UI = graphicsContextInfo->contextUIPtr;
				if (UI)
				{
					UI->AddImage(newTexture);
				}

				return true;
			}
		}

		return false;
	}

	void TextureManager::Destroy(const VkDevice l_device) 
	{
		for (size_t i = 0; i < mTextures.size(); ++i)
		{
			mTextures[i].Destroy(l_device);
		}
	}

	const Texture& TextureManager::GetTextureObject(size_t index) const
	{
		if (index < 0 || index >= mTextures.size()) 
		{
			throw std::runtime_error("could not find specified texture!\n");
		}

		return mTextures[index];
	}

	int TextureManager::GetTextureIndexByName(const char* fileName) const 
	{
		for (size_t i = 0; i < mTextures.size(); ++i)
		{
			if (strcmp(fileName, mTextures[i].mName.c_str()) == 0)
			{
				return i;
			}
		}

		std::cerr << "could not find specified texture!\n";

		return -1;

	}

	void TextureManager::BindTextureToObject(const std::string& fileName, Object& obj)
	{
		if (fileName != "")
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites = graphicsContextInfo.sceneWriteDescriptorSets; //TODO: copying a vector...inefficient.

			VkDescriptorSetAllocateInfo descriptorSetInfo = vk::init::DescriptorSetAllocateInfo
			(
				graphicsContextInfo.descriptorPool,
				&graphicsContextInfo.descriptorSetLayout, 1
			);

			int index = TextureManager::GetTextureIndexByName(fileName.c_str());
			if (index < 0)
			{
				std::cout << "adding texture...\n";

				bool result  = AddTexture(&graphicsContextInfo, fileName);

				if (!result) 
				{
					return;
				}

				index = mTextures.size() - 1;

			}

			VkWriteDescriptorSet dscWrite = vk::init::WriteDescriptorSet
			(
				VK_NULL_HANDLE,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				graphicsContextInfo.samplerBinding,
				&mTextures[index].descriptor
			);

			descriptorWrites.push_back(dscWrite);

			obj.UpdateDescriptorSets(descriptorWrites, &descriptorSetInfo);
		}

	}


	const std::vector<vk::Texture>& TextureManager::Textures() const 
	{
		return this->mTextures;
	}
}