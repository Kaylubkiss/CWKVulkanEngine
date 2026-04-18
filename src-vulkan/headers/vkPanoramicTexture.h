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

            uint32_t mipLevels = 1;

            VkFence submissionFence = vk::init::CreateFence(devicePtr->GetDevice(), false);

            VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4 * sizeof(float);

            vk::Buffer stagingBuffer = vk::Buffer(devicePtr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, imageSize, pixels);


            m_image = vk::init::CreateImage(devicePtr->GetGPU(), devicePtr->GetDevice(), width, height, mipLevels,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_memory);

            VkCommandPool transferCmdPool = vk::init::CommandPool(devicePtr->GetDevice(),
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, devicePtr->GetQueue(DeviceQueue::TRANSFER).family);

            VkCommandBuffer transferCmd = beginSingleTimeCommand(devicePtr->GetDevice(), transferCmdPool);
            VkSubmitInfo submitInfo = {};

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

                vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, 0,
                    nullptr, 0, nullptr, 1,
                    &barrier); //asking the gpu to reconfigure the old image layout to the new layout.

                VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

                submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &transferCmd;

                {
                    std::lock_guard<std::mutex> lock(transferMutex);
                    VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
                        submissionFence));
                }

                VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
                VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));
            }


            //copy buffer into image.
            {
                VkBufferImageCopy region = {};
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel = 0;
                region.imageSubresource.baseArrayLayer = 0;
                region.imageSubresource.layerCount = 1;

                region.imageOffset = { 0,0,0 };
                region.imageExtent =
                {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height),
                    1
                };

                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

                vkCmdCopyBufferToImage(transferCmd, stagingBuffer.GetHandle(), m_image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

                submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &transferCmd;

                {
                    std::lock_guard<std::mutex> lock(transferMutex);
                    VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle,
                        1, &submitInfo, submissionFence));
                }

                VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
                VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));
            }

            //release transfer queue to graphics queue
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
				releaseBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; //since we just wrote to the image.
				releaseBarrier.dstAccessMask = 0;

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				VK_CHECK_RESULT(vkBeginCommandBuffer(transferCmd, &beginInfo));

				vkCmdPipelineBarrier(transferCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					0, 0,
					nullptr, 0, nullptr, 1,
					&releaseBarrier); //asking the gpu to reconfigure the old image layout to the new layout.

				VK_CHECK_RESULT(vkEndCommandBuffer(transferCmd));

				submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &transferCmd;

				{
					std::lock_guard<std::mutex> lock(transferMutex);
					VK_CHECK_RESULT(vkQueueSubmit(devicePtr->GetQueue(DeviceQueue::TRANSFER).handle, 1, &submitInfo,
						submissionFence));
				}

				VK_CHECK_RESULT(vkWaitForFences(devicePtr->GetDevice(), 1, &submissionFence, VK_TRUE, UINT64_MAX));
				VK_CHECK_RESULT(vkResetFences(devicePtr->GetDevice(), 1, &submissionFence));
			}



            std::cout << "\033[32m" << "successfully loaded Panormaic Texture in PanoramicTexture::Create()... " << "\033[0m\n";

            stbi_image_free(pixels);
            stagingBuffer.Destroy();

            vkFreeCommandBuffers(devicePtr->GetDevice(), transferCmdPool, 1, &transferCmd);
            vkDestroyCommandPool(devicePtr->GetDevice(), transferCmdPool, nullptr);



            c_device = devicePtr->GetDevice();
        }
    };


}