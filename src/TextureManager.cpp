#include "TextureManager.h"
#include "vkUtility.h"
#include "vkBuffer.h"
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
			int index = TextureManager::GetTextureIndexByName(fileName.c_str());

			if (index < 0)
			{
				std::cout << "adding texture...\n";

				bool result  = AddTexture(&graphicsContextInfo, fileName);

				if (!result) 
				{
					return;
				}

				VkDescriptorSetAllocateInfo descriptorSetInfo = vk::init::DescriptorSetAllocateInfo
				(
					graphicsContextInfo.descriptorPool, 
					&graphicsContextInfo.descriptorSetLayout, 1
				);

				VK_CHECK_RESULT(vkAllocateDescriptorSets(graphicsContextInfo.logicalDevice, &descriptorSetInfo, &mTextures.back().descriptorSet));

				std::vector<VkWriteDescriptorSet> descriptorWrites = graphicsContextInfo.sceneWriteDescriptorSets; //TODO: copying a vector...inefficient.

				//need to make sure the descriptor set pointed at by the writes is the texture's.
				for (int i = 0; i < descriptorWrites.size(); ++i)
				{
					descriptorWrites[i].dstSet = mTextures.back().descriptorSet;
				}

				//writing the texture sampler.
				VkWriteDescriptorSet dscWrite = vk::init::WriteDescriptorSet(
					mTextures.back().descriptorSet, 
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
					graphicsContextInfo.samplerBinding, 
					&mTextures.back().descriptor
				);

			
				descriptorWrites.push_back(dscWrite);
				

				vkUpdateDescriptorSets(graphicsContextInfo.logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

				index = mTextures.size() - 1;

			}

			obj.AddTextureDescriptorSet(mTextures[index].descriptorSet);
		}

	}


	const std::vector<vk::Texture>& TextureManager::Textures() const 
	{
		return this->mTextures;
	}
}