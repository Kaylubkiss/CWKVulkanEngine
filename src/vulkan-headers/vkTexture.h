#pragma once

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#define TEXTURE_PATH "assets/textures/"

namespace vk 
{
	struct Texture
	{
		Texture() = default;
		Texture& operator=( const Texture& other ) = delete;
		Texture( const Texture& other ) = delete;
		//From filename
		Texture( const vk::Device* devicePtr, const std::string& fileName );
		~Texture();

		static VkImageView CreateImageView( VkDevice l_device,
			const VkImage& textureImage, uint32_t mipLevels );
		static VkSampler CreateSampler( VkPhysicalDevice p_device,
			VkDevice l_device, uint32_t mipLevels );
		
		//member variables
		VkDevice cLogicalDevice = VK_NULL_HANDLE;

		VkImage mImage = VK_NULL_HANDLE;
		VkDeviceMemory mMemory = VK_NULL_HANDLE;
		VkImageView mImageView = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE; //different mip-levels might need different samplers

		VkDescriptorImageInfo descriptor = {};
	};

}
