#pragma once

#include "Common.h"
#include <array>
#include <cassert>
#include <iostream>
#include <cmath>
#include <string>

#define VK_CHECK_RESULT(function) {VkResult check = function; assert(check == VK_SUCCESS); if (check != VK_SUCCESS) {std::cout << check << '\n';}}


namespace vk 
{
	namespace util 
	{
		static void check_vk_result(VkResult err)
		{
			if (err == 0)
				return;
			//fun fact: fprintf will return the length of the string it outputted..
			//https://stackoverflow.com/questions/29931016/return-value-of-fprintf
			int result = fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);

			if (err < 0)
				abort();
		}

		bool CheckValidationSupport();

		/* IDEA:
		VkPipelineShaderStageCreateInfo PipelineStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);*/

		VkFormat findSupportedFormat(const VkPhysicalDevice p_device, const std::vector<VkFormat>& possibleFormats,
			VkImageTiling tiling, VkFormatFeatureFlags features);

		void TransitionImageLayout(const VkDevice l_device, const VkCommandPool cmdPool, const VkQueue& gfxQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

		void copyBufferToImage(const VkDevice l_device, const VkCommandPool cmdPool, VkBuffer buffer, const VkQueue gfxQueue, VkImage image, uint32_t width, uint32_t height);

		void GenerateMipMaps(const VkPhysicalDevice p_device, const VkDevice l_device, const VkCommandPool& cmdPool, const VkQueue gfxQueue, VkImage image, VkFormat imgFormat, uint32_t textureWidth, uint32_t textureHeight, uint32_t mipLevels);

		uint32_t CalculateMipLevels(const uint32_t& imageWidth, const uint32_t& imageHeight);

		std::string ReadFile(const char* filename);

		void WriteSpirvFile(const char* filename, const std::vector<uint32_t>& data);
	}
}