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
	typedef std::vector<std::vector<const vk::Buffer*>> resourceBufferPtrs2D;
	typedef std::vector<std::vector<VkDescriptorImageInfo>> imageBuffers2D;

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
		[[nodiscard]] const std::vector<VkDeviceSize>& GetBindingOffsets() const;
		[[nodiscard]] VkDeviceSize GetLayoutSize() const;
		[[nodiscard]] VkDescriptorSetLayout GetLayout() const;

		void Destroy();
		void Create( const vk::DescriptorBufferCreateInfo& createInfo );

		void WriteDescriptors( vk::Device* devicePtr, uint32_t layoutIndex, size_t writeSize,
			const imageBuffers2D& imageDescriptors )
		{
			//TODO: might need to map the memory first before accessing
			char* descriptorPtr = static_cast<char*>(m_buffer.GetMappedMemory());

			for (size_t frame = 0; frame < imageDescriptors.size(); ++frame)
			{
				size_t imageBindingCount = imageDescriptors[frame].size();

				assert(imageBindingCount <= m_bindingOffsets.size());

				for (size_t binding = 0; binding < imageBindingCount; ++binding)
				{
					VkDescriptorGetInfoEXT imageDescriptorGetInfo = {};
					imageDescriptorGetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
					imageDescriptorGetInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					imageDescriptorGetInfo.data.pCombinedImageSampler = &imageDescriptors[frame][binding];

					g_vkGetDescriptorEXT(devicePtr->GetDevice(), &imageDescriptorGetInfo,
						writeSize,
						descriptorPtr + frame * layoutIndex * m_setLayoutSize +
						m_bindingOffsets[binding]);
				}
			}

			//TODO: might need to unmap the memory before leaving.
		}
		void WriteDescriptors( vk::Device* devicePtr, uint32_t layoutIndex, size_t writeSize,
			const resourceBufferPtrs2D& resourceBuffers )
		{
			//TODO: might need to map the memory first before accessing
			char* descriptorPtr = static_cast<char*>(m_buffer.GetMappedMemory());

			for (size_t frame = 0; frame < resourceBuffers.size(); ++frame)
			{
				size_t imageBindingCount = resourceBuffers[frame].size();

				assert(imageBindingCount <= m_bindingOffsets.size());

				for (size_t binding = 0; binding < imageBindingCount; ++binding)
				{
					VkDescriptorAddressInfoEXT addrInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
					addrInfo.address = resourceBuffers[frame][binding]->GetDeviceAddress();
					addrInfo.range = resourceBuffers[frame][binding]->GetSize();
					addrInfo.format = VK_FORMAT_UNDEFINED;

					VkDescriptorGetInfoEXT bufferDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
					bufferDescriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					bufferDescriptorInfo.data.pUniformBuffer = &addrInfo;

					g_vkGetDescriptorEXT(devicePtr->GetDevice(), &bufferDescriptorInfo,
						writeSize,
						descriptorPtr + frame * (layoutIndex * m_setLayoutSize) +
						m_bindingOffsets[binding]);
				}
			}
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

		//at least 1 binding (binding 0)
		std::vector<VkDeviceSize> m_bindingOffsets = { 0ull };

		VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
		VkDeviceSize m_setLayoutSize = 0ull;

		vk::Device* m_devicePtr = nullptr;
	};

}