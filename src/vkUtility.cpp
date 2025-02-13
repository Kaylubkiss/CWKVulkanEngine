#include "Common.h"
#include "vkUtility.h"


namespace vk {

	namespace util 
	{
		VkPipelineShaderStageCreateInfo CreateShaderModule(VkDevice logicalDevice, const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage)
		{
			std::ifstream file(name, std::ios::ate | std::ios::binary); //when we initialize, we std::ios::ate points to the end.

			if (!file.is_open())
			{
				throw std::runtime_error("failed to open shader file!");
			}

			char* buffer = nullptr;


			//reads the offset from the beginning of the file
			size_t fileSize = (size_t)file.tellg();

			buffer = new char[fileSize];

			//set the stream to the beginning of the file after being positioned at the end.
			file.seekg(0);

			file.read(buffer, fileSize);

			file.close();


			VkShaderModuleCreateInfo shaderVertModuleInfo =
			{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				fileSize,
				reinterpret_cast<const uint32_t*>(buffer)
			};

			VK_CHECK_RESULT(vkCreateShaderModule(logicalDevice, &shaderVertModuleInfo, nullptr, &shaderModule));

			VkPipelineShaderStageCreateInfo shaderStageInfo =
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				nullptr,
				0,
				stage,
				shaderModule,
				"main", //entry point -->pName
				nullptr //no specialization constants
			};

			delete[] buffer;

			return shaderStageInfo;

		}

		bool CheckValidationSupport()
		{
			uint32_t layerCount = 0;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);

			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


			for (const char* layerName : vulkan_engine::enabledLayerNames)
			{
				bool layerFound = false;
				for (const VkLayerProperties& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						layerFound = true;
						break;
					}
				}

				if (layerFound == false)
				{
					return false;
				}

			}


			return true;

		}

		VkFormat findSupportedFormat(const VkPhysicalDevice p_device, const std::vector<VkFormat>& possibleFormats,
			VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : possibleFormats)
			{
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(p_device, format, &properties);

				if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
				{
					return format;
				}

			}

			throw std::runtime_error("couldn't find a suitable format supported on the physical device.");
		}

		void TransitionImageLayout(const VkDevice l_device, const VkCommandPool cmdPool, const VkQueue& gfxQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
		{
			VkCommandBuffer cmdBuffer = beginCmd(l_device, cmdPool);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = mipLevels;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags srcStage = 0;
			VkPipelineStageFlags dstStage = 0;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else
			{
				throw std::invalid_argument("bad layout transition");
			}

			//first two parameters 
			vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			endCmd(l_device, cmdBuffer, cmdPool, gfxQueue);

		}

		void copyBufferToImage(const VkDevice l_device, const VkCommandPool cmdPool, VkBuffer buffer, const VkQueue gfxQueue, VkImage image, uint32_t width, uint32_t height)
		{
			VkCommandBuffer cmdBuffer = beginCmd(l_device, cmdPool);

			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0,0,0 };
			region.imageExtent =
			{
				width,
				height,
				1
			};

			vkCmdCopyBufferToImage(cmdBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			endCmd(l_device, cmdBuffer, cmdPool, gfxQueue);
		}

		void GenerateMipMaps(const VkPhysicalDevice p_device, const VkDevice l_device, const VkCommandPool& cmdPool, const VkQueue gfxQueue, VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels)
		{

			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(p_device, imgFormat, &formatProperties);

			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			{
				throw std::runtime_error("your physical device does not support linear blitting");
				//TODO: generate mipmap levels with software/storing mip levels in texture image and sampling that.
			}

			VkCommandBuffer cmdBuffer = beginCmd(l_device, cmdPool);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			uint32_t mipWidth = textureWidth;
			uint32_t mipHeight = textureHeight;

			for (uint32_t i = 1; i < mipLevels; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
					nullptr, 0, nullptr, 1, &barrier);

				VkImageBlit blit = {};
				blit.srcOffsets[0] = { 0,0,0 };
				blit.srcOffsets[1] = { (int)(mipWidth), (int)(mipHeight), 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;

				blit.dstOffsets[0] = { 0,0,0 };
				blit.dstOffsets[1] = { (int)(mipWidth > 1 ? mipWidth / 2 : 1), (int)(mipHeight > 1 ? mipHeight / 2 : 1), 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(cmdBuffer,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&blit, VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &barrier);


				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;

			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);

			endCmd(l_device, cmdBuffer, cmdPool, gfxQueue);

		}
	}
}