#pragma once

#include "vkDescriptorBuffer.h"

enum class DescriptorCategory
{
    eStatic,
    ePerPassUBO,
    ePerPassImages,
    eMaterial
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

    void AllocateDescriptorBuffer(DescriptorCategory category, size_t layoutCount, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
    {
        //create the layout for the descriptor buffer.
        //allocate the buffer for the descriptor buffer.
    }

    // [frame][binding]
    void WriteDescriptor(DescriptorCategory category, uint32_t layoutIndex, uint32_t setIndex, const vk::imageBuffers2D& imageDescriptors)
    {
        auto& descriptor = m_descriptorBuffers[category].descriptor;

        descriptor.WriteDescriptors(m_devicePtr, layoutIndex, setIndex, m_properties.combinedImageSamplerDescriptorSize,
            imageDescriptors);
    }

    void WriteDescriptor(DescriptorCategory category, uint32_t layoutIndex, uint32_t setIndex, const vk::resourceBufferPtrs2D& resourceDescriptors)
    {
        auto& descriptor = m_descriptorBuffers[category].descriptor;

        descriptor.WriteDescriptors(m_devicePtr, layoutIndex, setIndex, m_properties.uniformBufferDescriptorSize,
            resourceDescriptors);
    }
private:

    struct DescriptorBufferData
    {
        vk::DescriptorBuffer descriptor;
        std::vector<size_t> freeList;
    };

    std::unordered_map<DescriptorCategory, DescriptorBufferData> m_descriptorBuffers;

    vk::Device* m_devicePtr = nullptr;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT m_properties;


};