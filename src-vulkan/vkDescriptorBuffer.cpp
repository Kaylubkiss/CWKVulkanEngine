#include "vkDescriptorBuffer.h"
#include "vkInit.h"

namespace vk
{

    const vk::Buffer& DescriptorBuffer::GetBuffer() const
    {
        return m_buffer;
    }

    size_t DescriptorBuffer::GetBufferSize() const
    {
        return m_bufferSize;
    }

    const std::vector<VkDeviceSize>& DescriptorBuffer::GetBindingOffsets() const
    {
        return m_bindingOffsets;
    }

    VkDeviceSize DescriptorBuffer::GetLayoutSize() const
    {
        return m_setLayoutSize;
    }

    VkDescriptorSetLayout DescriptorBuffer::GetLayout() const
    {
        return m_setLayout;
    }

    void DescriptorBuffer::Destroy()
    {
        if (m_devicePtr != nullptr)
        {
            auto sharedDevicePtr = m_devicePtr;

            m_buffer.Destroy();

            vkDestroyDescriptorSetLayout(sharedDevicePtr->GetDevice(), m_setLayout, nullptr);
            m_setLayout = VK_NULL_HANDLE;
        }
    }

    void DescriptorBuffer::Allocate( vk::Device* devicePtr, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferMemoryProps,
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
			m_bufferSize = numFrames * layoutCount * m_setLayoutSize;
			m_buffer = vk::Buffer(m_devicePtr, bufferUsage, bufferMemoryProps, m_bufferSize);

			m_buffer.Map(); //TODO: remove once integrating descriptor manager. Not all resources should be mapped.
		}

	void DescriptorBuffer::WriteDescriptor( vk::Device* devicePtr, const WriteResource& writeData, uint32_t layoutIndex,
		uint32_t frame, uint32_t binding, size_t writeSize ) const
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
			descriptorPtr + (m_numCols * layoutIndex + frame) * m_setLayoutSize +
			m_bindingOffsets[binding]);
	}

    void DescriptorBuffer::GetDescriptorLayoutSize( const vk::Device* device, VkDescriptorSetLayout layout, VkDeviceSize* size )
    {
        assert(size);
        g_vkGetDescriptorSetLayoutSizeEXT(device->GetDevice(), layout, size);
        *size = AlignedSize(*size, device->GetDescriptorBufferProperties().descriptorBufferOffsetAlignment);
    }

    void DescriptorBuffer::GetDescriptorLayoutBindingOffsets( const vk::Device* device, VkDescriptorSetLayout layout,
    VkDeviceSize offsets[], uint32_t binding_count )
    {
        //get the offsets of each descriptor binding in the layout
        for (uint32_t i = 0; i < binding_count; ++i)
        {
            g_vkGetDescriptorSetLayoutBindingOffsetEXT(device->GetDevice(), layout, i, &offsets[i]);
        }
    }

}