#pragma once

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#define TEXTURE_PATH "External/textures/"

namespace vk 
{
	struct Texture
	{
		//member variables
		VkDevice cLogicalDevice = VK_NULL_HANDLE;

		VkImage mImage = VK_NULL_HANDLE;
		VkDeviceMemory mMemory = VK_NULL_HANDLE;
		VkImageView mImageView = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE; //different mip-levels might need different samplers

		VkDescriptorImageInfo descriptor = {};

		static VkImageView CreateImageView(const VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels);
		static VkSampler CreateSampler(const VkPhysicalDevice p_device, const VkDevice l_device, uint32_t mipLevels);

		Texture() = default;
		Texture& operator=(const Texture& other) = delete;
		Texture(const Texture& other) = delete;
		~Texture();

		//From filename
		Texture(vk::Device* devicePtr, const std::string& fileName);

		//From Gltf file
		Texture(vk::Device* devicePtr, const fastgltf::Asset& asset, fastgltf::Image& gltfImage);
	};

}
