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
		this->graphicsContext = context;
	}

	void TextureManager::Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const std::string& fileName)
	{
		this->mTextures.emplace_back(p_device, l_device, gfxQueue, fileName);
	}

	void TextureManager::Add(const Texture& nTexture) 
	{
		//will use copy constructor
		this->mTextures.push_back(nTexture);
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

				TextureManager::Add(graphicsContext->PhysicalDevice(),
					graphicsContext->LogicalDevice(),
					graphicsContext->GraphicsQueue().handle,
					fileName);

				VkDescriptorImageInfo imageInfo = {};

				imageInfo.imageView = mTextures.back().descriptor.imageView;
				imageInfo.imageLayout = mTextures.back().descriptor.imageLayout;
				imageInfo.sampler = mTextures.back().descriptor.sampler;

				mTextures.back().descriptorSet = vk::init::DescriptorSet(graphicsContext->LogicalDevice(),
					graphicsContext->DescriptorPool(),
					graphicsContext->DescriptorSetLayout());

				std::vector<VkWriteDescriptorSet> descriptorWrites = graphicsContext->WriteDescriptorBuffers(mTextures.back().descriptorSet);

				//writing the texture sampler.
				VkWriteDescriptorSet dscWrite = {};
				dscWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				dscWrite.dstSet = mTextures.back().descriptorSet;
				dscWrite.dstBinding = graphicsContext->SamplerDescriptorSetBinding();
				dscWrite.dstArrayElement = 0;
				dscWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				dscWrite.descriptorCount = 1; //how many images
				dscWrite.pImageInfo = &imageInfo;
				dscWrite.pTexelBufferView = nullptr; // Optional

				descriptorWrites.push_back(dscWrite);

				vkUpdateDescriptorSets(graphicsContext->LogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

				index = mTextures.size() - 1;

			}

			obj.AddTextureDescriptor(mTextures[index].descriptorSet);
		}

	}
}