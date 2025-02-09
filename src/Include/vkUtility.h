#pragma once

#include "Common.h"
#include <array>
#include <cassert>
#include <iostream>

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

		VkPipelineShaderStageCreateInfo CreateShaderModule(VkDevice logicalDevice, const char* name, VkShaderModule& shaderModule, VkShaderStageFlagBits stage);

		/* IDEA:
		VkPipelineShaderStageCreateInfo PipelineStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);*/

		VkFormat findSupportedFormat(const VkPhysicalDevice p_device, const std::vector<VkFormat>& possibleFormats,
			VkImageTiling tiling, VkFormatFeatureFlags features);
	}
}