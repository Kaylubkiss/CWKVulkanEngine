#include "TextureManager.h"
#include "vkUtility.h"
#include "vkBuffer.h"
#include "vkResource.h"
#include "vkInit.h"
#include "ApplicationGlobal.h"

namespace vk 
{
	
	void TextureManager::Init(const ContextBase* graphicsContext)
	{
		assert(_Application != nullptr);

		this->contextDescriptorPool = graphicsContext->DescriptorPool();
	}

	//Inefficient. Look into a way to individually write descriptor sets. Shouldn't be hard. Later.
	void TextureManager::UpdateDescriptorSets(const VkDevice l_device, const VkDescriptorBufferInfo* pUniformDescriptorBuffers, size_t uniformDescriptorCount) 
	{
		VkDescriptorImageInfo imageInfo = {};

		for (size_t i = 0; i < this->mTextures.size(); ++i)
		{
			//potentially extra check in here to see if the descriptor set needs to be updated??

			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = this->mTextures[i].mTextureImageView;
			imageInfo.sampler = this->mTextures[i].mTextureSampler;

			//writing the uniform transforms.
			VkWriteDescriptorSet descriptorWrite[3] = {};
			descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[0].dstSet = this->mTextures[i].mDescriptorSet;
			descriptorWrite[0].dstBinding = 0;
			descriptorWrite[0].dstArrayElement = 0;
			descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite[0].descriptorCount = 1; //how many buffers
			descriptorWrite[0].pBufferInfo = &pUniformDescriptorBuffers[0];
			descriptorWrite[0].pImageInfo = nullptr; // Optional
			descriptorWrite[0].pTexelBufferView = nullptr; // Optional

			//writing the texture sampler.
			descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[1].dstSet = this->mTextures[i].mDescriptorSet;
			descriptorWrite[1].dstBinding = 1;
			descriptorWrite[1].dstArrayElement = 0;
			descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite[1].descriptorCount = 1; //how many images
			descriptorWrite[1].pImageInfo = &imageInfo;
			descriptorWrite[1].pTexelBufferView = nullptr; // Optional

			descriptorWrite[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[2].dstSet = this->mTextures[i].mDescriptorSet;
			descriptorWrite[2].dstBinding = 2;
			descriptorWrite[2].dstArrayElement = 0;
			descriptorWrite[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite[2].descriptorCount = 1; //how many buffers
			descriptorWrite[2].pBufferInfo = &pUniformDescriptorBuffers[1];
			descriptorWrite[2].pImageInfo = nullptr; // Optional
			descriptorWrite[2].pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(l_device, 3, descriptorWrite, 0, nullptr);
		}

	}

	void TextureManager::Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName)
	{
		this->mTextures.emplace_back(p_device, l_device, gfxQueue, *this->contextDescriptorPool.get(), dscSetLayout, fileName);
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

		/*throw std::runtime_error("could not find specified texture!\n");*/
		std::cerr << "could not find specified texture!\n";

		return -1;

	}


	void TextureManager::BindTextureToObject(const std::string& fileName, ContextBase& graphicsSystem, Object& obj)
	{
		int index = TextureManager::GetTextureIndexByName(fileName.c_str());

		
		if (index < 0) 
		{
			std::cout << "adding texture...\n";

			TextureManager::Add(graphicsSystem.PhysicalDevice(), 
							    graphicsSystem.LogicalDevice(), 
								graphicsSystem.GraphicsQueue().handle, 
								graphicsSystem.DescriptorSetLayout(),
								fileName);

			std::vector<VkDescriptorBufferInfo> bufferInfo = graphicsSystem.DescriptorBuffers();

			TextureManager::UpdateDescriptorSets(graphicsSystem.LogicalDevice(), bufferInfo.data(), bufferInfo.size());

			index = mTextures.size() - 1;
		}

		obj.UpdateTexture(this->mTextures[index].mDescriptorSet);
	}
}