#pragma once

namespace vk 
{
	class Texture
	{
	public:
		Texture() = default;
		//From filename
		virtual ~Texture();
		Texture& operator=( const Texture& other ) = delete;
		Texture( const Texture& other ) = delete;

		virtual void Create( const vk::Device* devicePtr, const std::vector<std::string>& fileNames, std::mutex& transferMutex );

		[[nodiscard]] VkDescriptorImageInfo GetDescriptor() const;
		[[nodiscard]] VkImage GetImage() const;

		static VkImageView CreateImageView( VkDevice l_device, const VkImage& textureImage, VkImageViewType type );
		static VkSampler CreateSampler( VkPhysicalDevice p_device, VkDevice l_device );
	protected:
		void RecordTransferOperations(const vk::Device* devicePtr, const vk::Buffer& stagingBuffer, std::mutex& submissionMutex);
	protected:
		//member variables
		VkDevice c_device = VK_NULL_HANDLE;

		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE; //different mip-levels might need different samplers

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		size_t m_imageCount = 0;

		VkDeviceSize m_imageLayerSize = 0;

		VkDescriptorImageInfo m_descriptor = {};

	};

}
