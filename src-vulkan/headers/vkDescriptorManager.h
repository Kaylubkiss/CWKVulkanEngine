#ifndef DESCRIPTOR_MANAGER_HPP
#define DESCRIPTOR_MANAGER_HPP

#include "vkDescriptorBuffer.h"

enum class DescriptorCategory
{
    eUBO, //set 0: ubo
    eCompositionImage, //set 1
    eMaterial, //set 2
    eObject, //set 3: model transforms, animation state. Unused for now.
};

namespace vk
{
    class DescriptorManager
    {
    public:
        DescriptorManager() = default;
        ~DescriptorManager() = default;

        void Init( vk::Device* devicePtr );

        void Destroy();

        void AllocateDescriptorBuffer( DescriptorCategory category, size_t numFrames, size_t layoutCount,
            const std::vector<VkDescriptorSetLayoutBinding>& bindings );

        // [frame][binding]
        void WriteDescriptors( DescriptorCategory category, uint32_t layoutIndex,
            vk::imageBuffers2D& imageDescriptors, bool storageResource = false, int baseBinding = 0 );

        void WriteDescriptors( DescriptorCategory category, uint32_t layoutIndex,
            vk::resourceBufferPtrs2D& resourceDescriptors );

        uint32_t GetLayoutIndex( DescriptorCategory category );

        VkDeviceSize GetLayoutSize( DescriptorCategory category );

        VkDescriptorSetLayout GetLayout( DescriptorCategory category );

        [[nodiscard]] VkDeviceAddress GetDescriptorAddress( DescriptorCategory category );


    private:
        std::shared_mutex m_mutex;

        struct DescriptorBufferData
        {
            vk::DescriptorBuffer descriptor;
            std::vector<uint32_t> freeList;
        };

        std::unordered_map<DescriptorCategory, DescriptorBufferData> m_descriptorBuffers;

        vk::Device* m_devicePtr = nullptr;
        VkPhysicalDeviceDescriptorBufferPropertiesEXT m_properties = {};
    };
}

#endif