#include "SpirvHelper.h"
#include <fstream>

PFN_vkGetDescriptorSetLayoutBindingOffsetEXT g_vkGetDescriptorSetLayoutBindingOffsetEXT = nullptr;
PFN_vkGetDescriptorSetLayoutSizeEXT g_vkGetDescriptorSetLayoutSizeEXT = nullptr;
PFN_vkGetDescriptorEXT g_vkGetDescriptorEXT = nullptr;
PFN_vkCmdBindDescriptorBuffersEXT g_vkCmdBindDescriptorBuffersEXT = nullptr;
PFN_vkCmdSetDescriptorBufferOffsetsEXT g_vkCmdSetDescriptorBufferOffsetsEXT = nullptr;

namespace vk {

	namespace util 
	{
		VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) 
		{
			//NOTE: doing this to ignore an overlay bug with OBS/Steam
			/*if (pCallbackData->messageIdNumber == 0x9b4c6071)
			{
				return VK_FALSE;
			}*/

			std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

		VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo()
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;

			return createInfo;
		}

		VkFormat findSupportedFormat(const VkPhysicalDevice p_device, const std::vector<VkFormat>& possibleFormats,
			VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (VkFormat format : possibleFormats)
			{
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(p_device, format, &properties);

				if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features))
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features))
				{
					return format;
				}

			}

			throw std::runtime_error("couldn't find a suitable format supported on the physical device.");
		}

		bool FormatIsSupported(const VkPhysicalDevice p_device, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(p_device, format, &properties);

			if (tiling == VK_IMAGE_TILING_LINEAR)
			{
				return (properties.linearTilingFeatures & features);
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL)
			{
				return (properties.optimalTilingFeatures & features);
			}

			throw std::runtime_error("specified VkFormat " + std::to_string(format) + " is not supported on the physical device.");
		}


		bool FormatIsFilterable( const VkPhysicalDevice p_device, VkFormat format, VkImageTiling tiling )
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(p_device, format, &formatProperties);

			if (tiling == VK_IMAGE_TILING_OPTIMAL) 
			{
				return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
			}
			else if (tiling == VK_IMAGE_TILING_LINEAR) {

				return formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
			}

			return false;
		}
		
		//use command pool
		void RecordImageLayoutTransition( VkCommandBuffer cmdBuffer, VkImage image,
			uint32_t srcQueue, uint32_t dstQueue,
			VkImageLayout oldLayout, VkImageLayout newLayout )
		{

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = srcQueue;
			barrier.dstQueueFamilyIndex = dstQueue;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			VkPipelineStageFlags srcStage = 0;
			VkPipelineStageFlags dstStage = 0;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				(newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
			{
				barrier.srcAccessMask = 0;
				if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
				{
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				}
				else
				{
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				}

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				newLayout == VK_IMAGE_LAYOUT_GENERAL)
			{
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
				(newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
			{
				//note: automatically assumes that general layout was for writing.
				barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
				{
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				}
				else
				{
					barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				}

				//NOTE: also assume that the general layout was for writing to an image in a compute shader
				srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
				newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
				oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) &&
				newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				else
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				}
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else
			{
				throw std::invalid_argument("bad layout transition");
			}

			vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0,
				nullptr, 0, nullptr,
				1, &barrier); //asking the gpu to reconfigure the old image layout to the new layout.
		}

		void SubmitCommandToQueue( VkDevice device, VkCommandBuffer cmdBuffer, VkQueue queue,
			VkFence fence, std::optional<std::mutex> submissionMutex )
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			if (submissionMutex.has_value())
			{
				std::lock_guard lock(submissionMutex.value());

				VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo,
					fence));
			}
			else
			{
				VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo,
					fence));
			}

			if (fence != VK_NULL_HANDLE)
			{
				VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
				VK_CHECK_RESULT(vkResetFences(device, 1, &fence));
			}

		}

		void RecordBlitMipMapImages( VkCommandBuffer cmdBuffer, VkImage image,
			uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels, uint32_t layerCount )
		{

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.layerCount = layerCount;
			barrier.subresourceRange.levelCount = 1; //only 1 mip level will be transitioned.

			auto mipWidth = static_cast<int32_t>(textureWidth);
			auto mipHeight = static_cast<int32_t>(textureHeight);

			for (uint32_t i = 1; i < mipLevels; i++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
					nullptr, 0,
					nullptr, 1, &barrier);

				VkImageBlit blit = {};
				blit.srcOffsets[0] = { 0,0,0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = layerCount;

				blit.dstOffsets[0] = { 0,0,0 };
				blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = layerCount;

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

				vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &barrier);


				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;

			}

			/*barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &barrier);*/

		}

		uint32_t CalculateMipLevels( uint32_t imageWidth, uint32_t imageHeight )
		{
			return static_cast<uint32_t>(std::floor(std::log2(std::max(imageWidth, imageHeight))) + 1);
		}

		std::optional<std::string> ReadFile( const std::string& filename )
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary); //when we initialize, we std::ios::ate points to the end.

			if (!file.is_open())
			{
				std::cerr << "failed to open " + filename << std::endl;
				return std::nullopt;
			}

			//reads the offset from the beginning of the file
			size_t fileSize = (size_t)file.tellg();

			std::vector<char> buffer (fileSize);

			//set the stream to the beginning of the file after being positioned at the end.
			file.seekg(0);

			file.read(buffer.data(), fileSize);

			file.close();

			std::string src_string(buffer.data(), fileSize);

			return src_string;
		}

		bool CheckInstanceLayerSupport( const char* layers[], int layersSize )
		{
			uint32_t layerCount = 0;
    		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (int i = 0; i < layersSize; ++i)
			{
				bool foundLayer = false;
				for (int j = 0; j < availableLayers.size(); ++j)
				{
					if (strcmp(layers[i], availableLayers[j].layerName) == 0)
					{
						foundLayer = true;
						break;
					}
				}

				if (foundLayer == false) 
				{
					return false;
				}
			}

			return true;
		}

		bool CheckInstanceExtensionSupport( const char* enabled_extensions[], int enabled_extension_count )
		{
			uint32_t instance_extension_count = 0;
			std::vector<VkExtensionProperties> instance_extensions;
			
			vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, nullptr);			
			instance_extensions.resize(instance_extension_count);

			vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count, instance_extensions.data());

			for (int i = 0; i < enabled_extension_count; ++i)
			{
				bool foundExtension = false;
				for (auto& extension : instance_extensions) 
				{
					if (strcmp(enabled_extensions[i], extension.extensionName) == 0) 
					{
						foundExtension = true;
						break;
					}
				}

				if (!foundExtension) 
				{
					return false;
				}
			}

			return true;

		}

		VkCommandBuffer beginSingleTimeCommand(const VkDevice l_device, const VkCommandPool cmdPool)
		{

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = cmdPool;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer cmdBuffer;
			VK_CHECK_RESULT(vkAllocateCommandBuffers(l_device, &allocInfo, &cmdBuffer));

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

			return cmdBuffer;

		}

		void endSingleTimeCommand(const VkDevice l_device, VkCommandBuffer commandBuffer, const VkCommandPool cmdPool, const VkQueue gfxQueue)
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VK_CHECK_RESULT(vkQueueSubmit(gfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
			VK_CHECK_RESULT(vkQueueWaitIdle(gfxQueue));

			vkFreeCommandBuffers(l_device, cmdPool, 1, &commandBuffer);
		}
	}
}