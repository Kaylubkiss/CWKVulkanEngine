#pragma once


//each of these has its own descriptor set layout
//when allocating descriptor buffer for uniforms, include the
enum class DescriptorCategory
{
    eStatic, //set 0 --> skybox texture, default texture
    ePerPass, //set 1 --> MRT, uniforms, etc.
    eMaterial //set 2 --> per-object textures
};

class DescriptorManager
{
public:
    DescriptorManager() = default;
    ~DescriptorManager() = default;

    void Init( vk::Device* devicePtr )
    {
        m_devicePtr = devicePtr;
        m_properties = m_devicePtr->GetDescriptorBufferProperties();
    }

    void WriteToDescriptorBuffer( DescriptorCategory category, VkDescriptorAddressInfoEXT addressInfo, size_t slot, size_t binding )
    {
        VkDescriptorGetInfoEXT bufferDescriptorInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
        bufferDescriptorInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bufferDescriptorInfo.data.pUniformBuffer = &addressInfo;

        /*g_vkGetDescriptorEXT(sharedDevicePtr->GetDevice(), &bufferDescriptorInfo,
                  m_properties.uniformBufferDescriptorSize,
                  descriptor_ptr + frame * m_setLayoutSize + m_bindingOffsets[binding]);*/

    }

    //need to write to the offset
    //mrt
    //
    void WriteToDescriptorBuffer( DescriptorCategory category, VkDescriptorImageInfo imageInfo, size_t slot, size_t binding )
    {
        VkDescriptorGetInfoEXT imageDescriptorGetInfo = {};
        imageDescriptorGetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
        imageDescriptorGetInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageDescriptorGetInfo.data.pCombinedImageSampler = &imageInfo;

        auto& descriptor = m_descriptorBuffers[category].descriptor;
        auto& descriptorBuffer = descriptor.GetBuffer();
        void* descriptorData = descriptorBuffer.GetMappedMemory();

        /*g_vkGetDescriptorEXT(m_devicePtr->GetDevice(), &imageDescriptorGetInfo,
                  m_properties.combinedImageSamplerDescriptorSize,
                  descriptorData + slot * m_setLayoutSize + m_bindingOffsets[binding]);*/



    }
    void AllocateDescriptorBuffer( DescriptorCategory category, size_t layoutCount, const std::vector<VkDescriptorSetLayoutBinding>& setBindings )
    {
        std::lock_guard lock(m_mutex);
        //VkMemoryFlag
        VkMemoryPropertyFlags bufferMemoryFlags = 0;
        //NOTE: it is technically slower to include both sampler and resource descriptors. This is temporary.
        VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
            VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        switch (category)
        {
            case DescriptorCategory::eStatic:
            case DescriptorCategory::ePerPass:
            case DescriptorCategory::eMaterial:
                bufferMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            default:
                std::cout << "AllocateDescriptorBuffer(): this should not be possible.\n";
                return;
        }
        m_descriptorBuffers[category].descriptor.AllocateSetLayout(m_devicePtr, setBindings);
        m_descriptorBuffers[category].descriptor.AllocateBuffer(m_devicePtr, bufferUsageFlags, bufferMemoryFlags, layoutCount);
        m_descriptorBuffers[category].layoutSize = m_descriptorBuffers[category].descriptor.GetLayoutSize();
    }

    //which layout within the descriptor buffer does this fit into?
    size_t AllocateMaterialLayoutSlot()
    {
        std::lock_guard lock(m_mutex);
        auto& data = m_descriptorBuffers[DescriptorCategory::eMaterial];
        size_t index = 0;
        if (data.availableSlots.empty())
        {
            std::cerr << "descriptor pool size has been filled! Consider allocating bigger size.\n";
            throw std::runtime_error("DescriptorManager::AllocateLayoutSlot() Failed!\n");
        }
        index = data.availableSlots.back();
        data.availableSlots.pop_back();

        return index;
    }
private:
    std::mutex m_mutex;

    struct DescriptorBufferData
    {
        vk::DescriptorBuffer descriptor;
        std::vector<size_t> availableSlots;

        size_t layoutSize = 0;

    };
    std::unordered_map<DescriptorCategory, DescriptorBufferData> m_descriptorBuffers;

    VkPhysicalDeviceDescriptorBufferPropertiesEXT m_properties;

    vk::Device* m_devicePtr = nullptr;
};