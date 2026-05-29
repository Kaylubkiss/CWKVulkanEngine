#pragma once

namespace vk 
{
	class Texture
	{
	public:
		Texture() = default;
		Texture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );
		Texture( const Texture& other ) = delete;
		Texture( Texture&& other ) noexcept;

		Texture& operator=( const Texture& other ) = delete;
		Texture& operator=( Texture&& other ) noexcept;

		virtual ~Texture();

		[[nodiscard]] VkDescriptorImageInfo GetDescriptor() const;
		[[nodiscard]] VkImage GetImage() const;

		void SetImageLayout( VkImageLayout layout );

		static VkImageView CreateImageView( VkDevice l_device, const VkImage& textureImage, VkFormat format, VkImageViewType type );
		static VkSampler CreateSampler( VkPhysicalDevice p_device, VkDevice l_device, uint32_t mipLevels );
	protected:
		void RecordTransferAndReleaseOperations( const vk::Device* devicePtr, const vk::Buffer& stagingBuffer, std::mutex* submissionMutex );
	private:
		void CreateFromFileName( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );
		void CreateBlankTexture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );
	protected:
		VkDevice c_device = VK_NULL_HANDLE;

		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		size_t m_imageCount = 0;

		VkDeviceSize m_imageLayerSize = 0;

		VkDescriptorImageInfo m_descriptor = {};
	};

}
