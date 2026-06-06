#ifndef VK_DESCRIPTOR_BUFFER_HPP
#define VK_DESCRIPTOR_BUFFER_HPP

namespace vk
{
	//row = frame
	//col = binding
	typedef std::vector<std::vector<vk::Buffer*>> resourceBufferPtrs2D;
	typedef std::vector<std::vector<VkDescriptorImageInfo>> imageBuffers2D;

	struct WriteResource
	{
		vk::Buffer* pResourceData = nullptr;
		VkDescriptorImageInfo* pImageData = nullptr;
		[[nodiscard]] bool IsValid() const
		{
			return (pResourceData == nullptr && pImageData) || (pImageData == nullptr && pResourceData);
		}
	};

    struct DescriptorBufferCreateInfo
	{
		uint32_t minTextureCount = 0;
		vk::Device* devicePtr;
		VkDescriptorSetLayoutBinding* pLayoutBindings = nullptr;
    	uint32_t layoutBindingCount = 0;
		resourceBufferPtrs2D resourceDescriptorData;
    	imageBuffers2D imageDescriptorData;
		VkBufferUsageFlags bufferUsageFlags;
		VkMemoryPropertyFlags memoryProperties;
	};

	class DescriptorBuffer
	{
	public:
		DescriptorBuffer() = default;
		DescriptorBuffer( const vk::Device* devicePtr, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferMemoryProps,
			size_t numFrames, size_t layoutCount, const std::vector<VkDescriptorSetLayoutBinding>& bindings );

		DescriptorBuffer( const DescriptorBuffer& other ) = delete;
		DescriptorBuffer& operator=( const DescriptorBuffer& other ) = delete;

		DescriptorBuffer( DescriptorBuffer&& other ) noexcept;
		DescriptorBuffer& operator=( DescriptorBuffer&& other ) noexcept;

		~DescriptorBuffer();

		[[nodiscard]] const vk::Buffer& GetBuffer() const;
		[[nodiscard]] size_t GetBufferSize() const;
		[[nodiscard]] const std::vector<VkDeviceSize>& GetBindingOffsets() const;
		[[nodiscard]] VkDeviceSize GetLayoutSize() const;
		[[nodiscard]] VkDescriptorSetLayout GetLayout() const;

		void WriteDescriptor( const WriteResource& writeData, uint32_t layoutIndex,
			uint32_t frame, uint32_t binding, size_t writeSize, bool storageResource = false ) const;
	private:
		static void GetDescriptorLayoutSize( const vk::Device* device, VkDescriptorSetLayout layout, VkDeviceSize* size );
		static void GetDescriptorLayoutBindingOffsets( const vk::Device* device, VkDescriptorSetLayout layout,
		VkDeviceSize offsets[], uint32_t binding_count );
	private:
		//per-frame resources need to be independently updated according to which frame is active.
		//1st index of "buffers" is the minimum ever used by a descriptor buffer.
		vk::Buffer m_buffer;
		size_t m_bufferSize = 0ull;
		size_t m_numCols = 0; // [descriptor][frame]

		//at least 1 binding (binding 0)
		std::vector<VkDeviceSize> m_bindingOffsets = { 0ull };

		VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
		VkDeviceSize m_setLayoutSize = 0ull;

		VkDevice c_device = VK_NULL_HANDLE;
	};

}

#endif