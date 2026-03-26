#pragma once

//NOTE: entire implementation is WIP
#include <stb_image.h> //THIS SHOULD NOT BE SPECIFIED HERE BUT THERE ARE SYMBOL ERRS
#include "vkTexture.h"

constexpr const char* CUBEMAP_DIR = "art/extern-textures/cubemaps/";

namespace vk
{
    class Cubemap : public Texture
    {
    public:
        Cubemap() = default;
        ~Cubemap() override = default;

        //just one particular environment map for now
        void Create( const vk::Device* devicePtr, const std::string& fileName, std::mutex& transferMutex ) override
        {
            (void)(transferMutex);
            (void)(fileName);

            constexpr int image_count = 6;
            //texture sizes should be square and/or the same in a cubemap
            int image_width, image_height, channels;

            std::array<stbi_uc*, image_count> texture_data = {};
            std::array<std::string, image_count> file_names = {
                "IceRiver/posx.jpg", //right (+X)
                "IceRiver/negx.jpg", //left (-X)
                "IceRiver/posy.jpg", //up (+Y)
                "IceRiver/negy.jpg", //down (-Y)
                "IceRiver/posz.jpg", //forward (+Z)
                "IceRiver/negz.jpg", //back (-Z)
            };

            for (size_t i = 0; i < image_count; ++i)
            {
                texture_data[i] = stbi_load((CUBEMAP_DIR + file_names[i]).c_str(),
                    &image_width, &image_height, &channels, STBI_rgb_alpha);

                if (texture_data[i] == nullptr)
                {
                    std::cerr << "failed to load Cubemap image  " + file_names[i] << "\n";
                    throw std::runtime_error("Cubemap::CreateImage() failed!\n");
                }
            }

            const VkDeviceSize image_size = image_width * image_height *
                4 * image_count;
            const VkDeviceSize layer_size = image_size / static_cast<VkDeviceSize>(image_count);

            vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, image_size);

            stagingBuffer.Map();
            void* stagingBufferData = stagingBuffer.GetMappedMemory();
            for (uint8_t i = 0; i < image_count; ++i)
            {
                memcpy(static_cast<stbi_uc*>(stagingBufferData) + (layer_size * i), texture_data[i], layer_size);
            }

            stagingBuffer.Flush();
            stagingBuffer.UnMap();

            VkImageCreateInfo imageCI = vk::init::ImageCreateInfo();
            imageCI.extent = { static_cast<uint32_t>(image_width), static_cast<uint32_t>(image_height), 1 };
            imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
            imageCI.imageType = VK_IMAGE_TYPE_2D;
            imageCI.arrayLayers = image_count;
            imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCI.mipLevels = 1;
            imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            VK_CHECK_RESULT(vkCreateImage(devicePtr->GetDevice(), &imageCI, nullptr, &m_image));

            VkMemoryAllocateInfo memoryAllocInfo = vk::init::MemoryAllocateInfo();
            VkMemoryRequirements memRequirements = {};
            vkGetImageMemoryRequirements(devicePtr->GetDevice(), m_image, &memRequirements);

            memoryAllocInfo.allocationSize = memRequirements.size;
            memoryAllocInfo.memoryTypeIndex = devicePtr->GetMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VK_CHECK_RESULT(vkAllocateMemory(devicePtr->GetDevice(), &memoryAllocInfo, nullptr, &m_memory));
            VK_CHECK_RESULT(vkBindImageMemory(devicePtr->GetDevice(), m_image, m_memory, 0));

            VkCommandPool transferCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::TRANSFER).family);

            VkCommandBuffer transferCmd = VK_NULL_HANDLE;

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = transferCmdPool;
            allocInfo.commandBufferCount = 1;
            VK_CHECK_RESULT(vkAllocateCommandBuffers(devicePtr->GetDevice(), &allocInfo, &transferCmd));

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            //transition image to dst-optimal layout so the staging buffer can be copied into it.
            {
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = m_image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

                vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0,
                    nullptr, 0, nullptr, 1,
                    &barrier); //asking the gpu to reconfigure the old image layout to the new layout.

                VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

                {
                    VkSubmitInfo submitInfo = {};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &transferCmd;

                    //std::lock_guard<std::mutex> lock(transferMutex);
                    VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
                        VK_NULL_HANDLE));

                    VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle));
                }
            }

            //copy the buffer image data to the image.
            {
                VkDeviceSize bufferCopyAlignment = devicePtr->GetProperties().limits.optimalBufferCopyOffsetAlignment;
                VkDeviceSize alignedLayerSize = (layer_size + bufferCopyAlignment - 1) & ~(bufferCopyAlignment - 1);

                std::array<VkBufferImageCopy, image_count> regions = {};
                for (int i = 0; i < image_count; ++i)
                {
                    regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    regions[i].imageSubresource.mipLevel = 0;
                    regions[i].imageSubresource.baseArrayLayer = i;
                    regions[i].imageSubresource.layerCount = 1;
                    regions[i].bufferOffset = alignedLayerSize * i;
                    regions[i].imageExtent =
                    {
                        static_cast<uint32_t>(image_width),
                        static_cast<uint32_t>(image_height),
                        1
                    };
                }

                VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

                vkCmdCopyBufferToImage(transferCmd, stagingBuffer.GetHandle(), m_image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());

                VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

                {
                    VkSubmitInfo submitInfo = {};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &transferCmd;

                    // std::lock_guard<std::mutex> lock(transferMutex);
                    VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle,
                        1, &submitInfo, VK_NULL_HANDLE));

                    VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle));
                }
            }

            //release the resource to the graphics queue.
            {
                VkImageMemoryBarrier releaseBarrier = {};
                releaseBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                releaseBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                releaseBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                releaseBarrier.srcQueueFamilyIndex = devicePtr->GetQueue(DeviceQueue::TRANSFER).family;
                releaseBarrier.dstQueueFamilyIndex = devicePtr->GetQueue(DeviceQueue::GRAPHICS).family;
                releaseBarrier.image = m_image;
                releaseBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                releaseBarrier.subresourceRange.baseMipLevel = 0;
                releaseBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                releaseBarrier.subresourceRange.baseArrayLayer = 0;
                releaseBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                releaseBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

                vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    0, 0,
                    nullptr, 0, nullptr, 1,
                    &releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.

                VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &transferCmd;

                //std::lock_guard<std::mutex> lock(transferMutex);
                VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
                    VK_NULL_HANDLE));

                VK_CHECK_RESULT(vkQueueWaitIdle(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle));
            }

            VkSamplerCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            createInfo.addressModeV = createInfo.addressModeU;
            createInfo.addressModeW = createInfo.addressModeU;

            createInfo.maxAnisotropy = devicePtr->GetProperties().limits.maxSamplerAnisotropy / 2.f;
            createInfo.anisotropyEnable = VK_TRUE;

            createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            createInfo.unnormalizedCoordinates = VK_FALSE;

            createInfo.compareEnable = VK_FALSE;
            createInfo.compareOp = VK_COMPARE_OP_NEVER;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            createInfo.minLod = 0.f;
            createInfo.maxLod = static_cast<float>(1);
            createInfo.mipLodBias = 0.f; //optional...

            VK_CHECK_RESULT(vkCreateSampler(devicePtr->GetDevice(), &createInfo, nullptr, &m_sampler));

            VkImageViewCreateInfo imageViewCI = vk::init::ImageViewCreateInfo();
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            imageViewCI.format = imageCI.format;
            imageViewCI.components = vk::init::ComponentMappingSwizzleIdentity();
            imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCI.subresourceRange.baseMipLevel = 0;
            imageViewCI.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            imageViewCI.subresourceRange.baseArrayLayer = 0;
            imageViewCI.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
            imageViewCI.image = m_image;

            VK_CHECK_RESULT(vkCreateImageView(devicePtr->GetDevice(), &imageViewCI, nullptr, &m_imageView));

            //deallocate resources
            vkFreeCommandBuffers(devicePtr->GetDevice(), transferCmdPool, 1, &transferCmd);
            vkDestroyCommandPool(devicePtr->GetDevice(), transferCmdPool, nullptr);

            for (size_t i = 0; i < image_count; ++i)
            {
                stbi_image_free(texture_data[i]);
            }
            stagingBuffer.Destroy();

            c_device = devicePtr->GetDevice();
            m_descriptor.imageView = m_imageView;
            m_descriptor.sampler = m_sampler;
            m_descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            std::cout << "\033[32m" << "successfully loaded Cubemap in CreateImage()... " << "\033[0m\n";
        }
    };


}