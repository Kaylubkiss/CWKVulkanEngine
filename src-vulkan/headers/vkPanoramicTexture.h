#pragma once
#include "vkTexture.h"

namespace vk
{
    class PanoramicTexture : public Texture //technically a type of cubemap, but inheriting cubemap would be useless.
    {
    public:
        void Create( const vk::Device* devicePtr, const std::vector<std::string>& fileNames, std::mutex& transferMutex ) override
        {
            int width, height, nChannels;
            float* pixels = stbi_loadf(fileNames[0].c_str(), &width, &height, &nChannels, 4);

            if (pixels == nullptr)
            {
                throw std::runtime_error("PanoramicTexture::Create() failed!\n");
            }

            m_width = width;
            m_height = height;

            VkFence submissionFence = vk::init::CreateFence(devicePtr->GetDevice(), false);

            VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4 * sizeof(float);

            vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, imageSize, pixels);

            VkImageCreateInfo panoramicImageCI = vk::init::ImageCreateInfo();
            panoramicImageCI.imageType = VK_IMAGE_TYPE_2D;
            panoramicImageCI.extent = { m_width, m_height, 1 };
            panoramicImageCI.arrayLayers = 1;
            panoramicImageCI.mipLevels = 1;
            panoramicImageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
            panoramicImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            panoramicImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            panoramicImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            panoramicImageCI.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            m_image = vk::init::CreateImage(devicePtr, panoramicImageCI, m_memory,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            RecordTransferOperations(devicePtr, stagingBuffer, transferMutex);

            VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
            imageCI.extent = { 2048, 2048, 1 };
            imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.arrayLayers = 6;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.mipLevels = 1;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


            VkImage cubemapImage = vk::init::CreateImage(devicePtr, imageCI, m_memory,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);





            std::cout << "\033[32m" << "successfully loaded Panormaic Texture in PanoramicTexture::Create()... " << "\033[0m\n";

            stbi_image_free(pixels);
            stagingBuffer.Destroy();

            c_device = devicePtr->GetDevice();
        }
    };


}