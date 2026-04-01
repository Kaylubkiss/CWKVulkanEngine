#pragma once

extern PFN_vkGetDescriptorSetLayoutBindingOffsetEXT g_vkGetDescriptorSetLayoutBindingOffsetEXT;
extern PFN_vkGetDescriptorSetLayoutSizeEXT g_vkGetDescriptorSetLayoutSizeEXT;
extern PFN_vkGetDescriptorEXT g_vkGetDescriptorEXT;
extern PFN_vkCmdBindDescriptorBuffersEXT g_vkCmdBindDescriptorBuffersEXT;
extern PFN_vkCmdSetDescriptorBufferOffsetsEXT g_vkCmdSetDescriptorBufferOffsetsEXT;

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
		~DescriptorBuffer() = default;

		[[nodiscard]] const vk::Buffer& GetBuffer() const;
		[[nodiscard]] size_t GetBufferSize() const;
		[[nodiscard]] const std::vector<VkDeviceSize>& GetBindingOffsets() const;
		[[nodiscard]] VkDeviceSize GetLayoutSize() const;
		[[nodiscard]] VkDescriptorSetLayout GetLayout() const;

		void Destroy();
		void Create( const vk::DescriptorBufferCreateInfo& createInfo );
		void Allocate( vk::Device* devicePtr, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferMemoryProps,
			size_t numFrames, size_t layoutCount, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
			m_devicePtr = devicePtr;

			VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = vk::init::DescriptorSetLayoutCreateInfo();
			setLayoutCreateInfo.pBindings = bindings.data();
			setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devicePtr->GetDevice(), &setLayoutCreateInfo,
				nullptr, &this->m_setLayout));

			GetDescriptorLayoutSize(m_devicePtr, m_setLayout, &m_setLayoutSize);

			m_bindingOffsets.resize(setLayoutCreateInfo.bindingCount);

			GetDescriptorLayoutBindingOffsets(m_devicePtr, m_setLayout, m_bindingOffsets.data(),
				setLayoutCreateInfo.bindingCount);

			m_numCols = numFrames;
			m_bufferSize = layoutCount * m_setLayoutSize;
			m_buffer = vk::Buffer(m_devicePtr, bufferUsage, bufferMemoryProps, m_bufferSize);

			m_buffer.Map(); //TODO: remove once integrating descriptor manager. Not all resources should be mapped.
		}

		void WriteDescriptor( vk::Device* devicePtr, const WriteResource& writeData, uint32_t layoutIndex, uint32_t frame,
			uint32_t binding, size_t writeSize )
		{
			assert(writeData.IsValid());

			//TODO: might need to map the memory first before accessing
			char* descriptorPtr = static_cast<char*>(m_buffer.GetMappedMemory());

			VkDescriptorGetInfoEXT descriptorGetInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};

			if (writeData.pImageData)
			{
				descriptorGetInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorGetInfo.data.pCombinedImageSampler = writeData.pImageData;
			}
			else
			{

				descriptorGetInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

				VkDescriptorAddressInfoEXT addrInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
				addrInfo.address = writeData.pResourceData->GetDeviceAddress();
				addrInfo.range = writeData.pResourceData->GetSize();
				addrInfo.format = VK_FORMAT_UNDEFINED;

				descriptorGetInfo.data.pUniformBuffer = &addrInfo;
			}

			g_vkGetDescriptorEXT(devicePtr->GetDevice(), &descriptorGetInfo,
				writeSize,
				descriptorPtr + (layoutIndex * m_numCols + frame) * m_setLayoutSize +
				m_bindingOffsets[binding]);
		}
	private:
		void FillResourceDescriptorBuffers( const vk::DescriptorBufferCreateInfo& createInfo );
		void FillImageDescriptorBuffers( const vk::DescriptorBufferCreateInfo& createInfo );
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


		vk::Device* m_devicePtr = nullptr;
	};

}