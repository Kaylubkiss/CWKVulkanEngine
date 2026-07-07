#include "headers/vkDescriptorManager.h"

namespace vk
{

    void DescriptorManager::Init( vk::Device* devicePtr )
    {
        assert(devicePtr != nullptr);

        m_devicePtr = devicePtr;
        m_properties = m_devicePtr->GetDescriptorBufferProperties();
    }

    void DescriptorManager::Destroy()
    {
        std::lock_guard lock(m_mutex);
        m_descriptorBuffers.clear();
    }

    void DescriptorManager::AllocateDescriptorBuffer(DescriptorCategory category, size_t numFrames, size_t layoutCount,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        std::lock_guard lock(m_mutex);

        if (m_descriptorBuffers.contains(category))
        {
            m_descriptorBuffers[category].descriptor.~DescriptorBuffer();
            m_descriptorBuffers[category].freeList.clear();
        }

        //create the layout for the descriptor buffer.
        //allocate the buffer for the descriptor buffer.
        VkBufferUsageFlags bufferUsageFlags;
        VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; //TODO: TEMPORARY. Not all the buffers should use these flags.

        if (category == DescriptorCategory::eUBO || category == DescriptorCategory::eObject)
        {
            bufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }
        else if (category == DescriptorCategory::eMaterial)
        {
            bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }
        else if (category == DescriptorCategory::eCompositionImage) //TODO: might get away with the same usage as eMaterial.
        {
            bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }
        else
        {
            std::cerr << "Unknown descriptor category specified\n";
            throw std::runtime_error("DescriptorManager::AllocateDescriptorBuffer() Failed\n");
        }

        m_descriptorBuffers[category].descriptor = vk::DescriptorBuffer(
            m_devicePtr, bufferUsageFlags, memoryProperties,
            numFrames, layoutCount, bindings
            );

        m_descriptorBuffers[category].freeList.reserve(layoutCount);

        for (uint32_t i = 0; i < layoutCount; ++i)
        {
            m_descriptorBuffers[category].freeList.push_back(i);
        }
    }

    // [frame][binding]
    void DescriptorManager::WriteDescriptors(DescriptorCategory category, uint32_t layoutIndex,
        vk::imageBuffers2D& imageDescriptors, bool storageResource, int baseBinding )
    {
        auto& descriptor = m_descriptorBuffers[category].descriptor;

        vk::WriteResource writeResource;
        size_t writeSize = 0;

        if (storageResource)
        {
            writeSize = m_properties.storageImageDescriptorSize;
        }
        else
        {
            writeSize = m_properties.combinedImageSamplerDescriptorSize;
        }

        for (int frame = 0; frame < imageDescriptors.size(); ++frame)
        {
            for (int binding = 0; binding < imageDescriptors[frame].size(); ++binding)
            {
                writeResource.pImageData = &imageDescriptors[frame][binding];

                {
                    std::lock_guard lock(m_mutex);
                    descriptor.WriteDescriptor(writeResource,
                       layoutIndex, frame, baseBinding + binding, writeSize);
                }
            }
        }
    }

    void DescriptorManager::WriteDescriptors(DescriptorCategory category, uint32_t layoutIndex,
        vk::resourceBufferPtrs2D& resourceDescriptors)
    {
        auto& descriptor = m_descriptorBuffers[category].descriptor;

        vk::WriteResource writeResource;

        for (int frame = 0; frame < resourceDescriptors.size(); ++frame)
        {
            for (int binding = 0; binding < resourceDescriptors[frame].size(); ++binding)
            {
                writeResource.pResourceData = resourceDescriptors[frame][binding];

                {
                    std::lock_guard lock(m_mutex);
                    descriptor.WriteDescriptor( writeResource,
                        layoutIndex, frame, binding, m_properties.uniformBufferDescriptorSize);
                }
            }
        }
    }

    uint32_t DescriptorManager::GetLayoutIndex(DescriptorCategory category)
    {
        std::lock_guard lock(m_mutex);

        if (m_descriptorBuffers.contains(category) == false)
        {
            return -1;
        }

        if (m_descriptorBuffers[category].freeList.empty())
        {
            std::cerr << "no more space in the free list for descriptor category\n";
            throw std::runtime_error("DescriptorBuffer::GetLayoutIndex() failed");
        }

        uint32_t index = m_descriptorBuffers[category].freeList.back();
        m_descriptorBuffers[category].freeList.pop_back();

        return index;
    }

    VkDeviceSize DescriptorManager::GetLayoutSize(DescriptorCategory category)
    {
        std::lock_guard lock(m_mutex);

        if (m_descriptorBuffers.contains(category))
        {
            return m_descriptorBuffers[category].descriptor.GetLayoutSize();
        }

        return -1;
    }

    VkDescriptorSetLayout DescriptorManager::GetLayout(DescriptorCategory category)
    {
        std::shared_lock lock(m_mutex);

        if (m_descriptorBuffers.contains(category))
        {
            return m_descriptorBuffers[category].descriptor.GetLayout();
        }

        return VK_NULL_HANDLE;
    }

    VkDeviceAddress DescriptorManager::GetDescriptorAddress(DescriptorCategory category)
    {
        std::lock_guard lock(m_mutex);

        if (m_descriptorBuffers.contains(category)) {
            return m_descriptorBuffers[category].descriptor.GetBuffer().GetDeviceAddress();
        }

        std::cerr << "DescriptorManager::GetDescriptorAddress(), device address is invalid for requested category\n";

        return -1;
    }
}
