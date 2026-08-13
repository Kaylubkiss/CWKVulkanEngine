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

	DescriptorBuffer::~DescriptorBuffer()
	{
    	if (c_device != VK_NULL_HANDLE)
    	{
    		vkDestroyDescriptorSetLayout(c_device, m_setLayout, nullptr);
    		m_setLayout = VK_NULL_HANDLE;
    	}
    }

    DescriptorBuffer::DescriptorBuffer( const vk::Device* devicePtr, VkBufferUsageFlags bufferUsage,
    	VkMemoryPropertyFlags bufferMemoryProps,
		size_t numFrames, size_t layoutCount, const std::vector<VkDescriptorSetLayoutBinding>& bindings )
		{
			c_device = devicePtr->GetDevice();

			VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = vk::init::DescriptorSetLayoutCreateInfo();
			setLayoutCreateInfo.pBindings = bindings.data();
			setLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(c_device, &setLayoutCreateInfo,
				nullptr, &this->m_setLayout));

			GetDescriptorLayoutSize(devicePtr, m_setLayout, &m_setLayoutSize);

			m_bindingOffsets.resize(setLayoutCreateInfo.bindingCount);

			GetDescriptorLayoutBindingOffsets(devicePtr, m_setLayout, m_bindingOffsets.data(),
				setLayoutCreateInfo.bindingCount);

			m_numCols = numFrames;
			m_bufferSize = numFrames * layoutCount * m_setLayoutSize;
			m_buffer = vk::Buffer(devicePtr, bufferUsage, bufferMemoryProps, m_bufferSize);

			m_buffer.Map(); //TODO: remove once integrating descriptor manager. Not all resources should be mapped.
		}

	DescriptorBuffer::DescriptorBuffer( DescriptorBuffer&& other ) noexcept
	{
		if (this != &other)
		{
			this->c_device = other.c_device;
			this->m_setLayout = other.m_setLayout;
			this->m_numCols = other.m_numCols;
			this->m_buffer = std::move(other.m_buffer);
			this->m_bufferSize = other.m_bufferSize;
			this->m_bindingOffsets = other.m_bindingOffsets;
			this->m_setLayoutSize = other.m_setLayoutSize;

			//because Destroy() hinges on c_device being valid, we'll just invalidate
			//c_device on the original resource.
			other.c_device = VK_NULL_HANDLE;
		}
    }

	DescriptorBuffer& DescriptorBuffer::operator=( DescriptorBuffer&& other ) noexcept
    {
    	if (this != &other)
    	{
    		std::swap(this->c_device, other.c_device);
    		std::swap(this->m_setLayout, other.m_setLayout);
    		std::swap(this->m_numCols, other.m_numCols);
    		std::swap(this->m_buffer, other.m_buffer);
    		std::swap(this->m_bufferSize, other.m_bufferSize);
    		std::swap(this->m_bindingOffsets, other.m_bindingOffsets);
    		std::swap(this->m_setLayoutSize, other.m_setLayoutSize);
    	}

    	return *this;
    }

	void DescriptorBuffer::WriteDescriptor( const WriteResource& writeData, uint32_t layoutIndex,
		uint32_t frame, uint32_t binding, size_t writeSize, bool storageResource ) const
	{
    	assert(c_device != VK_NULL_HANDLE);
		assert(writeData.IsValid());

		//TODO: might need to map the memory first before accessing
		char* descriptorPtr = static_cast<char*>(m_buffer.GetMappedMemory());

		VkDescriptorGetInfoEXT descriptorGetInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT};
    	VkDescriptorAddressInfoEXT addrInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };

		if (writeData.pImageData)
		{
			if (writeData.pImageData->imageView == VK_NULL_HANDLE ||
				writeData.pImageData->sampler == VK_NULL_HANDLE)
			{
				return;
			}

			if (storageResource)
			{
				descriptorGetInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}
			else
			{
				descriptorGetInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}

			descriptorGetInfo.data.pCombinedImageSampler = writeData.pImageData;
		}
		else
		{
			descriptorGetInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

			addrInfo.address = writeData.pResourceData->GetDeviceAddress();
			addrInfo.range = writeData.pResourceData->GetSize();
			addrInfo.format = VK_FORMAT_UNDEFINED;

			descriptorGetInfo.data.pUniformBuffer = &addrInfo;
		}

		g_vkGetDescriptorEXT(c_device, &descriptorGetInfo,
			writeSize,
			descriptorPtr + (m_numCols * layoutIndex + frame) * m_setLayoutSize +
			m_bindingOffsets[binding]);
	}

    void DescriptorBuffer::GetDescriptorLayoutSize( const vk::Device* device, VkDescriptorSetLayout layout, VkDeviceSize* size )
    {
        assert(size);
        g_vkGetDescriptorSetLayoutSizeEXT(device->GetDevice(), layout, size);
        *size = vk::util::AlignedSize(*size, device->GetDescriptorBufferProperties().descriptorBufferOffsetAlignment);
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