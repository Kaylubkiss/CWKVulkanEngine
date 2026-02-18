#pragma once

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#define TEXTURE_PATH "art/textures/"

namespace vk 
{
	class Texture
	{
	public:
		Texture() = default;
		//From filename
		Texture( const vk::Device* devicePtr, const std::string& fileName, std::mutex& transferMutex );
		~Texture();
		Texture& operator=( const Texture& other ) = delete;
		Texture( const Texture& other ) = delete;

		[[nodiscard]] VkDescriptorImageInfo GetDescriptor() const;
		[[nodiscard]] VkImage GetImage() const;

		static VkImageView CreateImageView( VkDevice l_device,
			const VkImage& textureImage, uint32_t mipLevels );
		static VkSampler CreateSampler( VkPhysicalDevice p_device,
			VkDevice l_device, uint32_t mipLevels );
	private:
		//member variables
		VkDevice c_device = VK_NULL_HANDLE;

		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE; //different mip-levels might need different samplers

		VkDescriptorImageInfo m_descriptor = {};
	};

}
