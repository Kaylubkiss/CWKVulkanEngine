#include "TextureManager.h"
#include "vkUtility.h"
#include "vkBuffer.h"
#include "vkResource.h"
#include "vkInit.h"

namespace vk 
{
	
	void TextureManager::Init(const VkDevice l_device) 
	{
		this->descriptorPool = vk::init::DescriptorPool(l_device);
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
			VkWriteDescriptorSet descriptorWrite[2] = {};
			descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite[0].dstSet = this->mTextures[i].mDescriptorSet;
			descriptorWrite[0].dstBinding = 0;
			descriptorWrite[0].dstArrayElement = 0;
			descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite[0].descriptorCount = uniformDescriptorCount; //how many buffers
			descriptorWrite[0].pBufferInfo = pUniformDescriptorBuffers;
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

			vkUpdateDescriptorSets(l_device, 2, descriptorWrite, 0, nullptr);
		}

	}

	void TextureManager::Add(const VkPhysicalDevice p_device, const VkDevice l_device, const VkQueue gfxQueue, const VkDescriptorSetLayout dscSetLayout, const std::string& fileName)
	{
		this->mTextures.emplace_back(p_device, l_device, gfxQueue, this->descriptorPool, dscSetLayout, fileName);
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

		vkDestroyDescriptorPool(l_device, this->descriptorPool, nullptr);

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


	void TextureManager::BindTextureToObject(const std::string& fileName, GraphicsSystem& graphicsSystem, Object& obj)
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

			VkDescriptorBufferInfo uTransformbufferInfo = {};
			uTransformbufferInfo.buffer = global::uniformBuffer.handle;
			uTransformbufferInfo.offset = 0;
			uTransformbufferInfo.range = sizeof(uTransformObject);

			VkDescriptorBufferInfo bufferInfo[1] = { uTransformbufferInfo };

			TextureManager::UpdateDescriptorSets(graphicsSystem.LogicalDevice(), bufferInfo, 1);

			index = TextureManager::GetTextureIndexByName(fileName.c_str());
		}

		obj.UpdateTexture(this->mTextures[index].mDescriptorSet);
	}
}