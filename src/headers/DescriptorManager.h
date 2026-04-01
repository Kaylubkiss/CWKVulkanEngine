#pragma once

#include "vkDescriptorBuffer.h"

enum class DescriptorCategory
{
    eUBO, //set 0: ubo
    eCompositionImage, //set 1
    eMaterial, //set 2
    eObject, //set 3: model transforms, animation state. Unused for now.
};

class DescriptorManager
{
public:
    DescriptorManager() = default;
    ~DescriptorManager() = default;

    void Init( vk::Device* devicePtr )
    {
        assert(devicePtr != nullptr);

        m_devicePtr = devicePtr;
        m_properties = m_devicePtr->GetDescriptorBufferProperties();
    }

    void AllocateDescriptorBuffer(DescriptorCategory category, size_t slots, size_t layoutCount, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        if (m_descriptorBuffers.contains(category))
        {
            return;
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
            bufferUsageFlags = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
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

        m_descriptorBuffers[category].descriptor.Allocate(m_devicePtr, bufferUsageFlags, memoryProperties,
            gMaxFramesInFlight, layoutCount, bindings);

        m_descriptorBuffers[category].freeList.resize(layoutCount);

        for (size_t i = 0; i < slots; ++i)
        {
            m_descriptorBuffers[category].freeList.push_back(i);
        }
    }

    // [frame][binding]
    void WriteDescriptors(DescriptorCategory category, uint32_t layoutIndex, vk::imageBuffers2D& imageDescriptors)
    {
        auto& descriptor = m_descriptorBuffers[category].descriptor;

        vk::WriteResource writeResource;

        for (int frame = 0; frame < imageDescriptors.size(); ++frame)
        {
            for (int binding = 0; binding < imageDescriptors[frame].size(); ++binding)
            {
                writeResource.pImageData = &imageDescriptors[frame][binding];

                descriptor.WriteDescriptor(m_devicePtr, writeResource,
                   layoutIndex, frame, binding, m_properties.combinedImageSamplerDescriptorSize);
            }
        }
    }

    void WriteDescriptors(DescriptorCategory category, uint32_t layoutIndex, vk::resourceBufferPtrs2D& resourceDescriptors)
    {
        auto& descriptor = m_descriptorBuffers[category].descriptor;

        vk::WriteResource writeResource;

        for (int frame = 0; frame < resourceDescriptors.size(); ++frame)
        {
            for (int binding = 0; binding < resourceDescriptors[frame].size(); ++binding)
            {
                writeResource.pResourceData = resourceDescriptors[frame][binding];

                descriptor.WriteDescriptor(m_devicePtr, writeResource,
                    layoutIndex, frame, binding, m_properties.uniformBufferDescriptorSize);
            }
        }
    }

    uint32_t GetLayoutIndex(DescriptorCategory category)
    {
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

    VkDeviceSize GetLayoutSize(DescriptorCategory category)
    {
        if (m_descriptorBuffers.contains(category))
        {
            return m_descriptorBuffers[category].descriptor.GetLayoutSize();
        }

        return -1;
    }

    VkDescriptorSetLayout GetLayout(DescriptorCategory category)
    {
        if (m_descriptorBuffers.contains(category))
        {
            return m_descriptorBuffers[category].descriptor.GetLayout();
        }

        return VK_NULL_HANDLE;
    }

    VkDeviceAddress GetDescriptorAddress(DescriptorCategory category)
    {
        if (m_descriptorBuffers.contains(category)) {
            return m_descriptorBuffers[category].descriptor.GetBuffer().GetDeviceAddress();
        }

        std::cerr << "DescriptorManager::GetDescriptorAddress(), device address is invalid for requested category\n";

        return -1;
    }


private:
    std::mutex m_mutex;

    struct DescriptorBufferData
    {
        vk::DescriptorBuffer descriptor;
        std::vector<uint32_t> freeList;
    };

    std::unordered_map<DescriptorCategory, DescriptorBufferData> m_descriptorBuffers;

    vk::Device* m_devicePtr = nullptr;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT m_properties = {};


};