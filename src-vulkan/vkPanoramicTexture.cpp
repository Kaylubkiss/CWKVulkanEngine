#include "vkPanoramicTexture.h"
#include "vkInit.h"

namespace vk
{
    void PanoramicTexture::Create( vk::Device* devicePtr,  const std::vector<vk::TextureCreateInfo>& createInfos, std::mutex& transferMutex )
    {
        int width, height, nChannels;
        float* pixels = stbi_loadf(createInfos[0].name.c_str(), &width, &height, &nChannels, 4);

        if (pixels == nullptr)
        {
            throw std::runtime_error("PanoramicTexture::Create() failed!\n");
        }

        m_width = width;
        m_height = height;

        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4 * sizeof(float);

        m_imageCount = createInfos.size();
        m_imageLayerSize = imageSize; //technically, this will be transformed into a cubemap, so the image layer size will be divided by 6.

        vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, imageSize, pixels);

        VkImageCreateInfo panoramicImageCI = vk::init::ImageCreateInfo();
        panoramicImageCI.imageType = VK_IMAGE_TYPE_2D;
        panoramicImageCI.extent = { m_width, m_height, 1 };
        panoramicImageCI.arrayLayers = 1;
        panoramicImageCI.mipLevels = 1;
        panoramicImageCI.format = createInfos[0].format;
        panoramicImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        panoramicImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        panoramicImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        panoramicImageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        m_image = vk::init::CreateImage(devicePtr, panoramicImageCI, m_memory,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        m_descriptor.imageView = vk::Texture::CreateImageView(devicePtr->GetDevice(), m_image,
            createInfos[0].format, VK_IMAGE_VIEW_TYPE_2D);

        m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //this must be respected by the time its accessed in the compute shader

        m_descriptor.sampler = vk::Texture::CreateSampler(devicePtr->GetGPU(), devicePtr->GetDevice());

        m_imageView = m_descriptor.imageView;


        TransitionImageLayoutAndWriteToCubeMap( devicePtr, stagingBuffer, transferMutex );

        std::cout << "\033[32m" << "successfully loaded Panormaic Texture in PanoramicTexture::Create()... " << "\033[0m\n";

        stbi_image_free(pixels);
        stagingBuffer.Destroy();


        c_device = devicePtr->GetDevice();
    }
}