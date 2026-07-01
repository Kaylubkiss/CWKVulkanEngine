#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include <stb_image.h>
#include <variant>

namespace vk 
{
	class Texture
	{
	public:
		Texture() = default;
		Texture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );

		Texture& operator=( const Texture& other ) = delete;
		Texture( const Texture& other ) = delete;

		Texture( Texture&& other ) noexcept;
		Texture& operator=( Texture&& other ) noexcept;

		virtual ~Texture();

		[[nodiscard]] VkDescriptorImageInfo GetDescriptor() const;
		[[nodiscard]] VkImage GetImage() const;
		[[nodiscard]] VkExtent2D GetImageExtent() const;

		void SetImageLayout( VkImageLayout layout );

		static VkImageView CreateImageView( VkDevice l_device, const VkImage& textureImage, VkFormat format, VkImageViewType type );
		static VkSampler CreateSampler( VkPhysicalDevice p_device, VkDevice l_device, uint32_t mipLevels );

		void RecordStagingCopy( VkCommandBuffer cmdBuffer );
		void RecordRelease( VkCommandBuffer cmdBuffer,  uint32_t srcQueueFamily, uint32_t dstQueueFamily );
	private:
		inline bool FormatIsSupported( VkFormat format );
		std::variant<std::monostate, stbi_uc*, float*> LoadPixels( const char* fileName, VkFormat format, int* width, int* height );
		void CreateFromFileName( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );
		void CreateBlankTexture( const vk::Device* devicePtr, const vk::TextureCreateInfo& createInfo );
	protected:
		VkDevice c_device = VK_NULL_HANDLE;

		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;

		uint32_t m_width = 0;
		uint32_t m_height = 0;

		VkDeviceSize m_imageLayerSize = 0;

		VkDescriptorImageInfo m_descriptor = {};

		vk::Buffer m_stagingBuffer; //has to persist because synchronization is done outside of initialization.
		//potential idea: create a vector of vector for each frame, and it stores the synchronization elements in it
		//after waitforfences() is done, then can clear the vector and delete the vk::Buffer.
	};

}

#endif