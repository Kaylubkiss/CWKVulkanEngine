#pragma once

#include "Resource.h"

struct TransferSubmissionInfo;

namespace vk 
{
	class Texture : public Resource
	{
	public:
		Texture() = default;
		explicit Texture(const std::string& id) : Resource(id) {};
		//From filename
		~Texture() override = default;
		Texture& operator=( const Texture& other ) = delete;
		Texture( const Texture& other ) = delete;

		bool NeedsTransferSubmission() const override { return true; }
		bool NeedsDescriptor() const override { return true; }

		[[nodiscard]] VkDescriptorImageInfo GetDescriptor() const;
		[[nodiscard]] VkImage GetImage() const;

		static VkImageView CreateImageView( VkDevice l_device, const VkImage& textureImage, uint32_t mipLevels );
		static VkSampler CreateSampler( VkPhysicalDevice p_device, VkDevice l_device, uint32_t mipLevels );
	protected:
		bool doLoad( vk::Device* devicePtr, ResourceManager& resourceManager ) override;
		bool doTransferSubmission( vk::Device* devicePtr, TransferSubmissionInfo& transferInfo ) override;
		void doUnload( vk::Device* devicePtr ) override;
	private:
		//member variables
		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE; //different mip-levels might need different samplers
		unsigned char* m_pixels = nullptr;
		int m_width = 0;
		int m_height = 0;
		int m_channels = 0;
		VkDescriptorImageInfo m_descriptor = {};

	};

}
