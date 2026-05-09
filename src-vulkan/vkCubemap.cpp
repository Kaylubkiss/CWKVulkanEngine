#include "vkCubemap.h"
#include "vkInit.h"

namespace vk
{
    //just one particular environment map for now
    void Cubemap::Create( const vk::Device* devicePtr, const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex )
    {
        assert(devicePtr);

        m_imageCount = createInfos.size();

        assert(m_imageCount== 6);

        //texture sizes should be square and/or the same in a cubemap
        int image_width, image_height, channels;

        std::vector<stbi_uc*> texture_data(m_imageCount, nullptr);

        VkFormat format = createInfos[0].format;

        for (size_t i = 0; i < m_imageCount; ++i)
        {
            texture_data[i] = stbi_load((CUBEMAP_DIR + createInfos[i].name).c_str(),
                &image_width, &image_height, &channels, STBI_rgb_alpha);

            if (texture_data[i] == nullptr)
            {
                std::cerr << "failed to load Cubemap image  " + createInfos[i].name << "\n";
                throw std::runtime_error("Cubemap::CreateImage() failed!\n");
            }
        }

        const VkDeviceSize image_size = image_width * image_height *
            4 * m_imageCount;

        m_imageLayerSize = image_size / static_cast<VkDeviceSize>(m_imageCount);

        m_width = image_width;
        m_height = image_height;

        vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, image_size);

        m_imageLayerSize = AlignedSize(m_imageLayerSize, devicePtr->GetProperties().limits.optimalBufferCopyOffsetAlignment);

        stagingBuffer.Map();
        void* stagingBufferData = stagingBuffer.GetMappedMemory();
        for (VkDeviceSize i = 0; i < m_imageCount; ++i)
        {
            memcpy(static_cast<stbi_uc*>(stagingBufferData) + (m_imageLayerSize * i), texture_data[i], m_imageLayerSize);
        }
        stagingBuffer.Flush();
        stagingBuffer.UnMap();

        VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
        imageCI.extent = { m_width, m_height, 1 };
        imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCI.format = format;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.arrayLayers = static_cast<uint32_t>(m_imageCount);
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.mipLevels = 1;
        imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        m_image = vk::init::CreateImage(devicePtr, imageCI, m_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        RecordTransferAndReleaseOperations(devicePtr, stagingBuffer, transferMutex);

        for (size_t i = 0; i < m_imageCount; ++i)
        {
            stbi_image_free(texture_data[i]);
        }
        stagingBuffer.Destroy();

        c_device = devicePtr->GetDevice();
        m_imageView = vk::Texture::CreateImageView(c_device, m_image, createInfos[0].format, VK_IMAGE_VIEW_TYPE_CUBE);
        m_sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), c_device );
        m_descriptor.imageView = m_imageView;
        m_descriptor.sampler = m_sampler;
        m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::cout << "\033[32m" << "successfully loaded Cubemap in CreateImage()... " << "\033[0m\n";
    }
}