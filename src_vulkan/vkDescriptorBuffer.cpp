#include "vkDescriptorBuffer.h"

namespace vk
{

    const vk::Buffer& DescriptorBuffer::GetBuffer() const
    {
        return m_buffer;
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

    void DescriptorBuffer::Destroy(vk::Device* devicePtr)
    {
        if (devicePtr != nullptr)
        {
            auto sharedDevicePtr = devicePtr;

            m_buffer.Destroy();

            vkDestroyDescriptorSetLayout(sharedDevicePtr->GetDevice(), m_setLayout, nullptr);
            m_setLayout = VK_NULL_HANDLE;
        }
    }

    void DescriptorBuffer::FillResourceDescriptorBuffers( const vk::DescriptorBufferCreateInfo& createInfo )
    {
        /*const auto& dataBuffer = createInfo.resourceDescriptorData;

        if (dataBuffer.empty())
        {
            std::cerr << "allocated a resource buffer, but no data was provided.\n";
            return;
        }

        size_t frameCount = dataBuffer.size();

        auto sharedDevicePtr = m_devicePtr;

        m_buffer = vk::Buffer(sharedDevicePtr, createInfo.bufferUsageFlags,
              createInfo.memoryProperties, frameCount * m_setLayoutSize);
        m_buffer.Map();

        char* descriptor_ptr = static_cast<char*>(m_buffer.GetMappedMemory());
        for (size_t frame = 0; frame < frameCount; ++frame)
        {
            assert(m_bindingOffsets.size() >= dataBuffer[frame].size());

            for (size_t binding = 0; binding < dataBuffer[frame].size(); ++binding)
            {
                VkDescriptorAddressInfoEXT addrInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
                addrInfo.address = dataBuffer[frame][binding]->GetDeviceAddress();
                addrInfo.range = dataBuffer[frame][binding]->GetSize();
                addrInfo.format = VK_FORMAT_UNDEFINED;

                VkDescriptorGetInfoEXT bufferDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
                bufferDescriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                bufferDescriptorInfo.data.pUniformBuffer = &addrInfo;

                g_vkGetDescriptorEXT(sharedDevicePtr->GetDevice(), &bufferDescriptorInfo,
                    sharedDevicePtr->GetDescriptorBufferProperties().uniformBufferDescriptorSize,
                    descriptor_ptr + frame * m_setLayoutSize + m_bindingOffsets[binding]);
            }
        }

        if ((createInfo.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            m_buffer.UnMap();
        }*/
    }

    void DescriptorBuffer::FillImageDescriptorBuffers( const vk::DescriptorBufferCreateInfo& createInfo )
    {
        /*const auto& imageDescriptors = createInfo.imageDescriptorData;

        if (imageDescriptors.empty())
        {
            std::cerr << "allocated a image descriptor buffer, but no data was provided.\n";
            return;
        }

        size_t frameCount = createInfo.imageDescriptorData.size();

        auto sharedDevicePtr = m_devicePtr;

        m_buffer = vk::Buffer(sharedDevicePtr, createInfo.bufferUsageFlags,
                createInfo.memoryProperties, frameCount * m_setLayoutSize);
        m_buffer.Map();

        char* descriptor_ptr = static_cast<char*>(m_buffer.GetMappedMemory());
        for (size_t frame = 0; frame < frameCount; ++frame)
        {
            assert(m_bindingOffsets.size() >= imageDescriptors[frame].size());

            for (size_t binding = 0; binding < imageDescriptors[frame].size(); ++binding)
            {
                VkDescriptorGetInfoEXT imageDescriptorGetInfo = {};
                imageDescriptorGetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
                imageDescriptorGetInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                imageDescriptorGetInfo.data.pCombinedImageSampler = &imageDescriptors[frame][binding];

                g_vkGetDescriptorEXT(sharedDevicePtr->GetDevice(), &imageDescriptorGetInfo,
                    sharedDevicePtr->GetDescriptorBufferProperties().combinedImageSamplerDescriptorSize,
                    descriptor_ptr + frame * m_setLayoutSize + m_bindingOffsets[binding]);
            }
        }

        if ((createInfo.memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            m_buffer.UnMap();
        }*/
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