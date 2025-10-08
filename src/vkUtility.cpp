#include "Common.h"
#include "vkUtility.h"
#include <string>
#include <fstream>
#include "SpirvHelper.h"

namespace vk {

	namespace util 
	{

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

		bool FormatIsFilterable(const VkPhysicalDevice p_device, VkFormat format, VkImageTiling tiling) 
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
		void TransitionImageLayout(const VkDevice l_device, const VkCommandPool cmdPool, 
			const VkQueue& gfxQueue, 
			VkImage image, VkFormat format, 
			VkImageLayout oldLayout, VkImageLayout newLayout, 
			uint32_t mipLevels)
		{
			VkCommandBuffer cmdBuffer = beginSingleTimeCommand(l_device, cmdPool);

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //this might be the key to help sync 
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


			vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier); //asking the gpu to reconfigure the old image layout to the new layout.

			endSingleTimeCommand(l_device, cmdBuffer, cmdPool, gfxQueue);

		}

		//use command buffer
		void TransitionImageLayout(const VkDevice l_device, const VkCommandBuffer cmdBuffer,
			const VkQueue& gfxQueue,
			VkImage image, VkFormat format,
			VkImageLayout oldLayout, VkImageLayout newLayout,
			uint32_t mipLevels)
		{
			
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; //this might be the key to help sync 
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


			vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier); //asking the gpu to reconfigure the old image layout to the new layout.

		}


		//use command pool
		void copyBufferToImage(const VkDevice l_device, const VkCommandPool cmdPool, VkBuffer buffer, const VkQueue gfxQueue, VkImage image, uint32_t width, uint32_t height)
		{
			VkCommandBuffer cmdBuffer = beginSingleTimeCommand(l_device, cmdPool);

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

			endSingleTimeCommand(l_device, cmdBuffer, cmdPool, gfxQueue);
		}

		//use command buffer
		void copyBufferToImage(const VkDevice l_device, const VkCommandBuffer cmdBuffer, VkBuffer buffer, const VkQueue gfxQueue, VkImage image, uint32_t width, uint32_t height)
		{

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

			VkCommandBuffer cmdBuffer = beginSingleTimeCommand(l_device, cmdPool);

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

			endSingleTimeCommand(l_device, cmdBuffer, cmdPool, gfxQueue);

		}


		uint32_t CalculateMipLevels(const uint32_t& imageWidth, const uint32_t& imageHeight) 
		{
			return std::floor(std::log2(std::max(imageWidth, imageHeight))) + 1;

		}

		std::string ReadFile(const std::string& filename) 
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary); //when we initialize, we std::ios::ate points to the end.

			if (!file.is_open())
			{
				throw std::runtime_error("failed to open shader file!");
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

		void WriteSpirvFile(const char* filename, const std::vector<uint32_t>& data) 
		{
			std::ofstream output(filename,std::ios::out | std::ios::binary);

			if (!output.is_open()) {

				std::cerr << "could not write to file: " + std::string(filename) << "\n";
			}

			output.write((char*)(data.data()), data.size());

			output.close();
		}

		std::string ReadSourceAndWriteToSprv(std::string fileNamePath, shaderc_shader_kind shader_kind) 
		{

			std::cout << "WARNING: make sure to load .spv instead of .vert in release!\n";
			//vertex shader reading and compilation
			vk::shader::CompilationInfo shaderInfo = {};
			shaderInfo.source = vk::util::ReadFile(fileNamePath);
			shaderInfo.filename = fileNamePath.c_str();
			shaderInfo.kind = shader_kind;

			std::vector<uint32_t> output = vk::shader::SourceToSpv(shaderInfo);

			if (output.empty()) 
			{
				return std::string();
			}

			//WARNING: sloow!!!
			for (size_t i = 0; i < fileNamePath.size(); ++i)
			{
				if (fileNamePath[i] == '.')
				{
					size_t ext_size = fileNamePath.size() - i;

					fileNamePath.resize(fileNamePath.size() - ext_size);

					break;
				}
			}

			if (shader_kind == shaderc_vertex_shader) { fileNamePath += "vert"; }
			else if (shader_kind == shaderc_fragment_shader) { fileNamePath += "frag"; }
			else {
				std::cerr << "unsupported shader type: " << shader_kind << '\n';
				return std::string();
			}

			fileNamePath += ".spv";

			vk::util::WriteSpirvFile(fileNamePath.data(), output);

			return fileNamePath;
		}	
	}
}